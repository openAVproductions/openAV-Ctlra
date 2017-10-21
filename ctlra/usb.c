#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>

#include "impl.h"

#include <libusb.h>

#define USB_PATH_MAX 256

#define CTLRA_USE_ASYNC_XFER 1

#ifndef LIBUSB_HOTPLUG_MATCH_ANY
#define LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT 0xcafe
#define LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED 0xcafe
#define LIBUSB_HOTPLUG_MATCH_ANY 0xcafe
#define LIBUSB_CAP_SUPPORTS_DETACH_KERNEL_DRIVER 0xcafe
#define LIBUSB_CAP_HAS_HOTPLUG 0xcafe
#warning "Please update your very old libusb version"
#define libusb_hotplug_event int
typedef int libusb_hotplug_callback_handle;
int libusb_hotplug_register_callback() {return 0xcafe;};
int libusb_set_auto_detach_kernel_driver() {return 0xcafe;}
#endif

/* From cltra.c */
extern int ctlra_impl_get_id_by_vid_pid(uint32_t vid, uint32_t pid);
extern int ctlra_impl_accept_dev(struct ctlra_t *ctlra, int dev_id);
extern int ctlra_impl_dev_get_by_vid_pid(struct ctlra_t *ctlra, int32_t vid,
					 int32_t pid, struct ctlra_dev_t **out_dev);

/* struct to track async USB transfers */
struct usb_async_t {
	struct usb_async_t *next;
	char malloc_mem[0];
};

#define CTLRA_USB_XFER_SPACE_OR_RET(dev,ret)			\
	do {							\
	if (dev->usb_xfer_head - dev->usb_xfer_tail >=		\
			CTLRA_USB_XFER_COUNT) {			\
		dev->usb_xfer_counts[USB_XFER_ERROR]++;		\
		return ret;					\
	} } while (0)

static inline void
ctlra_usb_impl_xfer_push(struct ctlra_dev_t *dev, void *xfer)
{
	dev->usb_xfer_head++;
	if(dev->usb_xfer_head >= CTLRA_USB_XFER_COUNT)
		dev->usb_xfer_head = 0;

	dev->usb_xfer_ptr[dev->usb_xfer_head] = xfer;
	printf("xfr %p at %d\n", xfer, dev->usb_xfer_head);
}

static inline void
ctlra_usb_impl_xfer_pop(struct ctlra_dev_t *dev)
{
	dev->usb_xfer_tail++;
	if(dev->usb_xfer_head >= CTLRA_USB_XFER_COUNT)
		dev->usb_xfer_head = 0;
	dev->usb_xfer_ptr[dev->usb_xfer_tail] = NULL;
}

static inline void
ctlra_usb_impl_xfer_release(struct ctlra_dev_t *dev)
{
	for(int i = 0; i < CTLRA_USB_XFER_COUNT; i++) {
		if(dev->usb_xfer_ptr[i]) {
			struct libusb_transfer *xfer = dev->usb_xfer_ptr[i];
			int ret = libusb_cancel_transfer(xfer);
			if(ret)
				CTLRA_ERROR(dev->ctlra_context,
					    "usb cancel xfer failed: %s\n",
					    libusb_strerror(ret));
		}
	}
}

static int ctlra_usb_impl_get_serial(struct libusb_device_handle *handle,
				     uint8_t desc_serial, uint8_t *buffer,
				     uint32_t buf_size)
{
	if (desc_serial > 0) {
		int ret = libusb_get_string_descriptor_ascii(handle,
							     desc_serial,
							     buffer,
							     buf_size);
		if (ret < 0)
			return -1;

		return 0;
	}
	return -1;
}

static int ctlra_usb_impl_hotplug_cb(libusb_context *ctx,
                                     libusb_device *dev,
                                     libusb_hotplug_event event,
                                     void *user_data)
{
	int ret;
	struct ctlra_t *ctlra = user_data;
	struct libusb_device_descriptor desc;
	ret = libusb_get_device_descriptor(dev, &desc);
	if(ret != LIBUSB_SUCCESS) {
		CTLRA_ERROR(ctlra, "libusb err device desc: %d\n", ret);
		return -1;
	}
	if(event == LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT) {
		/* Quirks:
		 * If a device is unplugged, usually the libusb read/write
		 * will fail, causing the device to be banished, and then
		 * cleaned up and removed automatically by Ctlra. The
		 * exception is devices that read /dev/hidrawX manually,
		 * because they return -1 if no data is available or there
		 * is an error reading the file descriptor.
		 *
		 * The solution used here it to use libusb to detect the
		 * removal of the device, and then banish the ctlra_dev_t
		 * instance if it matches the device */
		CTLRA_INFO(ctlra, "Device removed: %04x:%04x\n",
			   desc.idVendor, desc.idProduct);

		/* NI Maschine Mikro MK2 */
		if(desc.idVendor == 0x17cc && desc.idProduct == 0x1200) {
			struct ctlra_dev_t *ni_mm;
			int err = ctlra_impl_dev_get_by_vid_pid(ctlra,
								0x17cc,
								0x1200,
								&ni_mm);
			ctlra_dev_disconnect(ni_mm);
		}
		return 0;
	}

	if(event == LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED) {

		libusb_device_handle *handle = 0;
		ret = libusb_open(dev, &handle);
		if(ret != LIBUSB_SUCCESS)
			return -1;
		uint8_t buf[255];
		ret = ctlra_usb_impl_get_serial(handle, desc.iSerialNumber,
					  buf, 255);
		if(ret)
			snprintf((char *)buf, sizeof(buf), "---");
		CTLRA_INFO(ctlra, "Device attached: %04x:%04x, serial %s\n",
			   desc.idVendor, desc.idProduct, buf);
		/* Quirks:
		 * Here we can handle strange hotplug issues. For example,
		 * controllers that have a USB hub integrated show as the
		 * hub first (so the hotplug picks up that VID/PID pair,
		 * not the device itself for some reason). Here we can
		 * modify the VID/PID pair based on known corner cases:
		 */
		uint32_t quirk_vid = desc.idVendor;
		uint32_t quirk_pid = desc.idProduct;
		switch(quirk_vid) {
		case 0x17cc:
			/* NI Kontrol D2, change PID from 0x1403 (hub) back
			 * to the normal PID of 0x1400 */
			if(quirk_pid == 0x1403)
				quirk_pid = 0x1400;
			break;
		default: break;
		};

		int id = ctlra_impl_get_id_by_vid_pid(quirk_vid, quirk_pid);
		if(id < 0) {
			/* Device is not supported by Ctlra, so release
			 * the libusb handle which was opened to retrieve
			 * the serial from the device */
			CTLRA_WARN(ctlra, "Ctlra does not support hotplugged device %x %x\n",
				   quirk_vid, quirk_pid);
			libusb_close(handle);
			return -1;
		}

		int accepted = ctlra_impl_accept_dev(ctlra, id);
		if(!accepted)
			libusb_close(handle);
		return 0;
	}

	return 0;
}

void ctlra_impl_usb_idle_iter(struct ctlra_t *ctlra)
{
	struct timeval tv = {0};
	/* 1st: NULL context
	 * 2nd: timeval to wait - 0 returns as if non blocking
	 * 3rd: int* to completed event - unused by Ctlra */
	libusb_handle_events_timeout_completed(ctlra->ctx, &tv, NULL);
}

int ctlra_dev_impl_usb_init(struct ctlra_t *ctlra)
{
	int ret;
	/* TODO: move this to a usb specific cltra_init() function */
	if(ctlra->usb_initialized)
		return -1;

	/* Do not create a new USB context if we're flagged not to */
	if(ctlra->opts.flags_usb_no_own_context)
		ret = libusb_init(NULL);
	else
		ret = libusb_init(&ctlra->ctx);

	if (ret < 0) {
		CTLRA_ERROR(ctlra, "failed to initialise libusb: %s\n",
		       libusb_error_name(ret));
		return -1;
	}
	ctlra->usb_initialized = 1;

	if(!libusb_has_capability (LIBUSB_CAP_HAS_HOTPLUG)) {
		CTLRA_WARN(ctlra, "Ctlra: Hotplug support on platform: %d\n", 0);
		return -2;
	}

	/* setup hotplug callbacks */
	libusb_hotplug_callback_handle hp[2];
	ret = libusb_hotplug_register_callback(NULL,
					       /* Register arrive and leave */
					       LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED |
					       LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT,
					       0,
					       LIBUSB_HOTPLUG_MATCH_ANY,
					       LIBUSB_HOTPLUG_MATCH_ANY,
					       LIBUSB_HOTPLUG_MATCH_ANY,
					       ctlra_usb_impl_hotplug_cb,
					       ctlra,
					       &hp[0]);
	if (ret != LIBUSB_SUCCESS)
		CTLRA_WARN(ctlra, "hotplug register failure: %d\n", ret);

	return 0;
}

int ctlra_dev_impl_usb_open(struct ctlra_dev_t *ctlra_dev, int vid,
                            int pid)
{
	int ret;

	libusb_device **devs;
	libusb_device *dev;
	int i = 0, j = 0;
	uint8_t path[USB_PATH_MAX];

	int cnt = libusb_get_device_list(NULL, &devs);
	if (cnt < 0)
		goto fail;

	struct ctlra_t *ctlra = ctlra_dev->ctlra_context;

	while ((dev = devs[i++]) != NULL) {
		struct libusb_device_descriptor desc;
		int r = libusb_get_device_descriptor(dev, &desc);
		if (r < 0) {
			CTLRA_ERROR(ctlra, "device desc open failed %d", r);
			goto fail;
		}
#if 0
		printf("%04x:%04x (serial %d) (bus %d, device %d)",
		       desc.idVendor, desc.idProduct, desc.iSerialNumber,
		       libusb_get_bus_number(dev),
		       libusb_get_device_address(dev));

		r = libusb_get_port_numbers(dev, path, sizeof(path));
		if (r > 0) {
			printf(" path: %d", path[0]);
			for (j = 1; j < r; j++)
				printf(".%d", path[j]);
		}
		printf("\n");
#endif

		if(desc.idVendor  == vid &&
		    desc.idProduct == pid) {
			ctlra_dev->info.serial_number = desc.iSerialNumber;
			ctlra_dev->info.vendor_id     = desc.idVendor;
			ctlra_dev->info.device_id     = desc.idProduct;
			break;
		}
	}

	libusb_free_device_list(devs, 1);

	if(!dev)
		goto fail;
	ctlra_dev->usb_device = dev;

	memset(ctlra_dev->usb_handle, 0,
	       sizeof(ctlra_dev->usb_handle));

	return 0;
fail:
	return -1;
}

int ctlra_dev_impl_usb_open_interface(struct ctlra_dev_t *ctlra_dev,
                                      int interface,
                                      int handle_idx)
{
	struct ctlra_t *ctlra = ctlra_dev->ctlra_context;

	if(handle_idx >= CTLRA_USB_IFACE_PER_DEV) {
		CTLRA_ERROR(ctlra,
			    "request for handle beyond available iface per dev range %d\n",
			    handle_idx);
		return -1;
	}
	libusb_device *usb_dev = ctlra_dev->usb_device;
	libusb_device_handle *handle = 0;

	/* now that we've found the device, open the handle */
	int ret = libusb_open(usb_dev, &handle);
	if(ret != LIBUSB_SUCCESS) {
		CTLRA_ERROR(ctlra, "Error in opening interface, dev %s\n",
		       ctlra_dev->info.device);
		return -1;
	}

	ctlra_usb_impl_get_serial(handle, ctlra_dev->info.serial_number,
				  (uint8_t*)ctlra_dev->info.serial,
				  CTLRA_DEV_SERIAL_MAX);

	/* enable auto management of kernel claiming / unclaiming */
	if (libusb_has_capability(LIBUSB_CAP_SUPPORTS_DETACH_KERNEL_DRIVER)) {
		ret = libusb_set_auto_detach_kernel_driver(handle, 1);
		if(ret != LIBUSB_SUCCESS) {
			CTLRA_ERROR(ctlra,
				    "Enable auto-kernel unclaiming err: %d\n",
				    ret);
			return -1;
		}
	} else {
		printf("Warning: auto kernel claim/unclaiming not supported\n");
	}

	ret = libusb_claim_interface(handle, interface);
	if(ret != LIBUSB_SUCCESS) {
		CTLRA_ERROR(ctlra,
			    "Ctlra: Could not claim interface %d of dev %s, continuing...\n",
			    interface, ctlra_dev->info.device);
		int kernel_active = libusb_kernel_driver_active(handle,
		                    interface);
		if(kernel_active)
			CTLRA_ERROR(ctlra,
				    "Kernel has claimed the interface (%d): Stop other applications using this device and retry\n",
				    kernel_active);
		return -1;
	}

	/* Commit to success: update handles in struct and return ok*/
	ctlra_dev->usb_handle[handle_idx] = handle;
	ctlra_dev->usb_interface[handle_idx] = interface;

	return 0;
}

#if CTLRA_USE_ASYNC_XFER
static void ctlra_usb_xfr_write_done_cb(struct libusb_transfer *xfr)
{
	int failed = 0;
	switch(xfr->status) {
	/* Success */
	case LIBUSB_TRANSFER_COMPLETED:
		//printf("write completed OK\n");
		break;
	case LIBUSB_TRANSFER_TIMED_OUT:
		//printf("write timed out\n");
		break;
	default:
		//printf("write hit default error condition\n");
		break;
	}
}

static void ctlra_usb_xfr_done_cb(struct libusb_transfer *xfr)
{
	struct ctlra_dev_t *dev = xfr->user_data;
	struct ctlra_t *ctlra = dev->ctlra_context;

	switch(xfr->status) {
	/* Success */
	case LIBUSB_TRANSFER_COMPLETED: {
		dev = xfr->user_data;
		if(!dev->usb_read_cb) {
			CTLRA_ERROR(ctlra, "DRIVER ERROR: USB READ CB = %d\n", 0);
			break;
		}
		if(dev->usb_xfer_outstanding == 0) {
			CTLRA_ERROR(ctlra,
				    "oustanding xfers going negative %d\n",
				    dev->usb_xfer_outstanding);
		}
		dev->usb_xfer_outstanding--;
		ctlra_usb_impl_xfer_pop(dev);
		dev->usb_read_cb(dev, xfr->endpoint, xfr->buffer,
				 xfr->actual_length);
		} break;
	case LIBUSB_TRANSFER_CANCELLED:
		dev->usb_xfer_outstanding--;
		ctlra_usb_impl_xfer_pop(dev);
		break;

	/* Timeouts *can* happen, but are rare. */
	case LIBUSB_TRANSFER_TIMED_OUT:
		//printf("timeout... ctlra dev %p\n", xfr->user_data);
		break;
	/* Anything here is an error, and the device will be banished */
	case LIBUSB_TRANSFER_NO_DEVICE:
	case LIBUSB_TRANSFER_ERROR:
	case LIBUSB_TRANSFER_STALL:
	case LIBUSB_TRANSFER_OVERFLOW:
		CTLRA_DRIVER(ctlra, "Ctlra: USB transfer error %s\n",
			     libusb_error_name(xfr->status));
		dev = xfr->user_data;
		dev->banished = 1;
		break;
	default:
		CTLRA_DRIVER(ctlra, "USB transaction has unknown status: %d\n",
			    xfr->status);
		break;
	}

	struct usb_async_t *async = (struct usb_async_t *)
		(((uint8_t *)xfr->buffer) - offsetof(struct usb_async_t, malloc_mem));
	CTLRA_DRIVER(ctlra, "free async @ %p\n", async);

	free(async);
	libusb_free_transfer(xfr);
}
#endif /* CTLRA_USE_ASYNC_XFER */

int ctlra_dev_impl_usb_interrupt_read(struct ctlra_dev_t *dev, uint32_t idx,
                                      uint32_t endpoint, uint8_t *data,
                                      uint32_t size)
{
	int transferred;
	struct ctlra_t *ctlra = dev->ctlra_context;

/* we can use synchronous reads too, but the latency builds up of the
 * timeout. AKA: with 6 devices, at 100 ms each, 600ms between a re-poll
 * of the USB device - totally unacceptable.
 * The ASYNC method allows having reads outstanding on devices at the same
 * time, so should be preferred, unless there is a good reason to use the
 * sync method.
 */
#if CTLRA_USE_ASYNC_XFER
	CTLRA_USB_XFER_SPACE_OR_RET(dev, 0);

	/* timeout of zero means no timeout. For ASync case, this means
	 * the buffer will wait until data becomes available - good! */
	const uint32_t timeout = 0;
	struct libusb_transfer *xfr;
	xfr = libusb_alloc_transfer(0);
	if(xfr == 0) {
		CTLRA_DRIVER(ctlra, "xfr == %p\n", xfr);
	}

	if(dev->usb_xfer_outstanding >= CTLRA_USB_XFER_COUNT - 1) {
		CTLRA_DRIVER(ctlra, "int read, but xfer outstanding = %d\n",
			   dev->usb_xfer_outstanding);
		dev->usb_xfer_counts[USB_XFER_ERROR]++;
		return 0;
	}
	printf("%s : %d\n", __func__, __LINE__);

	/* Malloc space for the USB transaction - not ideal, but we have
	 * to pass ownership of the data to the USB library, and we can't
	 * pass the actual dev_t owned data, since the application may
	 * update it again before the USB transaction completes.
	 *
	 * Ctlra has to track the async references to cancel them for a
	 * clean shutdown. Hence, we allocate a usb_async_t struct, which
	 * is used as a linked list for xfers, at little extra overhead
	 * due to just allocating a slightly block, and keeping the list
	 * pointer at the start of the block, before the libusb xfer mem */
	struct usb_async_t *async = malloc(size + sizeof(struct usb_async_t));
	/* insert at head into linked list for device */
	async->next = dev->usb_async_next;
	dev->usb_async_next = async;
	void *usb_data = async->malloc_mem;
	printf("alloc async @ %p\n", async);

	libusb_fill_interrupt_transfer(xfr,
				  dev->usb_handle[idx],
	                          endpoint,
	                          usb_data,
	                          size,
	                          ctlra_usb_xfr_done_cb,
	                          dev,
	                          timeout);

	int res = libusb_submit_transfer(xfr);
	/* Only error experienced while developing was ERROR_IO, which was
	 * caused by stress testing the reading of multiple devices over
	 * time. The _IO error would show after (almost exactly) 1 minute
	 * of read requests. All button presses are still captured, and
	 * writes to LEDs are serviced correctly. There is no negative
	 * impact of these IO errors - so just free buffers and next iter
	 * of reads will catch any data if available */
	if(res) {
		libusb_free_transfer(xfr);
		free(usb_data);
		if(res == LIBUSB_ERROR_IO)
			return 0;

		printf("error submitting data: %s\n", libusb_error_name(res));
		return -1;
	}

	printf("%s : %d\n", __func__, __LINE__);
	dev->usb_xfer_outstanding++;
	dev->usb_xfer_counts[USB_XFER_INT_READ]++;
	ctlra_usb_impl_xfer_push(dev, xfr);

	/* do we want to return the size here? */
	/* This read op is async - there *IS* no data to read right now.
	 * We need a ring to store the data in an async way, so the xfer
	 * done callback can queue data to be returned here. AKA: we need
	 * to read data from the ring here now */
	return 0;
#else
	/* SYNC case, timeout is a balance between causing lag in the
	 * polling of the next device, and USB reads returning ERROR_TIMEOUT
	 * instead of actual data. This depends on the host system - laptops
	 * are significantly slower in servicing USB times than desktops */
	const uint32_t timeout = 100;
	int r = libusb_interrupt_transfer(dev->usb_handle[idx], endpoint,
	                                  data, size, &transferred, timeout);
	if(r == LIBUSB_ERROR_TIMEOUT)
		return 0;
	/* buffer too small, indicates data available. Could be used as
	 * canary to check if data available without reading it.
	if(r == LIBUSB_ERROR_OVERFLOW)
		return 0;
	*/
	if (r < 0) {
		CTLRA_DRIVER(ctlra, "dev banished, usb error %s : %s\n",
			libusb_error_name(r), libusb_strerror(r));
		ctlra_dev_impl_banish(dev);
		return r;
	}

	if(!dev->usb_read_cb) {
		CTLRA_ERROR(ctlra, "DRIVER ERROR: no USB READ CB = %p!\n",
			    dev->usb_read_cb);
		return 0;
	}
	dev->usb_read_cb(dev, endpoint, data, transferred);
	dev->usb_xfer_counts[USB_XFER_INT_READ]++;
	return r;
#endif /* CTLRA_USE_ASYNC_XFER */

}

int ctlra_dev_impl_usb_interrupt_write(struct ctlra_dev_t *dev, uint32_t idx,
                                       uint32_t endpoint, uint8_t *data,
                                       uint32_t size)
{
	int transferred;
	struct ctlra_t *ctlra = dev->ctlra_context;
	const uint32_t timeout = 100;

	CTLRA_USB_XFER_SPACE_OR_RET(dev, -ENOBUFS);

	if(dev->usb_xfer_outstanding > CTLRA_USB_XFER_COUNT - 1) {
		dev->usb_xfer_counts[USB_XFER_ERROR]++;
		return 0;
	}

#if CTLRA_USE_ASYNC_XFER
	struct libusb_transfer *xfr;
	xfr = libusb_alloc_transfer(0);
	if(xfr == 0)
		CTLRA_ERROR(ctlra, "write xfr == %p!\n", xfr);

	/* Malloc space for the USB transaction - not ideal, but we have
	 * to pass ownership of the data to the USB library, and we can't
	 * pass the actual dev_t owned data, since the application may
	 * update it again before the USB transaction completes. */
	void *usb_data = malloc(size);
	if(!usb_data) {
		dev->usb_xfer_counts[USB_XFER_ERROR]++;
		return -ENOSPC;
	}

	memcpy(usb_data, data, size);

	libusb_fill_interrupt_transfer(xfr,
				       dev->usb_handle[idx],
				       endpoint,
				       usb_data,
				       size,
				       ctlra_usb_xfr_write_done_cb,
				       dev, /* userdata - pass dev to
					       banish it if required */
				       timeout);
	if(libusb_submit_transfer(xfr) < 0) {
		libusb_free_transfer(xfr);
		free(usb_data);
		//printf("error submitting data!!\n");
		return -1;
	}

	dev->usb_xfer_counts[USB_XFER_INT_WRITE]++;
	printf("%s : %d\n", __func__, __LINE__);
	dev->usb_xfer_outstanding++;
	ctlra_usb_impl_xfer_push(dev, xfr);

	/* do we want to return the size here? */
	/* This read op is async - there *IS* no data written yet */
	return size;
#else
	int r = libusb_interrupt_transfer(dev->usb_handle[idx], endpoint,
	                                  data, size, &transferred, timeout);
	if(r == LIBUSB_ERROR_TIMEOUT)
		return 0;
	else if(r == LIBUSB_ERROR_BUSY)
		return 0;
	if (r < 0) {
		fprintf(stderr, "ctlra: usb error %s : %s\n",
			libusb_error_name(r), libusb_strerror(r));
		ctlra_dev_impl_banish(dev);
		return r;
	}
	dev->usb_xfer_counts[USB_XFER_INT_WRITE]++;
	return transferred;
#endif /* CTLRA_USE_ASYNC_XFER */
}

int ctlra_dev_impl_usb_bulk_write(struct ctlra_dev_t *dev, uint32_t idx,
                                  uint32_t endpoint, uint8_t *data,
                                  uint32_t size)
{
	const uint32_t timeout = 100;
	struct ctlra_t *ctlra = dev->ctlra_context;
	int transferred;
	int r = libusb_bulk_transfer(dev->usb_handle[idx], endpoint,
	                               data, size, &transferred, timeout);
	if(r == LIBUSB_ERROR_TIMEOUT)
		return 0;
	if (r < 0) {
		CTLRA_ERROR(ctlra, "usb error %s : %s, bulk write count %d\n",
			libusb_error_name(r), libusb_strerror(r),
			dev->usb_xfer_counts[USB_XFER_BULK_WRITE]);
		ctlra_dev_impl_banish(dev);
		return r;
	}

	dev->usb_xfer_counts[USB_XFER_BULK_WRITE]++;
	return transferred;

}

void ctlra_dev_impl_usb_close(struct ctlra_dev_t *dev)
{
	struct ctlra_t *ctlra = dev->ctlra_context;

	ctlra_usb_impl_xfer_release(dev);

	usleep(1000 * 100);
	ctlra_impl_usb_idle_iter(ctlra);

	for(int i = 0; i < CTLRA_USB_IFACE_PER_DEV; i++) {

		if(dev->usb_handle[i]) {
			int ret = libusb_release_interface(dev->usb_handle[i],
							   dev->usb_interface[i]);
			if(ret == LIBUSB_ERROR_NOT_FOUND) {
				// Seems to always happen? LibUSB bug?
				CTLRA_ERROR(ctlra, "release interface error: interface %d not found\n", i);
			} else if(ret < 0)
				CTLRA_ERROR(ctlra, "libusb release interface error: %s\n",
					libusb_strerror(ret));

			/* close() takes a handle* ptr... */
			libusb_close(dev->usb_handle[i]);
		}
	}
}

void ctlra_impl_usb_shutdown(struct ctlra_t *ctlra)
{

	if(ctlra->opts.flags_usb_no_own_context)
		libusb_exit(NULL);
	else
		libusb_exit(ctlra->ctx);
}

