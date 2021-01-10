#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"

#include "impl.h"
#include "usb.h"

#define CTLRA_MAX_DEVICES 64
struct ctlra_dev_connect_func_t __ctlra_devices[CTLRA_MAX_DEVICES];
uint32_t __ctlra_device_count;

__attribute__((constructor(101)))
static void ctlra_static_setup()
{
	/* placeholder */
}

int ctlra_impl_get_id_by_vid_pid(uint32_t vid, uint32_t pid)
{
	for(unsigned i = 0; i < __ctlra_device_count; i++) {
		if(__ctlra_devices[i].vid == vid && __ctlra_devices[i].pid == pid) {
			return i;
		}
	}
	return -1;
}

/* Search trough existing instances, and match against first VID/PID */
int ctlra_impl_dev_get_by_vid_pid(struct ctlra_t *ctlra, int32_t vid,
				  int32_t pid, struct ctlra_dev_t **out_dev)
{
	struct ctlra_dev_t *dev_iter = ctlra->dev_list;
	*out_dev = 0x0;
	while(dev_iter) {
		if(dev_iter->info.vendor_id == vid &&
		   dev_iter->info.device_id == pid) {
			*out_dev = dev_iter;
			return 0;
		}
		dev_iter = dev_iter->dev_list_next;
	}
	return -1;
}

struct ctlra_dev_t *ctlra_dev_connect(struct ctlra_t *ctlra,
				      ctlra_dev_connect_func connect,
				      ctlra_event_func event_func,
				      void *userdata, void *future)
{
	struct ctlra_dev_t *new_dev;

	/* TODO: pass ctlra instance to connect() so the ->ctlra_context
	 * pointer is always valid */
	new_dev = connect(event_func, userdata, future);
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
	return 0;
}


int32_t
ctlra_get_devices_by_vendor(const char *vendor, const char *devices[],
			    int32_t size)
{
	memset(devices, 0, sizeof(char *) * size);
	int device_idx = 0;

	int i;
	for(i = 0; i < __ctlra_device_count; i++) {
		if(__ctlra_devices[i].info) {
			const char *v = __ctlra_devices[i].info->vendor;
			const char *d = __ctlra_devices[i].info->device;

			/* check this device is by vendor */
			if(strcmp(vendor, v) == 0) {
				if(device_idx >= size)
					break;
				devices[device_idx++] = d;
			}
		}
	}

	return device_idx;
}

int32_t
ctlra_get_vendors(const char *vendors[], int32_t size)
{
	memset(vendors, 0, sizeof(char *) * size);
	int vendor_idx = 0;

	int i;
	for(i = 0; i < __ctlra_device_count; i++) {
		if(__ctlra_devices[i].info) {
			const char * v = __ctlra_devices[i].info->vendor;

			/* check this is not duplicate */
			int j = 0;
			int unique = 1;
			while(vendors[j] != 0) {
				if(strcmp(vendors[j], v) == 0) {
					unique = 0;
					break;
				}
				j++;
			}

			if(unique) {
				if(vendor_idx >= size)
					break;
				vendors[vendor_idx++] = v;
			}
		}
	}

	return vendor_idx;
}

int32_t
ctlra_dev_virtualize(struct ctlra_t *c, const char *vendor,
		     const char *device)
{
	int i;
	struct ctlra_dev_info_t *info = 0;

	for(i = 0; i < __ctlra_device_count; i++) {
		if(__ctlra_devices[i].info &&
		   strcmp(vendor, __ctlra_devices[i].info->vendor) == 0 &&
		   strcmp(device, __ctlra_devices[i].info->device) == 0) {
			info = __ctlra_devices[i].info;
			break;
		}
	}

	if(!info) {
		CTLRA_WARN(c, "Couldn't find device '%s' '%s' in %d registered drivers\n",
			   vendor, device, i);
		CTLRA_STRERROR(c, "Device not found\n");
		return -ENODEV;
	}

#ifdef HAVE_AVTKA
	/* TODO: find better solution to this hack */
CTLRA_DEVICE_DECL(avtka);

	/* call into AVTKA and virtualize the device, passing info through
	 * the future (void *) to the AVTKA backend. */
	CTLRA_INFO(c, "virtualizing dev '%s' '%s'\n",
		   info->vendor, info->device);
	struct ctlra_dev_t *dev = ctlra_dev_connect(c, ctlra_avtka_connect,
						    0x0, 0x0, info);
	if(!dev) {
		CTLRA_ERROR(c, "avtka dev returned %p\n", dev);
		return -EINVAL;
	}

	/* assuming info setup is ok, call accept dev callback in app */
	int accepted = c->accept_dev_func(c, &dev->info, dev,
					  c->accept_dev_func_userdata);

	CTLRA_INFO(c, "%s %s %s accepted\n", dev->info.vendor,
		   dev->info.device, accepted ? "" : "not");

	if(!accepted) {
		ctlra_dev_disconnect(dev);
		CTLRA_STRERROR(c, "Application refused device\n");
		return -ECONNREFUSED;
	}
	return 0;
#else
	CTLRA_STRERROR(c, "No virtualized backends available\n");
	return -ENOTSUP;
#endif
}

uint32_t ctlra_dev_poll(struct ctlra_dev_t *dev)
{
	if(dev && dev->poll && !dev->banished) {
		return dev->poll(dev);
	}
	return 0;
}


void
ctlra_dev_set_callback_userdata(struct ctlra_dev_t *dev,
				void *app_userdata)
{
	if(dev) {
		dev->event_func_userdata = app_userdata;
		dev->screen_redraw_ud = app_userdata;
	}
}

void
ctlra_dev_set_event_func(struct ctlra_dev_t* dev, ctlra_event_func f)
{
	if(dev)
		dev->event_func = f;
}

void
ctlra_dev_set_feedback_func(struct ctlra_dev_t *dev,
			    ctlra_feedback_func func)
{
	if(dev)
		dev->feedback_func = func;
}

void
ctlra_dev_set_screen_feedback_func(struct ctlra_dev_t *dev,
				   ctlra_screen_redraw_cb func)
{
	if(dev)
		dev->screen_redraw_cb = func;
}

void
ctlra_dev_set_remove_func(struct ctlra_dev_t *dev,
			  ctlra_remove_dev_func func)
{
	if(dev)
		dev->remove_func = func;
}

int32_t ctlra_dev_disconnect(struct ctlra_dev_t *dev)
{
	struct ctlra_t *ctlra = dev->ctlra_context;
	struct ctlra_dev_t *dev_iter = ctlra->dev_list;

	if(dev && dev->disconnect) {
		/* call the application remove_func() to inform app */
		if(dev->remove_func)
			dev->remove_func(dev, dev->banished,
					 dev->event_func_userdata);

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

void ctlra_dev_feedback_set(struct ctlra_dev_t *dev, uint32_t fb_id,
			    float value)
{
	if(dev && dev->feedback_set)
		dev->feedback_set(dev, fb_id, value);
}

void ctlra_dev_feedback_digits(struct ctlra_dev_t *dev,
			       uint32_t feedback_id,
			       float value)
{
	if(dev && dev->feedback_digits)
		dev->feedback_digits(dev, feedback_id, value);
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

int32_t ctlra_screen_get_data(struct ctlra_dev_t *dev,
				  uint32_t screen_idx,
				  uint8_t **pixels,
				  uint32_t *bytes,
				  struct ctlra_screen_zone_t *redraw,
				  uint8_t flush)
{
	if(dev && dev->screen_get_data)
		return dev->screen_get_data(dev, screen_idx, pixels, bytes,
				     redraw, flush);
	return -ENOTSUP;
}

int32_t ctlra_dev_screen_get_data(struct ctlra_dev_t *dev,
				 uint8_t **pixels,
				 uint32_t *bytes,
				 uint8_t flush)
{
	struct ctlra_screen_zone_t redraw;
	memset(&redraw, 0, sizeof(redraw));
	return ctlra_screen_get_data(dev, 0, pixels, bytes, &redraw, flush);
}

void ctlra_dev_get_info(const struct ctlra_dev_t *dev,
		       struct ctlra_dev_info_t * info)
{
	if(!dev)
		return;
	*info = dev->info;
}

const char *
ctlra_info_get_name(const struct ctlra_dev_info_t *info,
		    enum ctlra_event_type_t type, uint32_t control_id)
{
	if(!info)
		return 0;

	if(info->get_name)
		return info->get_name(type, control_id);

	return "N/A";
}

struct ctlra_t *ctlra_create(const struct ctlra_create_opts_t *opts)
{
	struct ctlra_t *c = calloc(1, sizeof(struct ctlra_t));
	if(!c) return 0;

	/* If options were passed, copy them to the instance */
	if(opts) {
		c->opts = *opts;
	} else {
		/* defaults */
		c->opts.debug_level = CTLRA_DEBUG_ERROR;
		c->opts.screen_redraw_target_fps = 30;
	}

	/* ENV variables override application opts */
	char *ctlra_debug = getenv("CTLRA_DEBUG");
	if(ctlra_debug) {
		int debug_level = atoi(ctlra_debug);
		c->opts.debug_level = debug_level;
		CTLRA_INFO(c, "debug level: %d\n", debug_level);
		CTLRA_INFO(c, "screen redraw target FPS: %d\n",
			   c->opts.screen_redraw_target_fps);
	}

	if(ctlra_debug) {
		CTLRA_INFO(c, "JACK: %s\n", CTLRA_OPT_JACK);
		CTLRA_INFO(c, "ALSA: %s\n", CTLRA_OPT_ALSA);
		CTLRA_INFO(c, "Cairo: %s\n", CTLRA_OPT_CAIRO);
	}

	/* Setup/compute runtime values */
	c->screen_redraw_ns = 1000000000.f / c->opts.screen_redraw_target_fps;

	/* register USB hotplug etc */
	int err = ctlra_dev_impl_usb_init(c);
	if(err)
		CTLRA_ERROR(c, "impl_usb_init() returned %d\n", err);

	return c;
}

int ctlra_impl_accept_dev(struct ctlra_t *ctlra,
			  int id)
{
	if(id < 0 || id >= __ctlra_device_count || !__ctlra_devices[id].connect) {
		CTLRA_WARN(ctlra, "invalid device id recieved %d\n", id);
		return 0;
	}

	struct ctlra_dev_t* dev = ctlra_dev_connect(ctlra,
						    __ctlra_devices[id].connect,
						    0x0,
						    0 /* userdata */,
						    0x0);
	if(dev) {
		/* Store the ctlra context into the dev pointer */
		dev->ctlra_context = ctlra;

		/* Application sets function pointers directly to device */
		int accepted = ctlra->accept_dev_func(ctlra,
						      &dev->info,
						      dev,
						      ctlra->accept_dev_func_userdata);

		CTLRA_INFO(ctlra, "%s %s %s accepted\n", dev->info.vendor,
			   dev->info.device, accepted ? "" : "not");

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
	uint32_t i = 0;
	int num_accepted = 0;

	ctlra->accept_dev_func = accept_func;
	ctlra->accept_dev_func_userdata = userdata;
	for(; i < __ctlra_device_count; i++) {
		num_accepted += ctlra_impl_accept_dev(ctlra, i);
	}

	/* virtualize device from ENV variable */
	char *virt_vendor = getenv("CTLRA_VIRTUAL_VENDOR");
	char *virt_device = getenv("CTLRA_VIRTUAL_DEVICE");
	if(virt_vendor && virt_device) {
		int32_t ret = ctlra_dev_virtualize(ctlra,
						   virt_vendor,
						   virt_device);
		/* could flag error, but dev_virtualize() already does */
		(void) ret;
		/* increment if the device was virtualized successfully */
		num_accepted += (ret == 0);
	}

	return num_accepted;
}

static void
ctlra_impl_screen_redraw(struct ctlra_t *ctlra,
			 struct ctlra_dev_t *dev_iter,
			 uint32_t screen_idx)
{
	uint8_t *pixel;
	uint32_t bytes;
	struct ctlra_screen_zone_t zone_redraw;

	int32_t err = ctlra_screen_get_data(dev_iter, screen_idx,
					    &pixel, &bytes,
					    &zone_redraw, 0);
	if(err) {
		return;
	}

	if(pixel == 0) {
		CTLRA_WARN(ctlra, "pixels[] = %p, programming error\n",
				 pixel);
		return;
	}

	uint8_t px_end_before = pixel[bytes];

	struct ctlra_screen_zone_t redraw;
	int32_t flush = dev_iter->screen_redraw_cb(dev_iter, screen_idx,
						   pixel, bytes, &redraw,
						   dev_iter->screen_redraw_ud);

	uint8_t px_end_after = pixel[bytes];
	if(px_end_before != px_end_after) {
		CTLRA_ERROR(ctlra,
"Application over-runs pixels[] by at least %d. "
"Please file bug on application screen drawing!\n", 1);
	}

	if(!flush)
		return;

	/* Flush data to screen */
	ctlra_screen_get_data(dev_iter, screen_idx, &pixel, &bytes,
			      &redraw, flush);
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
		if(dev_iter->banished) {
			dev_iter = dev_iter->dev_list_next;
			continue;
		}

		if(dev_iter->feedback_func) {
			dev_iter->feedback_func(dev_iter,
				dev_iter->event_func_userdata);
		}

		if(dev_iter->screen_redraw_cb) {
			struct timespec now;
			int err = clock_gettime(CLOCK_MONOTONIC_RAW, &now);
			if(err)
				CTLRA_ERROR(ctlra, "Error getting MONOTONIC_RAW clock: %d\n",
					    err);

			time_t secs = now.tv_sec  - dev_iter->screen_last_redraw.tv_sec;
			long nanos  = now.tv_nsec - dev_iter->screen_last_redraw.tv_nsec;
			uint64_t nanos_elapsed = secs * 1e9 + nanos;

			if (ctlra->screen_redraw_ns <= nanos_elapsed) {
				dev_iter->screen_last_redraw = now;
				for(int i = 0; i < CTLRA_NUM_SCREENS_MAX; i++) {
					ctlra_impl_screen_redraw(ctlra, dev_iter, i);
				}
			}
		}
		dev_iter = dev_iter->dev_list_next;
	}

	/* if any devices were banished (I/O Error, malfunctioned etc)
	 * then we disconnect them here. The dev_disconnect() call will
	 * inform the application if it registered a remove() callback */
	while(ctlra->banished_list) {
		void *tmp = ctlra->banished_list->banished_list_next;
		ctlra_dev_disconnect(ctlra->banished_list);
		ctlra->banished_list = tmp;
	}
}

void ctlra_dev_impl_banish(struct ctlra_dev_t *dev)
{
	struct ctlra_t *ctlra = dev->ctlra_context;
	dev->banished = 1;
	if(ctlra->banished_list == 0)
		ctlra->banished_list = dev;
	else {
		struct ctlra_dev_t *dev_iter = ctlra->banished_list;
		while(dev_iter->banished_list_next)
			dev_iter = dev_iter->banished_list_next;
		dev_iter->banished_list_next = dev;
		dev->banished_list_next = 0;
	}
}


void ctlra_exit(struct ctlra_t *ctlra)
{
	/* Ensures idle_iter is ran before cleanup to try handle any
	 * pending reads/writes */
	ctlra_idle_iter(ctlra);

	struct ctlra_dev_t *dev_iter = ctlra->dev_list;
	while(dev_iter) {
		struct ctlra_dev_t *dev_free = dev_iter;
		dev_iter = dev_iter->dev_list_next;

		ctlra_dev_usb_stats_debug(dev_free);
		ctlra_dev_disconnect(dev_free);
	}

	ctlra_impl_usb_shutdown(ctlra);

	free(ctlra);
}

void ctlra_strerror(struct ctlra_t *ctlra, FILE* out)
{
	fprintf(out, "Ctlra: %s\n", ctlra->strerror);
}
