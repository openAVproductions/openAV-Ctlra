#include <errno.h>
#include <stdio.h>

#include "ctlr.h"
#include "devices.h"
#include "devices/device_impl.h"

static int ctlr_libusb_initialized;

int ctlr_dev_impl_usb_open(int vid, int pid, struct ctlr_dev_t *ctlr_dev,
			   uint32_t num_skip)
{
	int ret;

	if(!ctlr_libusb_initialized) {
		ret = libusb_init (NULL);
		if (ret < 0) {
			printf("failed to initialise libusb: %s\n", libusb_error_name(ret));
			goto fail;
		}
	}

	libusb_device **devs;
	libusb_device *dev;
	libusb_device_handle *handle;
	int i = 0, j = 0;
	uint8_t path[8];

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
#if 0
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

		if(desc.idVendor == vid &&
		    desc.idProduct == pid) {
			break;
		}
	}

	libusb_free_device_list(devs, 1);

	if(!dev)
		goto fail;

	/* now that we've found the device, open the handle */
	ret = libusb_open(dev, &handle);
	if(ret != LIBUSB_SUCCESS) {
		printf("Error in claiming interface\n");
		goto fail;
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

#warning fixme: Maschine uses interface 0 - claim others?
#define MASCHINE_INTERFACE 0
	ret = libusb_claim_interface(handle, MASCHINE_INTERFACE);
	if(ret != LIBUSB_SUCCESS) {
		printf("Error in claiming interface\n");
		int kernel_active = libusb_kernel_driver_active(handle,
							MASCHINE_INTERFACE);
		if(kernel_active)
			printf("=> Kernel has claimed the interface."
			       "Stop other applications using this device and retry\n");
		return -1;
	}

	ctlr_dev->usb_device = dev;
	ctlr_dev->usb_handle = handle;
	return 0;
fail:
	return -ENODEV;
}

void ctlr_dev_impl_usb_close(struct ctlr_dev_t *dev)
{
	libusb_close(dev->usb_handle);
}
