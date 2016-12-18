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
#if 0 /* verbose ctlr logging */
		printf("%s : usb device open failed for device %s\n",
		       __func__, ctlr_dev->info.device);
#endif
		return -ENXIO;
	}

	ret = hid_set_nonblocking(ctlr_dev->hidapi_usb_handle, 1);
	if(ret < 0)
		printf("%s: Warning, failed to set device %s to non-blocking\n",
		       __func__, ctlr_dev->info.device);

	return 0;
fail:
	return -ENODEV;
}

int ctlr_dev_impl_usb_read(struct ctlr_dev_t *dev, int handle_idx,
                           uint8_t *data, uint32_t size)
{
	int result = hid_read_timeout(dev->hidapi_usb_handle, data, size, 500);
	printf("hid read timeout\n");
	if (result > 0) {
		//int res = hid_read(dev->hidapi_usb_handle, data, size);
		return 0;
	}
}

int ctlr_dev_impl_usb_xfer(struct ctlr_dev_t *dev, int handle_idx,
                           int endpoint,
                           uint8_t *data, uint32_t size)
{
	//int res = hid_write(handle, buf, 128);
	int res = hid_read(dev->hidapi_usb_handle, data, size);
	if (res < 0) {
#warning TODO: exception path, *error* on read, so stop polling
	}

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
