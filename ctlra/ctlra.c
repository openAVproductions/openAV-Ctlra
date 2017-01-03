#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ctlra.h"
#include "devices.h"
#include "device_impl.h"

/* For USB initialization */
int ctlra_dev_impl_usb_init(struct ctlra_t *ctlra);
/* For polling hotplug / other events */
void ctlra_impl_usb_idle_iter(struct ctlra_t *);
/* For cleaning up the USB subsystem */
void ctlra_impl_usb_shutdown(struct ctlra_t *ctlra);

struct ctlra_dev_connect_func_t {
	enum ctlra_dev_id_t id;
	uint32_t vid;
	uint32_t pid;
	ctlra_dev_connect_func connect;
};

#warning TODO: refactor this to be a tailq and have devices register themselves in the .c file instead of all here

DECLARE_DEV_CONNECT_FUNC(ni_kontrol_d2_connect);
DECLARE_DEV_CONNECT_FUNC(ni_kontrol_z1_connect);
DECLARE_DEV_CONNECT_FUNC(ni_kontrol_f1_connect);
DECLARE_DEV_CONNECT_FUNC(ni_kontrol_x1_mk2_connect);
DECLARE_DEV_CONNECT_FUNC(ni_maschine_mikro_mk2_connect);

static const struct ctlra_dev_connect_func_t devices[] = {
	/* D2 shows as 1400 when plugged in, 1403 on hotplug (due to hub) */
	{CTLRA_DEV_NI_KONTROL_D2, 0x17cc, 0x1400, ni_kontrol_d2_connect},
	{CTLRA_DEV_NI_KONTROL_D2, 0x17cc, 0x1403, ni_kontrol_d2_connect},

	{CTLRA_DEV_NI_KONTROL_Z1, 0x17cc, 0x1210, ni_kontrol_z1_connect},
	{CTLRA_DEV_NI_KONTROL_F1, 0x17cc, 0x1120, ni_kontrol_f1_connect},
	{CTLRA_DEV_NI_KONTROL_X1_MK2, 0x17cc, 0x1220, ni_kontrol_x1_mk2_connect},
	//{CTLRA_DEV_NI_MASCHINE_MIKRO_MK2, 0x0, 0x0, ni_maschine_mikro_mk2_connect},
};
#define CTLRA_NUM_DEVS (sizeof(devices) / sizeof(devices[0]))

enum ctlra_dev_id_t ctlra_impl_get_id_by_vid_pid(uint32_t vid, uint32_t pid)
{
	for(unsigned i = 0; i < CTLRA_NUM_DEVS; i++) {
		if(devices[i].vid == vid && devices[i].pid == pid) {
			return devices[i].id;
		}
	}
	return CTLRA_DEV_INVALID;
}

struct ctlra_dev_t *ctlra_dev_connect(struct ctlra_t *ctlra,
				      enum ctlra_dev_id_t dev_id,
				      ctlra_event_func event_func,
				      void *userdata, void *future)
{
	if(dev_id < CTLRA_NUM_DEVS && devices[dev_id].connect) {
		struct ctlra_dev_t *new_dev = 0;
		new_dev = devices[dev_id].connect(event_func,
						  userdata,
						  future);
		if(new_dev) {
			new_dev->ctlra_context = ctlra;
			new_dev->dev_list_next = 0;

			// if list empty, add as main ptr
			if(ctlra->dev_list == 0) {
				ctlra->dev_list = new_dev;
				return new_dev;
			}

			// skip to end of list, and append
			struct ctlra_dev_t *dev_iter = ctlra->dev_list;
			while(dev_iter->dev_list_next)
				dev_iter = dev_iter->dev_list_next;
			dev_iter->dev_list_next = new_dev;
			return new_dev;
		}
	}
	return 0;
}

uint32_t ctlra_dev_poll(struct ctlra_dev_t *dev)
{
	if(dev && dev->poll && !dev->banished) {
		return dev->poll(dev);
	}
	return 0;
}

void ctlra_dev_set_event_func(struct ctlra_dev_t* dev, ctlra_event_func ef)
{
	if(dev)
		dev->event_func = ef;
}

int32_t ctlra_dev_disconnect(struct ctlra_dev_t *dev)
{
	struct ctlra_t *ctlra = dev->ctlra_context;
	struct ctlra_dev_t *dev_iter = ctlra->dev_list;

	if(dev && dev->disconnect) {
		if(dev_iter == dev) {
			ctlra->dev_list = dev_iter->dev_list_next;
			return dev->disconnect(dev);
		}

		while(dev_iter) {
			if(dev_iter->dev_list_next == dev) {
				/* remove next item */
				dev_iter->dev_list_next =
					dev_iter->dev_list_next->dev_list_next;
				break;
			}
			dev_iter = dev_iter->dev_list_next;
		}
		int ret = dev->disconnect(dev);
		return ret;
	}
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

struct ctlra_t *ctlra_create(const struct ctlra_create_opts_t *opts)
{
	struct ctlra_t *c = calloc(1, sizeof(struct ctlra_t));
	if(!c) return 0;

	/* register USB hotplug etc */
	int err = ctlra_dev_impl_usb_init(c);
	if(err)
		printf("%s: impl_usb_init() returned %d\n", __func__, err);

	return c;
}

int ctlra_impl_accept_dev(struct ctlra_t *ctlra,
			  enum ctlra_dev_id_t dev_id)
{
	struct ctlra_dev_t* dev = ctlra_dev_connect(ctlra,
						    dev_id,
						    0x0,
						    0 /* userdata */,
						    0x0);
	if(dev) {
		/* TODO: can we not just pass &dev->func directly? */
		ctlra_event_func    app_event_func    = 0x0;
		ctlra_feedback_func app_feedback_func = 0x0;
		void *              app_func_userdata = 0x0;

		int accepted = ctlra->accept_dev_func(&dev->info,
					&app_event_func,
					&app_feedback_func,
					&app_func_userdata,
					ctlra->accept_dev_func_userdata);
		dev->event_func    = app_event_func;
		dev->feedback_func = app_feedback_func;
		dev->event_func_userdata = app_func_userdata;

		if(!accepted) {
			ctlra_dev_disconnect(dev);
			return 0;
		}
		return 1;
	}
	return 0;
}

int ctlra_probe(struct ctlra_t *ctlra,
		ctlra_accept_dev_func accept_func,
		void *userdata)
{
	/* For each device that we have, iter, attempt to open, and
	 * call the application supplied accept_func callback */
	int num_accepted = 0;
	ctlra->accept_dev_func = accept_func;
	ctlra->accept_dev_func_userdata = userdata;

	for(uint32_t i = 0; i < CTLRA_NUM_DEVS; i++) {
		num_accepted += ctlra_impl_accept_dev(ctlra, devices[i].id);
	}

	return num_accepted;
}


void ctlra_idle_iter(struct ctlra_t *ctlra)
{
	ctlra_impl_usb_idle_iter(ctlra);

	/* Poll events from all */
	struct ctlra_dev_t *dev_iter = ctlra->dev_list;
	while(dev_iter) {
		int poll = ctlra_dev_poll(dev_iter);
		dev_iter = dev_iter->dev_list_next;
		if(dev_iter == 0)
			break;
	}

	/* Then update state of all */
	dev_iter = ctlra->dev_list;
	while(dev_iter) {
		if(!dev_iter->banished) {
			dev_iter->feedback_func(dev_iter,
					dev_iter->event_func_userdata);
		}
		dev_iter = dev_iter->dev_list_next;
	}

	while(ctlra->banished_list) {
		void *tmp = ctlra->banished_list->banished_list_next;
		ctlra_dev_disconnect(ctlra->banished_list);
		ctlra->banished_list = tmp;
	}
}

void ctlra_exit(struct ctlra_t *ctlra)
{
	struct ctlra_dev_t *dev_iter = ctlra->dev_list;
	while(dev_iter) {
		struct ctlra_dev_t *dev_free = dev_iter;
		dev_iter = dev_iter->dev_list_next;
		ctlra_dev_disconnect(dev_free);
	}

	ctlra_impl_usb_shutdown(ctlra);

	free(ctlra);
}


