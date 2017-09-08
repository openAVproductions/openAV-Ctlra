/*
 * Copyright (c) 2017, OpenAV Productions,
 * Harry van Haaren <harryhaaren@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "impl.h"

#include <avtka/avtka.h>

#define PUGL_PARENT 0x0

#define MAX_ITEMS 1024

/* reverse map from item id to ctlra type/id */
struct id_to_ctlra_t {
	uint16_t type;
	uint16_t encoder_float_delta : 1;
	uint32_t id;
	uint32_t fb_id;
};

/* Represents the the virtual AVTK UI */
struct cavtka_t {
	/* base handles usb i/o etc */
	struct ctlra_dev_t base;
	/* represents the Avtka UI */
	struct avtka_t *a;
	uint32_t canary;
	/* item ids for each ui element, stored by event type */
	struct id_to_ctlra_t id_to_ctlra[MAX_ITEMS];
};

uint32_t avtka_poll(struct ctlra_dev_t *base)
{
	struct cavtka_t *dev = (struct cavtka_t *)base;
	avtka_iterate(dev->a);
	/* events can be "sent" to the app from the widget callbacks */
	return 0;
}

static void avtka_light_set(struct ctlra_dev_t *base,
				    uint32_t light_id,
				    uint32_t light_status)
{
	struct cavtka_t *dev = (struct cavtka_t *)base;
	/* TODO: figure out how to display feedback */
	struct avtka_t *a = dev->a;
	static float v;
	printf("%s: %d %f\n", __func__, light_id, v);
	avtka_item_value(a, light_id, v);
	v += 0.0066;
	if(v > 1.0f) v -= 1.0f;
	avtka_redraw(a);
}

void
avtka_light_flush(struct ctlra_dev_t *base, uint32_t force)
{
	struct cavtka_t *dev = (struct cavtka_t *)base;
}

static int32_t
avtka_disconnect(struct ctlra_dev_t *base)
{
	struct cavtka_t *dev = (struct cavtka_t *)base;

	free(dev);
	return 0;
}

/* TODO: add an Event Mirror cb here, which updates the UI based on the
 * incoming events (from a real hardware device) */

static void
event_cb(struct avtka_t *avtka, uint32_t item, float value, void *userdata)
{
	struct cavtka_t *dev = (struct cavtka_t *)userdata;
	uint32_t type = dev->id_to_ctlra[item].type;
	uint32_t id   = dev->id_to_ctlra[item].id;

	/* default type is button */
	struct ctlra_event_t event = {
		.type = type,
		.button = {
			.id = id,
			.pressed = (value == 1.0),
		},
	};

	/* modify to other type as required */
	switch(type) {
	case CTLRA_EVENT_SLIDER:
		event.slider.id = id;
		event.slider.value = value;
		break;
	case CTLRA_EVENT_GRID:
		event.grid.id = 0;
		event.grid.pos = id;
		event.grid.pressed = (value == 1.0);
	case CTLRA_EVENT_ENCODER:
		event.encoder.id = id;
		if(dev->id_to_ctlra[item].encoder_float_delta)
			event.encoder.delta_float = value;
		else
			event.encoder.delta = (int)value;
	default:
		break;
	}

	/* send event */
	struct ctlra_event_t *e = {&event};
	dev->base.event_func(&dev->base, 1, &e,
			     dev->base.event_func_userdata);
}

#define CTLRA_RESIZE 2
static inline void
ctlra_item_scale(struct avtka_item_opts_t *i)
{
	const uint32_t s = CTLRA_RESIZE;
	i->x *= s;
	i->y *= s;
	i->w *= s;
	i->h *= s;
}

struct avtka_t *
ctlra_build_avtka_ui(struct cavtka_t *dev,
		     struct ctlra_dev_info_t *info);

struct ctlra_dev_t *
ctlra_avtka_connect(ctlra_event_func event_func, void *userdata, void *future)
{
	struct cavtka_t *dev = calloc(1, sizeof(struct cavtka_t));
	if(!dev)
		goto fail;

	dev->canary = 0xcafe;

	struct ctlra_dev_info_t *info = future;

	/* reuse the existing info from the device backend, then update */
	dev->base.info = *info;

	snprintf(dev->base.info.vendor, sizeof(dev->base.info.vendor),
		 "Ctlra Virtual - %s", info->vendor);
	snprintf(dev->base.info.device, sizeof(dev->base.info.device),
		 "%s", info->device);

	dev->base.poll = avtka_poll;
	dev->base.disconnect = avtka_disconnect;
	dev->base.light_set = avtka_light_set;

	dev->base.event_func = event_func;
	dev->base.event_func_userdata = userdata;

	printf("%s, self %p\n", __func__, dev);
	/* pass in back-pointer to ctlra_dev_t class for sending events */
	dev->a = ctlra_build_avtka_ui(dev, info);
	if(!dev->a)
		goto fail;

	return (struct ctlra_dev_t *)dev;
fail:
	printf("%s, failed to create dev, ret 0\n", __func__);
	free(dev);
	return 0;
}

struct avtka_t *
ctlra_build_avtka_ui(struct cavtka_t *dev,
		     struct ctlra_dev_info_t *info)
{
	/* initialize the Avtka UI */
	struct avtka_opts_t opts = {
		.w = info->size_x * CTLRA_RESIZE,
		.h = info->size_y * CTLRA_RESIZE,
		.event_callback = event_cb,
		.event_callback_userdata = dev,
	};
	char name[64];
	snprintf(name, sizeof(name), "%s - %s", dev->base.info.vendor,
		 dev->base.info.device);
	struct avtka_t *a = avtka_create(name, &opts);
	printf("%s %d; avtka_t %p\n", __func__, __LINE__, a);
	if(!a)
		return 0;

	for(int i = 0; i < info->control_count[CTLRA_EVENT_BUTTON]; i++) {
		struct ctlra_item_info_t *item =
			&info->control_info[CTLRA_EVENT_BUTTON][i];
		if(!item)
			break;
		struct avtka_item_opts_t ai = {
			 //.name = name,
			.x = item->x,
			.y = item->y,
			.w = item->w,
			.h = item->h,
			.draw = AVTKA_DRAW_BUTTON,
			.interact = AVTKA_INTERACT_CLICK,
		};
		ctlra_item_scale(&ai);
		uint32_t idx = avtka_item_create(a, &ai);
		if(idx > MAX_ITEMS) {
			printf("CTLRA ERROR: > MAX ITEMS in AVTKA dev\n");
			return 0;
		}
		dev->id_to_ctlra[idx].type = CTLRA_EVENT_BUTTON;
		dev->id_to_ctlra[idx].id   = i;
	}

	for(int i = 0; i < info->control_count[CTLRA_EVENT_SLIDER]; i++) {
		struct ctlra_item_info_t *item =
			&info->control_info[CTLRA_EVENT_SLIDER][i];
		if(!item)
			break;

		const char *name = ctlra_info_get_name(info,
					CTLRA_EVENT_SLIDER, i);

		struct avtka_item_opts_t ai = {
			.x = item->x,
			.y = item->y,
			.w = item->w,
			.h = item->h,
		};
		ctlra_item_scale(&ai);
		ai.draw = (item->flags & CTLRA_ITEM_FADER) ?
			  AVTKA_DRAW_SLIDER :  AVTKA_DRAW_DIAL;
		ai.interact = item->h > (item->w - 2) ?
			AVTKA_INTERACT_DRAG_V : AVTKA_INTERACT_DRAG_H ;

		snprintf(ai.name, sizeof(ai.name), "%s", name);
		uint32_t idx = avtka_item_create(a, &ai);
		if(idx > MAX_ITEMS) {
			printf("CTLRA ERROR: > MAX ITEMS in AVTKA dev\n");
			return 0;
		}
		dev->id_to_ctlra[idx].type = CTLRA_EVENT_SLIDER;
		dev->id_to_ctlra[idx].id   = i;
	}

	for(int i = 0; i < info->control_count[CTLRA_EVENT_ENCODER]; i++) {
		struct ctlra_item_info_t *item =
			&info->control_info[CTLRA_EVENT_ENCODER][i];
		if(!item)
			break;
		const char *name = ctlra_info_get_name(info,
					CTLRA_EVENT_ENCODER, i);

		struct avtka_item_opts_t ai = {
			.x = item->x,
			.y = item->y,
			.w = item->w,
			.h = item->h,
		};
		ctlra_item_scale(&ai);
		ai.draw = (item->flags & CTLRA_ITEM_CENTER_NOTCH) ?
			  AVTKA_DRAW_DIAL :  AVTKA_DRAW_JOG_WHEEL;
		ai.interact = item->h > (item->w - 2) ?
			AVTKA_INTERACT_DRAG_DELTA_V :
			AVTKA_INTERACT_DRAG_DELTA_H ;

		snprintf(ai.name, sizeof(ai.name), "%s", name);
		uint32_t idx = avtka_item_create(a, &ai);
		printf("encoder %d = %s\n", idx, name);
		if(idx > MAX_ITEMS) {
			printf("CTLRA ERROR: > MAX ITEMS in AVTKA dev\n");
			return 0;
		}
		dev->id_to_ctlra[idx].type = CTLRA_EVENT_ENCODER;
		dev->id_to_ctlra[idx].encoder_float_delta =
			!(item->flags & CTLRA_ITEM_CENTER_NOTCH);
		dev->id_to_ctlra[idx].id   = i;
	}

	for(int g = 0; g < info->control_count[CTLRA_EVENT_GRID]; g++) {
		struct ctlra_grid_info_t *gi = &info->grid_info[g];
		if(!gi)
			break;
		uint32_t size_w = (gi->info.w / gi->x);
		uint32_t size_h = (gi->info.h / gi->y);
		/* iterate over each pad */
		for(int i = 0; i < (gi->x * gi->y); i++) {
			/* draw big square for grid */
			struct avtka_item_opts_t ai = {
				 //.name = name,
				.x = gi->info.x + ((i % gi->x) * (size_w+2)),
				.y = gi->info.y + ((i / gi->y) * (size_h+2)),
				.w = size_w - 2,
				.h = size_h - 2,
				.draw = AVTKA_DRAW_BUTTON,
				.interact = AVTKA_INTERACT_CLICK,
			};
			ctlra_item_scale(&ai);
			printf("grid %d: %d %d, %d %d\n", i,
			       ai.x, ai.y, ai.w, ai.h);
			uint32_t idx = avtka_item_create(a, &ai);
			if(idx > MAX_ITEMS) {
				printf("CTLRA ERROR: > MAX ITEMS in AVTKA dev\n");
				return 0;
			}
			dev->id_to_ctlra[idx].type = CTLRA_EVENT_GRID;
			dev->id_to_ctlra[idx].id   = i;
		}
	}

	printf("%s %d; avtka_t %p\n", __func__, __LINE__, a);

	/* return the pointer to the new UI instance */
	return a;
}

