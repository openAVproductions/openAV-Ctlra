#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>

#include "impl.h"

#include <libusb.h>

#define USB_PATH_MAX 256

#define CTLRA_USE_ASYNC_XFER 1
#define CTLRA_ASYNC_READ_MAX 10

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
	struct usb_async_t *prev;
	struct libusb_transfer *xfer;
	char malloc_mem[0];
};

#include <assert.h>

static inline int __attribute__ ((unused))
ctlra_usb_impl_xfer_validate(struct ctlra_dev_t *dev)
{
	struct ctlra_t *c = dev->ctlra_context;
	struct usb_async_t *current = dev->usb_async_next;

	int i = 0;
	while(current) {
		printf("i = %d, async = %p, next %p\n", i, current, current->next);
		assert(current != current->next);
		current = current->next;
		i++;
	}
	CTLRA_DRIVER(c, "validate done, count = %d\n", i);
	return i;
}

#ifdef XFER_VALIDATE_DEBUG
#define XFER_VALIDATE(dev)						\
	printf("from %s, %d\n", __func__, __LINE__);			\
	ctlra_usb_impl_xfer_validate(dev)
#else
#define XFER_VALIDATE(dev)
#endif

static inline void
ctlra_usb_impl_xfer_release(struct ctlra_dev_t *dev)
{
	struct ctlra_t *c = dev->ctlra_context;
	struct usb_async_t *current = dev->usb_async_next;

	int i = 0;
	while(current) {
		CTLRA_DRIVER(c, "async free %d : %p\n", i, current);
		int ret = libusb_cancel_transfer(current->xfer);
		if(ret) {
			CTLRA_ERROR(c, "usb cancel xfer failed: %s, async %p\n",
				    libusb_strerror(ret), current);
		}
		dev->usb_xfer_counts[USB_XFER_INFLIGHT_CANCEL]++;

		if(current == current->next) {
			CTLRA_ERROR(c, "list corrupt, cur %p == cur->next %p\n",
				    current, current->next);
			break;
		}

		current = current->next;
		/* todo fixme */
		if(!current || i > 10000)
			break;
		i++;
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
static void ctlra_usb_xfr_done_generic(struct libusb_transfer *xfr,
				       const int read)
{
	struct ctlra_dev_t *dev = xfr->user_data;
	struct ctlra_t *ctlra = dev->ctlra_context;

	const int stat_idx =
		read ?  USB_XFER_INFLIGHT_READ : USB_XFER_INFLIGHT_WRITE;

	switch(xfr->status) {
	/* Success */
	case LIBUSB_TRANSFER_COMPLETED: {
		CTLRA_DRIVER(ctlra, "Ctlra: USB transfer completed: size %d\n",
			     xfr->actual_length);
		dev = xfr->user_data;
		if(!dev->usb_read_cb) {
			CTLRA_ERROR(ctlra, "DRIVER ERROR: USB READ CB = %d\n", 0);
			break;
		}

		int inflight_xfers = dev->usb_xfer_counts[stat_idx];
		if(inflight_xfers == 0) {
			CTLRA_ERROR(ctlra,
				    "inflight xfers going negative %d\n",
				    inflight_xfers);
		}
		dev->usb_read_cb(dev, xfr->endpoint, xfr->buffer,
				 xfr->actual_length);
		} break;
	case LIBUSB_TRANSFER_CANCELLED:
		dev->usb_xfer_counts[USB_XFER_CANCELLED]++;
		dev->usb_xfer_counts[USB_XFER_INFLIGHT_CANCEL]--;
		break;
	case LIBUSB_TRANSFER_TIMED_OUT:
		/* Timeouts *can* happen, but are rare. */
		dev->usb_xfer_counts[USB_XFER_TIMEOUT]++;
		break;
	/* Anything here is an error, and the device will be banished */
	case LIBUSB_TRANSFER_NO_DEVICE:
	case LIBUSB_TRANSFER_ERROR:
	case LIBUSB_TRANSFER_STALL:
	case LIBUSB_TRANSFER_OVERFLOW:
		CTLRA_DRIVER(ctlra, "Ctlra: USB transfer error %s, dev banished.\n",
			     libusb_error_name(xfr->status));
		dev = xfr->user_data;
		dev->banished = 1;
		break;
	default:
		CTLRA_DRIVER(ctlra, "USB transaction has unknown status: %d\n",
			    xfr->status);
		break;
	}

	dev->usb_xfer_counts[stat_idx]--;

	/* get async from xfr->buffer address, see usb_async_t struct */
	struct usb_async_t *async = (struct usb_async_t *)
		(((uint8_t *)xfr->buffer) - offsetof(struct usb_async_t, malloc_mem));
	CTLRA_DRIVER(ctlra, "free %s async @ %p\n",
		     read == 1 ? "read" : "write", async);

	XFER_VALIDATE(dev);

	/* remove node from double linked list*/
	struct usb_async_t *next = async->next;
	struct usb_async_t *prev = async->prev;
	CTLRA_DRIVER(ctlra, "async = %p, next %p, prev %p\n", async, next, prev);
	if(next)
		next->prev = prev;
	if(prev) {
		prev->next = next;
	} else {
		dev->usb_async_next = next;
	}

	XFER_VALIDATE(dev);

	free(async);

	libusb_free_transfer(xfr);
}

static void ctlra_usb_xfr_done_cb(struct libusb_transfer *xfr)
{
	const int read = 1;
	ctlra_usb_xfr_done_generic(xfr, read);
}

static void ctlra_usb_xfr_write_done_cb(struct libusb_transfer *xfr)
{
	const int read = 0;
	ctlra_usb_xfr_done_generic(xfr, read);
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
	int inf_reads = dev->usb_xfer_counts[USB_XFER_INFLIGHT_READ];
	if(inf_reads >= CTLRA_ASYNC_READ_MAX) {
		dev->usb_xfer_counts[USB_XFER_ERROR]++;
		return 0;
	}

	/* timeout of zero means no timeout. For ASync case, this means
	 * the buffer will wait until data becomes available - good! */
	const uint32_t timeout = 0;
	struct libusb_transfer *xfr;
	xfr = libusb_alloc_transfer(0);
	if(xfr == 0) {
		CTLRA_DRIVER(ctlra, "xfr == %p\n", xfr);
	}

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

	XFER_VALIDATE(dev);

	/* insert at head into double-linked list for device */
	struct usb_async_t *dev_current = dev->usb_async_next;
	if(dev_current)
		dev_current->prev = async;
	async->next = dev_current;
	async->prev = 0;
	dev->usb_async_next = async;

	XFER_VALIDATE(dev);

	/* back-pointer from async to xfer */
	async->xfer = xfr;

	void *usb_data = &async->malloc_mem;

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


	dev->usb_xfer_counts[USB_XFER_INFLIGHT_READ]++;
	dev->usb_xfer_counts[USB_XFER_INT_READ]++;
	CTLRA_DRIVER(ctlra, "async int read @ %p\n", async);

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
	const uint32_t timeout = 0;

	int inf = dev->usb_xfer_counts[USB_XFER_INFLIGHT_WRITE];
	if(inf >= CTLRA_ASYNC_READ_MAX) {
		dev->usb_xfer_counts[USB_XFER_ERROR]++;
		return 0;
	}

#if CTLRA_USE_ASYNC_XFER
	struct libusb_transfer *xfr;
	xfr = libusb_alloc_transfer(0);
	if(xfr == 0)
		CTLRA_ERROR(ctlra, "write xfr == %p!\n", xfr);

	/* see comment in interrupt read for malloc() and async details */
	struct usb_async_t *async = malloc(size + sizeof(struct usb_async_t));
	if(!async) {
		dev->usb_xfer_counts[USB_XFER_ERROR]++;
		return -ENOSPC;
	}
	XFER_VALIDATE(dev);
	struct usb_async_t *dev_current = dev->usb_async_next;
	if(dev_current)
		dev_current->prev = async;
	async->next = dev_current;
	async->prev = 0;
	dev->usb_async_next = async;
	XFER_VALIDATE(dev);

	/* back-pointer from async to xfer */
	async->xfer = xfr;

	void *usb_data = &async->malloc_mem;
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
	dev->usb_xfer_counts[USB_XFER_INFLIGHT_WRITE]++;

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

	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 10;

	/* if there are inflight writes, these are often to disable any
	 * LEDs or lights on the device. If so, wait a bit, to be nice :)
	 */
	int wait_count = 0;
	do {
		libusb_handle_events_timeout(ctlra->ctx, &tv);
	} while(dev->usb_xfer_counts[USB_XFER_INFLIGHT_WRITE] &&
		wait_count++ < 100);

	int32_t inf_writes = dev->usb_xfer_counts[USB_XFER_INFLIGHT_WRITE];
	if(inf_writes)
		CTLRA_WARN(ctlra, "[%s] inflight writes at close = %d\n"
				  "     Some lights on the device may still be on\n",
			   dev->info.device, inf_writes);

	ctlra_usb_impl_xfer_release(dev);

	libusb_context *ctx = ctlra->ctx;

	int ret = libusb_handle_events_completed(ctlra->ctx, 0);
	int32_t inf_cancels = dev->usb_xfer_counts[USB_XFER_INFLIGHT_CANCEL];
	if(ret || inf_cancels) {
		CTLRA_WARN(ctlra,
			   "[%s] inflight cancels at close = %d, ret %d\n",
			   dev->info.device, inf_cancels, ret);
	}

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

	static const char *usb_xfer_str[] = {
		"Int. Read",
		"Int. Write",
		"Bulk Write",
		"Cancelled",
		"Timeout",
		"Error",
		"Inflight Read",
		"Inflight Write",
		"Inflight Cancel",
	};
	for(int i = 0; i < USB_XFER_COUNT; i++) {
		CTLRA_INFO(ctlra, "[%s] usb %s count (type %d) = %d\n",
			   dev->info.device, usb_xfer_str[i], i,
			   dev->usb_xfer_counts[i]);
	}
}

void ctlra_impl_usb_shutdown(struct ctlra_t *ctlra)
{

	if(ctlra->opts.flags_usb_no_own_context)
		libusb_exit(NULL);
	else
		libusb_exit(ctlra->ctx);
}

