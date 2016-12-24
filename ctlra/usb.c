#include <errno.h>
#include <stdio.h>

#include "ctlra.h"
#include "devices.h"
#include "device_impl.h"

#include "libusb.h"

#define USB_PATH_MAX 256

static int ctlra_libusb_initialized;

int ctlra_dev_impl_usb_open(struct ctlra_dev_t *ctlra_dev, int vid, int pid)
{
	int ret;

	if(!ctlra_libusb_initialized) {
		ret = libusb_init (NULL);
		if (ret < 0) {
			printf("failed to initialise libusb: %s\n", libusb_error_name(ret));
			goto fail;
		}
		ctlra_libusb_initialized = 1;
	}

	libusb_device **devs;
	libusb_device *dev;
	libusb_device_handle *handle;
	int i = 0, j = 0;
	uint8_t path[USB_PATH_MAX];

	int cnt = libusb_get_device_list(NULL, &devs);
	if (cnt < 0)
		goto fail;

	while ((dev = devs[i++]) != NULL) {
		struct libusb_device_descriptor desc;
		int r = libusb_get_device_descriptor(dev, &desc);
		if (r < 0) {
			printf("failed to get device descriptor");
			goto fail;
		}
#if 1
		printf("%04x:%04x (bus %d, device %d)",
		       desc.idVendor, desc.idProduct,
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
			break;
		}
	}

	libusb_free_device_list(devs, 1);

	if(!dev)
		goto fail;

	printf("%s: device handle %p\n", __func__, dev);
	ctlra_dev->usb_device = dev;

	return 0;
fail:
	return -1;
}

int ctlra_dev_impl_usb_open_interface(struct ctlra_dev_t *ctlra_dev,
				      int interface,
				      int handle_idx)
{
	if(handle_idx >= CTLRA_USB_IFACE_PER_DEV) {
		printf("request for handle beyond available iface per dev range\n");
		return -1;
	}
	libusb_device *usb_dev = ctlra_dev->usb_device;
	libusb_device_handle *handle = 0;

	/* now that we've found the device, open the handle */
	int ret = libusb_open(usb_dev, &handle);
	if(ret != LIBUSB_SUCCESS) {
		printf("Error in claiming interface\n");
		return -1;
	}
	printf("got device OK\n");

	/* enable auto management of kernel claiming / unclaiming */
	if (libusb_has_capability(LIBUSB_CAP_SUPPORTS_DETACH_KERNEL_DRIVER)) {
		ret = libusb_set_auto_detach_kernel_driver(handle, 1);
		if(ret != LIBUSB_SUCCESS) {
			printf("Error setting auto kernel unclaiming\n");
			return -1;
		}
	} else {
		printf("Warning: auto kernel claim/unclaiming not supported\n");
	}

	ret = libusb_claim_interface(handle, interface);
	if(ret != LIBUSB_SUCCESS) {
		printf("Error in claiming interface\n");
		int kernel_active = libusb_kernel_driver_active(handle,
							interface);
		if(kernel_active)
			printf("=> Kernel has claimed the interface."
			       "Stop other applications using this device and retry\n");
		return -1;
	}

	/* Commit to success: update handles in struct and return ok*/
	ctlra_dev->usb_interface[handle_idx] = handle;

	return 0;

#if 0
	ctlra_dev->hidapi_usb_handle[idx] = hid_open(vid, pid, NULL);
	if(!ctlra_dev->hidapi_usb_handle[idx]) {
#if 0 /* verbose ctlra logging */
		printf("%s : usb device open failed for device %s\n",
		       __func__, ctlra_dev->info.device);
#endif
		return -ENXIO;
	}

	ret = hid_set_nonblocking(ctlra_dev->hidapi_usb_handle[idx], 1);
	if(ret < 0)
		printf("%s: Warning, failed to set device %s to non-blocking\n",
		       __func__, ctlra_dev->info.device);
#endif
	return 0;
}

int ctlra_dev_impl_usb_read(struct ctlra_dev_t *dev, uint32_t idx,
			   uint8_t *data, uint32_t size)
{
	/*
	int res = hid_read(dev->hidapi_usb_handle[idx], data, size);
	if (res < 0) {
#warning TODO: exception path, *error* on read, so stop polling
	}
	return res;
	*/
	return 0;
}

int ctlra_dev_impl_usb_write(struct ctlra_dev_t *dev, uint32_t idx,
			    uint8_t *data, uint32_t size)
{
	/*
	int res = hid_write(dev->hidapi_usb_handle[idx], data, size);
	if (res < 0) {
#warning TODO: exception path, *error* on read, so stop polling
		printf("hid write failed %d\n", res);
	}
	*/

	return 0;
}

void ctlra_dev_impl_usb_close(struct ctlra_dev_t *dev)
{
#if 0
	for(int i = 0; i < CTLRA_USB_IFACE_PER_DEV; i++) {
		if(dev->hidapi_usb_handle[i])
			hid_close(dev->hidapi_usb_handle[i]);
	}
#endif
}

void ctlra_impl_usb_shutdown()
{
#if 0
	int res = hid_exit();
	if(res)
		printf("Ctlr: %s Warning: hid_exit() returns %d\n",
		       __func__, res);
#endif
}

