#include <errno.h>

#include "ctlr.h"
#include "devices.h"
#include "device_impl.h"

struct ctlr_dev_connect_func_t {
	enum ctlr_dev_id_t id;
	ctlr_dev_connect_func connect;
};

#warning TODO: refactor this to be a tailq and have devices register themselves in the .c file instead of all here

DECLARE_DEV_CONNECT_FUNC(simple_connect);
DECLARE_DEV_CONNECT_FUNC(ni_kontrol_z1_connect);
DECLARE_DEV_CONNECT_FUNC(ni_kontrol_x1_mk2_connect);
DECLARE_DEV_CONNECT_FUNC(ni_maschine_connect);

static const struct ctlr_dev_connect_func_t devices[] = {
	{CTLR_DEV_SIMPLE, simple_connect},
	{CTLR_DEV_NI_KONTROL_Z1, ni_kontrol_z1_connect},
	{CTLR_DEV_NI_KONTROL_X1_MK2, ni_kontrol_x1_mk2_connect},
	{CTLR_DEV_NI_MASCHINE_MIKRO_MK2, ni_maschine_connect},
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

void ctlr_dev_light_set(struct ctlr_dev_t *dev, uint32_t light_id,
			uint32_t light_status)
{
	if(dev && dev->light_set)
		dev->light_set(dev, light_id, light_status);
}

void ctlr_dev_light_flush(struct ctlr_dev_t *dev, uint32_t force)
{
	if(dev && dev->light_flush)
		dev->light_flush(dev, force);
}

void ctlr_dev_grid_light_set(struct ctlr_dev_t *dev, uint32_t grid_id,
			     uint32_t light_id, uint32_t light_status)
{
	if(dev && dev->grid_light_set)
		dev->grid_light_set(dev, grid_id, light_id, light_status);
}

const char * ctlr_dev_control_get_name(struct ctlr_dev_t *dev,
				       uint32_t control_id)
{
	if(dev && dev->control_get_name)
		return dev->control_get_name(dev, control_id);
	return 0;
}
