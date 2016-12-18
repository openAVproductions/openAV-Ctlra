#include <errno.h>
#include <stdio.h>

#include "ctlr.h"
#include "devices.h"
#include "device_impl.h"

#include "hidapi.h"

static int ctlr_libusb_initialized;

int ctlr_dev_impl_usb_open(struct ctlr_dev_t *ctlr_dev, int vid, int pid, 
			   int interface, uint32_t num_skip)
{
	int ret;

	if(!ctlr_libusb_initialized) {
		ret = hid_init();
		if (ret < 0) {
			printf("failed to initialise usb backend: %d\n", ret);
			goto fail;
		}
	}

	ctlr_dev->hidapi_usb_handle = hid_open(vid, pid, NULL);
	if(!ctlr_dev->hidapi_usb_handle) {
		printf("handle not ok\n");
		return -ENXIO;
	}
	printf("handle ok\n");

	hid_set_nonblocking(ctlr_dev->hidapi_usb_handle, 1);

	return 0;
fail:
	return -ENODEV;
}

int ctlr_dev_impl_usb_read(struct ctlr_dev_t *dev, int handle_idx,
			   uint8_t *data, uint32_t size)
{
	//int res = hid_read(dev->hidapi_usb_handle, data, size);
	return 0;
}

int ctlr_dev_impl_usb_xfer(struct ctlr_dev_t *dev, int handle_idx,
			   int endpoint,
			    uint8_t *data, uint32_t size)
{

	//int res = hid_write(handle, buf, 128);
	int res = hid_read(dev->hidapi_usb_handle, data, size);
	return res;
}

void ctlr_dev_impl_usb_close(struct ctlr_dev_t *dev)
{
	hid_close(dev->hidapi_usb_handle);
}

#warning TODO: cleanly shutdown entire USB subsystem
#if 0
void ctlr_impl_usb_shutdown()
{
	res = hid_exit();
	if(res)
		printf("Ctlr: %s Warning: hid_exit() returns %d\n",
		       __func__, red);
}
#endif
