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

#include <avtka.h>

#define PUGL_PARENT 0x0

#define MAX_ITEMS 1024

/* reverse map from item id to ctlra type/id */
struct id_to_ctlra_t {
	uint16_t type;
	uint16_t encoder_float_delta : 1;
	uint32_t id;
	uint32_t fb_id;
	uint32_t col;
};

struct avtka_screent_t {
	uint32_t h_px;
	uint32_t w_px;

	/* TODO: Should this be the value of the Ctlra #define for screen
	 * bytes-per-pixel, or "data format" type? Need to figure out how
	 * we want to represent the data type of a screen through the API,
	 * or what to do there. */
	uint32_t data_type;
	/* TODO: how to represent bits/bytes per pixel, based on format?
	 */
	uint8_t bits_per_pixel;
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

	/* ctlra id offsets for each event type */
	uint32_t type_to_item_offset[CTLRA_EVENT_T_COUNT];

	/* screen info */
	struct avtka_screent_t screen[CTLRA_NUM_SCREENS_MAX];
};

uint32_t
avtka_poll(struct ctlra_dev_t *base)
{
	struct cavtka_t *dev = (struct cavtka_t *)base;
	avtka_iterate(dev->a);
	/* events can be "sent" to the app from the widget callbacks */
	return 0;
}

static void
avtka_light_set(struct ctlra_dev_t *base, uint32_t light_id,
		uint32_t light_status)
{
	struct cavtka_t *dev = (struct cavtka_t *)base;
	/* TODO: figure out how to display feedback */
	struct avtka_t *a = dev->a;

	uint32_t in = (light_status >> 24);

	for(int i = 0; i < MAX_ITEMS; i++) {
		if(dev->id_to_ctlra[i].fb_id == light_id) {
			uint32_t mask = dev->id_to_ctlra[i].col;
			uint32_t bw = in | (in << 8) | (in << 16);
			uint32_t final_col = bw & mask;
			/* support RGB leds individual channels */
			if((mask & 0x00ffffff) == 0xffffff) {
				final_col = 0x00ffffff & light_status;
			}
			avtka_item_colour32(a, i, final_col);
			break;
		}
	}
	avtka_redraw(a);
}

void
avtka_feedback_digits(struct ctlra_dev_t *base, uint32_t feedback_id,
		      float value)
{
}

void
avtka_light_flush(struct ctlra_dev_t *base, uint32_t force)
{
	struct cavtka_t *dev = (struct cavtka_t *)base;
}

int32_t
avtka_disconnect(struct ctlra_dev_t *base)
{
	struct cavtka_t *dev = (struct cavtka_t *)base;
	avtka_destroy(dev->a);
	free(dev);
	return 0;
}

static void
event_cb(struct avtka_t *avtka, uint32_t item, float value, void *userdata)
{
	struct cavtka_t *dev = (struct cavtka_t *)userdata;
	uint32_t type = dev->id_to_ctlra[item].type;
	uint32_t id   = dev->id_to_ctlra[item].id;
	uint32_t col  = dev->id_to_ctlra[item].col;

	printf("%s : event\n", __func__);

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
		event.grid.flags |= CTLRA_EVENT_GRID_FLAG_BUTTON;
		break;
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

void
avtka_mirror_hw_cb(struct ctlra_dev_t* base, uint32_t num_events,
		   struct ctlra_event_t** events, void *userdata)
{
	struct cavtka_t *dev = (struct cavtka_t *)base;
	struct avtka_t *a = dev->a;

	/* loop over each event, calculate item id, and set value based
	 * on type of the event */
	for(uint32_t i = 0; i < num_events; i++) {
		struct ctlra_event_t *e = events[i];
		uint32_t id;
		switch(e->type) {
		case CTLRA_EVENT_BUTTON:
			id = e->button.id + 1;
			avtka_item_value(a, id, e->button.pressed);
			avtka_item_label_show(a, id, e->button.pressed);
			break;
		case CTLRA_EVENT_SLIDER:
			id = dev->type_to_item_offset[CTLRA_EVENT_SLIDER] +
				e->slider.id;
			avtka_item_value(a, id + 1, e->slider.value);
			if(e->slider.id < 2) {
				int vu = dev->type_to_item_offset[CTLRA_FEEDBACK_ITEM] +
					e->slider.id + 1;
				avtka_item_value(a, vu, e->slider.value);
			}
			break;
		case CTLRA_EVENT_ENCODER: {
			id = dev->type_to_item_offset[CTLRA_EVENT_ENCODER] +
				e->encoder.id;
			avtka_item_value_inc(a, id + 1, e->encoder.delta_float);
			} break;
		case CTLRA_EVENT_GRID:
			id = dev->type_to_item_offset[CTLRA_EVENT_GRID] +
				e->grid.pos;
			avtka_item_colour32(a, id + 1, 0xffffffff * e->grid.pressed);
			break;
		case CTLRA_FEEDBACK_ITEM:
		default: break;
		}
	}
	avtka_redraw(a);
}


int32_t
avtka_screen_get_data(struct ctlra_dev_t *base, uint8_t **pixels,
		      uint32_t *bytes, uint8_t flush)
{
	struct cavtka_t *dev = (struct cavtka_t *)base;
	struct avtka_t *a = dev->a;

	if(!pixels || !bytes)
		return -1;

	/* TODO update Ctlra API to return screen data for ID */
	uint8_t screen = 0;
	if(screen >= CTLRA_NUM_SCREENS_MAX)
		return -2;

	struct avtka_screent_t *scr = &dev->screen[screen];
	*pixels = avtka_screen_get_data_ptr(a, screen);
	/* TODO: fix hard coded size */
	*bytes = ((scr->h_px * scr->w_px) * scr->bits_per_pixel) / 8;
	//*bytes = (128 * 64 / 8);

	return 0;
}


#define CTLRA_RESIZE 2
static inline void
ctlra_item_scale(struct avtka_item_opts_t *i)
{
	const float s = CTLRA_RESIZE;
	i->x *= s;
	i->y *= s;
	i->w *= s;
	i->h *= s;
}

struct avtka_t *
ctlra_build_avtka_ui(struct cavtka_t *dev,
		     const struct ctlra_dev_info_t *info);

struct ctlra_dev_t *
ctlra_avtka_connect(ctlra_event_func event_func, void *userdata,
		    const void *future)
{
	struct cavtka_t *dev = calloc(1, sizeof(struct cavtka_t));
	if(!dev)
		goto fail;

	dev->canary = 0xcafe;

	const struct ctlra_dev_info_t *info = future;

	/* reuse the existing info from the device backend, then update */
	dev->base.info = *info;

	snprintf(dev->base.info.vendor, sizeof(dev->base.info.vendor),
		 "%s", info->vendor);
	snprintf(dev->base.info.device, sizeof(dev->base.info.device),
		 "%s", info->device);

	dev->base.poll = avtka_poll;
	dev->base.disconnect = avtka_disconnect;
	dev->base.light_set = avtka_light_set;
	dev->base.screen_get_data = avtka_screen_get_data;

	dev->base.event_func = event_func;
	dev->base.event_func_userdata = userdata;

	/* pass in back-pointer to ctlra_dev_t class for sending events */
	dev->a = ctlra_build_avtka_ui(dev, info);
	if(!dev->a)
		goto fail;

	CTLRA_INFO(dev->base.ctlra_context, "avtka based '%s' '%s' created\n",
		   dev->base.info.vendor, dev->base.info.device);

	return (struct ctlra_dev_t *)dev;
fail:
	printf("%s, failed to create dev, ret 0\n", __func__);
	free(dev);
	return 0;
}

static inline void
ctlra_avtka_string_strip(char *s)
{
	/* replace ( with \0 to terminate before details */
	for (int i=0; s[i] != '\0'; i++) {
		if (s[i] == '(')
			s[i] = '\0';
	}
}

struct avtka_t *
ctlra_build_avtka_ui(struct cavtka_t *dev,
		     const struct ctlra_dev_info_t *info)
{
	/* initialize the Avtka UI */
	struct avtka_opts_t opts = {
		.w = info->size_x * CTLRA_RESIZE,
		.h = info->size_y * CTLRA_RESIZE,
		.event_callback = event_cb,
		.event_callback_userdata = dev,
	};
	char name[64];
	snprintf(name, sizeof(name), "Ctlra Virtual: %s - %s",
		 dev->base.info.vendor, dev->base.info.device);
	struct avtka_t *a = avtka_create(name, &opts);
	if(!a)
		return 0;

	uint8_t col_red   = avtka_register_colour(a, 1,  0, 0, 1);
	uint8_t col_green = avtka_register_colour(a, 0,  1, 0, 1);
	uint8_t col_blue  = avtka_register_colour(a, 0, .3, 1, 1);

	/* i scope magic - it is used after the loop to set the offset */
	int i = 0;
	dev->type_to_item_offset[CTLRA_EVENT_BUTTON] = i;
	for(i = 0; i < info->control_count[CTLRA_EVENT_BUTTON]; i++) {
		struct ctlra_item_info_t *item =
			&info->control_info[CTLRA_EVENT_BUTTON][i];
		if(!item)
			break;
		const char *name = ctlra_info_get_name(info,
					CTLRA_EVENT_BUTTON, i);
		struct avtka_item_opts_t ai = {
			.x = item->x,
			.y = item->y,
			.w = item->w,
			.h = item->h,
			.draw = AVTKA_DRAW_BUTTON,
			.interact = AVTKA_INTERACT_CLICK,
		};
		ctlra_item_scale(&ai);
		snprintf(ai.name, sizeof(ai.name), "%s", name);
		ctlra_avtka_string_strip(ai.name);

		uint32_t idx = avtka_item_create(a, &ai);
		if(idx > MAX_ITEMS) {
			printf("CTLRA ERROR: > MAX ITEMS in AVTKA dev\n");
			return 0;
		}

		dev->id_to_ctlra[idx].type = CTLRA_EVENT_BUTTON;
		dev->id_to_ctlra[idx].id   = i;
		dev->id_to_ctlra[idx].fb_id = item->fb_id;
		dev->id_to_ctlra[idx].col = item->colour;
		/* turn off at startup */
		avtka_item_colour32(a, idx, 0);
	}

	dev->type_to_item_offset[CTLRA_EVENT_SLIDER] = i;
	for(i = 0; i < info->control_count[CTLRA_EVENT_SLIDER]; i++) {
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
		ctlra_avtka_string_strip(ai.name);
		uint32_t idx = avtka_item_create(a, &ai);
		if(idx > MAX_ITEMS) {
			printf("CTLRA ERROR: > MAX ITEMS in AVTKA dev\n");
			return 0;
		}
		dev->id_to_ctlra[idx].type = CTLRA_EVENT_SLIDER;
		dev->id_to_ctlra[idx].id   = i;
	}

	/* FIXME: current offset-method is hacky - required adding on the
	 * offset of previous item type manually - refactor */
	dev->type_to_item_offset[CTLRA_EVENT_ENCODER] =
		i + dev->type_to_item_offset[CTLRA_EVENT_SLIDER];
	for(i = 0; i < info->control_count[CTLRA_EVENT_ENCODER]; i++) {
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
		ctlra_avtka_string_strip(ai.name);
		uint32_t idx = avtka_item_create(a, &ai);
		if(idx > MAX_ITEMS) {
			printf("CTLRA ERROR: > MAX ITEMS in AVTKA dev\n");
			return 0;
		}
		dev->id_to_ctlra[idx].type = CTLRA_EVENT_ENCODER;
		dev->id_to_ctlra[idx].encoder_float_delta =
			!(item->flags & CTLRA_ITEM_CENTER_NOTCH);
		dev->id_to_ctlra[idx].id   = i;
	}

	dev->type_to_item_offset[CTLRA_EVENT_GRID] =
		i + dev->type_to_item_offset[CTLRA_EVENT_ENCODER];
	for(int g = 0; g < info->control_count[CTLRA_EVENT_GRID]; g++) {
		const struct ctlra_grid_info_t *gi = &info->grid_info[g];
		if(!gi)
			break;
		uint32_t size_w = (gi->info.w / gi->x);
		uint32_t size_h = (gi->info.h / gi->y);
		/* iterate over each pad */
		for(i = 0; i < (gi->x * gi->y); i++) {
			/* draw big square for grid */
			struct avtka_item_opts_t ai = {
				.x = gi->info.x + ((i % gi->x) * (size_w+2)),
				.y = gi->info.y + ((i / gi->y) * (size_h+2)),
				.w = size_w - 2,
				.h = size_h - 2,
				.draw = AVTKA_DRAW_BUTTON,
				.interact = AVTKA_INTERACT_CLICK,
			};
			ctlra_item_scale(&ai);
			//printf("grid %d: %d %d, %d %d\n", i, ai.x, ai.y, ai.w, ai.h);
			uint32_t idx = avtka_item_create(a, &ai);
			if(idx > MAX_ITEMS) {
				printf("CTLRA ERROR: > MAX ITEMS in AVTKA dev\n");
				return 0;
			}
			dev->id_to_ctlra[idx].type = CTLRA_EVENT_GRID;
			dev->id_to_ctlra[idx].id   = i;

			//printf("grid item %d: param[0] = %d, [1] = %d\n", i, gi->info.params[0], gi->info.params[1]);
			dev->id_to_ctlra[idx].fb_id = (i + gi->info.params[0]);
		}
	}

	dev->type_to_item_offset[CTLRA_FEEDBACK_ITEM] =
		dev->type_to_item_offset[CTLRA_EVENT_GRID] + i;
	for(i = 0; i < info->control_count[CTLRA_FEEDBACK_ITEM]; i++) {
		struct ctlra_item_info_t *item =
			&info->control_info[CTLRA_FEEDBACK_ITEM][i];
		if(!item)
			break;
		const char *name = ctlra_info_get_name(info,
					CTLRA_FEEDBACK_ITEM, i);

		struct avtka_item_opts_t ai = {
			.x = item->x,
			.y = item->y,
			.w = item->w,
			.h = item->h,
		};
		ctlra_item_scale(&ai);

		/* default draws a button if it is not understood */
		ai.draw = AVTKA_DRAW_BUTTON;

		/* LED strip */
		if(item->flags & CTLRA_ITEM_FB_LED_STRIP) {
			ai.draw = AVTKA_DRAW_LED_STRIP;
			/* get the number of segments from the LED ids */
			ai.params[0] = item->params[1] - item->params[0];
			ai.params[1] = item->params[2];
		}

		ai.interact = 0;

		/* Screen */
		int no_create_item = 0;
		if(item->flags & CTLRA_ITEM_FB_SCREEN) {
			struct avtka_screen_opts_t opts = {
				.x = ai.x,
				.y = ai.y,
				.w = ai.w,
				.h = ai.h,
				/* TODO: detect screen caps */
				.px_x = 128,
				.px_y = 64,
				.flags_1bit = 1,
			};
			int32_t screen_id = avtka_screen_create(a, &opts);
			if(screen_id < 0)
				CTLRA_ERROR(dev->base.ctlra_context,
					    "Failed to create screen; ret %d\n",
					    screen_id);

			/* don't create an item for the screen, as it is
			 * already handled by screen_create() */
			continue;
		}

		if(item->flags & CTLRA_ITEM_FB_7_SEGMENT) {
			ai.draw = AVTKA_DRAW_7_SEG;
			ai.params[0] = item->params[0];
		}

		ai.interact = 0;

		//snprintf(ai.name, sizeof(ai.name), "%s", name);
		ctlra_avtka_string_strip(ai.name);
		uint32_t idx = avtka_item_create(a, &ai);
		if(idx > MAX_ITEMS) {
			printf("CTLRA ERROR: > MAX ITEMS in AVTKA dev\n");
			return 0;
		}
		dev->id_to_ctlra[idx].type = CTLRA_FEEDBACK_ITEM;
		dev->id_to_ctlra[idx].id   = i;
	}

	/* return the pointer to the new UI instance */
	return a;
}

