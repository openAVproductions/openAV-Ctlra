#include <errno.h>

#include "ctlr.h"
#include "devices.h"
#include "devices/device_impl.h"

struct ctlr_dev_connect_func_t {
	enum ctlr_dev_id_t id;
	ctlr_dev_connect_func connect;
};

extern struct ctlr_dev_t *simple_connect(ctlr_event_func event_func,
				  void *userdata, void *future);

static const struct ctlr_dev_connect_func_t devices[] = {
	{CTLR_DEV_SIMPLE, simple_connect},
};
#define CTLR_NUM_DEVS (sizeof(devices) / sizeof(devices[0]))

struct ctlr_dev_t *ctlr_dev_connect(enum ctlr_dev_id_t dev_id,
				    ctlr_event_func event_func,
				    void *userdata,
				    void *future)
{
	if(dev_id < CTLR_NUM_DEVS && devices[dev_id].connect)
		return devices[dev_id].connect(event_func,
					       userdata,
					       future);
	return 0;
}

uint32_t ctlr_dev_poll(struct ctlr_dev_t *dev)
{
	if(dev && dev->poll)
		return dev->poll(dev);
	return 0;
}

int32_t ctlr_dev_disconnect(struct ctlr_dev_t *dev)
{
	if(dev && dev->disconnect)
		return dev->disconnect(dev);
	return -ENOTSUP;
}
