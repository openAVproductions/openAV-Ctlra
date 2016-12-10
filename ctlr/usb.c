#include <errno.h>
#include <stdio.h>

#include "ctlr.h"
#include "devices.h"
#include "device_impl.h"

static int ctlr_libusb_initialized;

int ctlr_dev_impl_usb_open(struct ctlr_dev_t *ctlr_dev, int vid, int pid, 
			   int interface, uint32_t num_skip)
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

	ctlr_dev->usb_device = dev;
	ctlr_dev->usb_handle = handle;
	return 0;
fail:
	return -ENODEV;
}

int ctlr_dev_impl_usb_write(struct ctlr_dev_t *dev, int handle_idx,
			    int endpoint, uint8_t *data, uint32_t size)
{

	int r;
	int transferred;
	r = libusb_interrupt_transfer(dev->usb_handle, endpoint, data,
				      size, &transferred, 1000);
#warning TODO: fix return values here, and document in header
	if (r < 0) {
		fprintf(stderr, "%s intr error %d\n", __func__, r);
		return r;
	}
	if (transferred < size) {
		fprintf(stderr, "short read (%d)\n", r);
		return -1;
	}
	return 0;
}

void ctlr_dev_impl_usb_close(struct ctlr_dev_t *dev)
{
	libusb_close(dev->usb_handle);
}
