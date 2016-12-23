#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "ctlra.h"
#include "devices.h"
#include "device_impl.h"

struct ctlra_dev_connect_func_t {
	enum ctlra_dev_id_t id;
	ctlra_dev_connect_func connect;
};

#warning TODO: refactor this to be a tailq and have devices register themselves in the .c file instead of all here

DECLARE_DEV_CONNECT_FUNC(simple_connect);
DECLARE_DEV_CONNECT_FUNC(ni_kontrol_d2_connect);
DECLARE_DEV_CONNECT_FUNC(ni_kontrol_z1_connect);
DECLARE_DEV_CONNECT_FUNC(ni_kontrol_f1_connect);
DECLARE_DEV_CONNECT_FUNC(ni_kontrol_x1_mk2_connect);
DECLARE_DEV_CONNECT_FUNC(ni_maschine_mikro_mk2_connect);

static const struct ctlra_dev_connect_func_t devices[] = {
	{CTLRA_DEV_SIMPLE, simple_connect},
	{CTLRA_DEV_NI_KONTROL_D2, ni_kontrol_d2_connect},
	{CTLRA_DEV_NI_KONTROL_Z1, ni_kontrol_z1_connect},
	{CTLRA_DEV_NI_KONTROL_F1, ni_kontrol_f1_connect},
	{CTLRA_DEV_NI_KONTROL_X1_MK2, ni_kontrol_x1_mk2_connect},
	{CTLRA_DEV_NI_MASCHINE_MIKRO_MK2, ni_maschine_mikro_mk2_connect},
};
#define CTLRA_NUM_DEVS (sizeof(devices) / sizeof(devices[0]))

struct ctlra_dev_t *ctlra_dev_connect(enum ctlra_dev_id_t dev_id,
				    ctlra_event_func event_func,
				    void *userdata,
				    void *future)
{
	if(dev_id < CTLRA_NUM_DEVS && devices[dev_id].connect)
		return devices[dev_id].connect(event_func,
					       userdata,
					       future);
	return 0;
}

uint32_t ctlra_dev_poll(struct ctlra_dev_t *dev)
{
	if(dev && dev->poll)
		return dev->poll(dev);
	return 0;
}

int32_t ctlra_dev_disconnect(struct ctlra_dev_t *dev)
{
	if(dev && dev->disconnect)
		return dev->disconnect(dev);
	return -ENOTSUP;
}

void ctlra_dev_light_set(struct ctlra_dev_t *dev, uint32_t light_id,
			uint32_t light_status)
{
	if(dev && dev->light_set)
		dev->light_set(dev, light_id, light_status);
}

void ctlra_dev_light_flush(struct ctlra_dev_t *dev, uint32_t force)
{
	if(dev && dev->light_flush)
		dev->light_flush(dev, force);
}

void ctlra_dev_grid_light_set(struct ctlra_dev_t *dev, uint32_t grid_id,
			     uint32_t light_id, uint32_t light_status)
{
	if(dev && dev->grid_light_set)
		dev->grid_light_set(dev, grid_id, light_id, light_status);
}

void ctlra_dev_get_info(const struct ctlra_dev_t *dev,
		       struct ctlra_dev_info_t * info)
{
	if(!dev)
		return;

	memset(info, 0, sizeof(*info));

	snprintf(info->vendor, sizeof(info->vendor), "%s", "Unknown");
	snprintf(info->device, sizeof(info->device), "%s", "Unknown");
	snprintf(info->serial, sizeof(info->serial), "%s", "Unknown");
	info->serial_number = 0;

	snprintf(info->vendor, sizeof(info->vendor), "%s", dev->info.vendor);
	snprintf(info->device, sizeof(info->device), "%s", dev->info.device);
	snprintf(info->serial, sizeof(info->serial), "%s", dev->info.serial);
	info->serial_number = dev->info.serial_number;
}

const char * ctlra_dev_control_get_name(const struct ctlra_dev_t *dev,
					enum ctlra_event_type_t type,
					uint32_t control_id)
{
	if(dev && dev->control_get_name)
		return dev->control_get_name(dev, type, control_id);
	return 0;
}
