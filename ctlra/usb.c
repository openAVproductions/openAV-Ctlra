#include <errno.h>
#include <stdio.h>

#include "ctlra.h"
#include "devices.h"
#include "device_impl.h"

#include "hidapi.h"

static int ctlra_libusb_initialized;

int ctlra_dev_impl_usb_open(struct ctlra_dev_t *ctlra_dev, int vid, int pid,
                           int interface, uint32_t idx)
{
	int ret;

	if(!ctlra_libusb_initialized) {
		ret = hid_init();
		if (ret < 0) {
			printf("failed to initialise usb backend: %d\n", ret);
			return -ENODEV;
		}
		ctlra_libusb_initialized = 1;
	}

	ctlra_dev->hidapi_usb_handle[idx] = hid_open(vid, pid, NULL);
	if(!ctlra_dev->hidapi_usb_handle[idx]) {
#if 1 /* verbose ctlra logging */
		printf("%s : usb device open failed for device %s\n",
		       __func__, ctlra_dev->info.device);
#endif
		return -ENXIO;
	}

#if 1
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
	int res = hid_read(dev->hidapi_usb_handle[idx], data, size);
	if (res < 0) {
#warning TODO: exception path, *error* on read, so stop polling
	}
	return res;
}

int ctlra_dev_impl_usb_write(struct ctlra_dev_t *dev, uint32_t idx,
			    uint8_t *data, uint32_t size)
{
	int res = hid_write(dev->hidapi_usb_handle[idx], data, size);
	if (res < 0) {
#warning TODO: exception path, *error* on read, so stop polling
		printf("hid write failed %d\n", res);
	}

	return res;
}

int ctlra_dev_impl_usb_xfer(struct ctlra_dev_t *dev, int handle_idx,
                           int endpoint,
                           uint8_t *data, uint32_t size)
{
	printf("%s : OLD OUTDATED  - FIX WHERE I'M CALLED FROM!!\n", __func__);
	return -1;
}

void ctlra_dev_impl_usb_close(struct ctlra_dev_t *dev)
{
	for(int i = 0; i < CTLR_USB_IFACE_PER_DEV; i++) {
		if(dev->hidapi_usb_handle[i])
			hid_close(dev->hidapi_usb_handle[i]);
	}
}

#warning TODO: cleanly shutdown entire USB subsystem
#if 0
void ctlra_impl_usb_shutdown()
{
	res = hid_exit();
	if(res)
		printf("Ctlr: %s Warning: hid_exit() returns %d\n",
		       __func__, red);
}
#endif
