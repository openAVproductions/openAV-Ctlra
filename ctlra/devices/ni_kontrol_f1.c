/*
 * Copyright (c) 2016, OpenAV Productions,
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

#include "ni_kontrol_f1.h"
#include "impl.h"

#define CTLRA_DRIVER_VENDOR (0x17cc)
#define CTLRA_DRIVER_DEVICE (0x1120)
#define USB_INTERFACE_ID   (0x00)
#define USB_HANDLE_IDX     (0x0)
#define USB_ENDPOINT_READ  (0x81)
#define USB_ENDPOINT_WRITE (0x01)

/* This struct is a generic struct to identify hw controls */
struct ni_kontrol_f1_ctlra_t {
	int event_id;
	int buf_byte_offset;
	uint32_t mask;
};

static const char *ni_kontrol_f1_names_sliders[] = {
	"Filter 1",
	"Filter 2",
	"Filter 3",
	"Filter 4",
	"Fader 1",
	"Fader 2",
	"Fader 3",
	"Fader 4",
};
#define CONTROL_NAMES_SLIDERS_SIZE (sizeof(ni_kontrol_f1_names_sliders) /\
				    sizeof(ni_kontrol_f1_names_sliders[0]))

static const char *ni_kontrol_f1_names_buttons[] = {
	"Shift",
	"Reverse",
	"Type",
	"Size",
	"Browse",
	"Enc. Press",
	"Stop 1",
	"Stop 2",
	"Stop 3",
	"Stop 4",
	"Sync",
	"Quantize",
	"Capture",
};
#define CONTROL_NAMES_BUTTONS_SIZE (sizeof(ni_kontrol_f1_names_buttons) /\
				    sizeof(ni_kontrol_f1_names_buttons[0]))
#define CONTROL_NAMES_SIZE (CONTROL_NAMES_SLIDERS_SIZE + \
			    CONTROL_NAMES_BUTTONS_SIZE)

static const struct ni_kontrol_f1_ctlra_t sliders[] = {
	/* Left */
	{NI_KONTROL_F1_SLIDER_FILTER_1,  6, UINT32_MAX},
	{NI_KONTROL_F1_SLIDER_FILTER_2,  8, UINT32_MAX},
	{NI_KONTROL_F1_SLIDER_FILTER_3, 10, UINT32_MAX},
	{NI_KONTROL_F1_SLIDER_FILTER_4, 12, UINT32_MAX},
	{NI_KONTROL_F1_SLIDER_FADER_1 , 14, UINT32_MAX},
	{NI_KONTROL_F1_SLIDER_FADER_2 , 16, UINT32_MAX},
	{NI_KONTROL_F1_SLIDER_FADER_3 , 18, UINT32_MAX},
	{NI_KONTROL_F1_SLIDER_FADER_4 , 20, UINT32_MAX},
};
#define SLIDERS_SIZE (sizeof(sliders) / sizeof(sliders[0]))

#define DIAL_CENTER (CTLRA_ITEM_DIAL | CTLRA_ITEM_CENTER_NOTCH)
static struct ctlra_item_info_t sliders_info[] = {
	/* Filter dials up top */
	{.x =  8, .y = 22, .w = 22, .h = 22, .flags = DIAL_CENTER},
	{.x = 35, .y = 22, .w = 22, .h = 22, .flags = DIAL_CENTER},
	{.x = 62, .y = 22, .w = 22, .h = 22, .flags = DIAL_CENTER},
	{.x = 90, .y = 22, .w = 22, .h = 22, .flags = DIAL_CENTER},
	/* sliders */
	{.x =  8, .y = 56, .w = 22, .h = 56, .flags = CTLRA_ITEM_FADER},
	{.x = 35, .y = 56, .w = 22, .h = 56, .flags = CTLRA_ITEM_FADER},
	{.x = 62, .y = 56, .w = 22, .h = 56, .flags = CTLRA_ITEM_FADER},
	{.x = 90, .y = 56, .w = 22, .h = 56, .flags = CTLRA_ITEM_FADER},
};

static const struct ni_kontrol_f1_ctlra_t buttons[] = {
	{NI_KONTROL_F1_BTN_SHIFT        , 3, 0x80},
	{NI_KONTROL_F1_BTN_REVERSE      , 3, 0x40},
	{NI_KONTROL_F1_BTN_TYPE         , 3, 0x20},
	{NI_KONTROL_F1_BTN_SIZE         , 3, 0x10},
	{NI_KONTROL_F1_BTN_BROWSE       , 3, 0x08},
	{NI_KONTROL_F1_BTN_ENCODER_PRESS, 3, 0x04},
	{NI_KONTROL_F1_BTN_STOP_1       , 4, 0x80},
	{NI_KONTROL_F1_BTN_STOP_2       , 4, 0x40},
	{NI_KONTROL_F1_BTN_STOP_3       , 4, 0x20},
	{NI_KONTROL_F1_BTN_STOP_4       , 4, 0x10},
	{NI_KONTROL_F1_BTN_SYNC         , 4, 0x08},
	{NI_KONTROL_F1_BTN_QUANT        , 4, 0x04},
	{NI_KONTROL_F1_BTN_CAPTURE      , 4, 0x02},
};
#define BUTTONS_SIZE (sizeof(buttons) / sizeof(buttons[0]))

#define F1_BTN (CTLRA_ITEM_BUTTON | CTLRA_ITEM_LED_INTENSITY | CTLRA_ITEM_HAS_FB_ID)
static struct ctlra_item_info_t buttons_info[] = {
	/* shift */
	{.x =   8, .y = 147, .w = 16,  .h = 8, .flags = F1_BTN, .colour = 0xff000000, .fb_id = 19},
	/* reverse, type, size */
	{.x =  30, .y = 147, .w = 16,  .h = 8, .flags = F1_BTN, .colour = 0xff000000, .fb_id = 18},
	{.x =  52, .y = 147, .w = 16,  .h = 8, .flags = F1_BTN, .colour = 0xff000000, .fb_id = 17},
	{.x =  74, .y = 147, .w = 16,  .h = 8, .flags = F1_BTN, .colour = 0xff000000, .fb_id = 16},
	/* browse */
	{.x =  96, .y = 147, .w = 16,  .h = 8, .flags = F1_BTN, .colour = 0x000000ff, .fb_id = 15},
	/* enc press */
	{.x =  96, .y = 126, .w = 16,  .h = 16, .flags = CTLRA_ITEM_BUTTON},
	/* stop left -> right */
	{.x =  8, .y = 260, .w = 22,  .h = 6, .flags = F1_BTN, .colour = 0xff000000, .fb_id = 42},
	{.x = 35, .y = 260, .w = 22,  .h = 6, .flags = F1_BTN, .colour = 0xff000000, .fb_id = 41},
	{.x = 62, .y = 260, .w = 22,  .h = 6, .flags = F1_BTN, .colour = 0xff000000, .fb_id = 40},
	{.x = 90, .y = 260, .w = 22,  .h = 6, .flags = F1_BTN, .colour = 0xff000000, .fb_id = 39},
	/* sync, quant, capture */
	{.x =   8, .y = 126, .w = 16,  .h = 8, .flags = F1_BTN, .colour = 0xff000000, .fb_id = 22},
	{.x =  30, .y = 126, .w = 16,  .h = 8, .flags = F1_BTN, .colour = 0xff000000, .fb_id = 21},
	{.x =  52, .y = 126, .w = 16,  .h = 8, .flags = F1_BTN, .colour = 0xff000000, .fb_id = 20},
};

#define GRID_SIZE 16

#define CONTROLS_SIZE (SLIDERS_SIZE + BUTTONS_SIZE + GRID_SIZE)

/* Represents the the hardware device */
struct ni_kontrol_f1_t {
	/* base handles usb i/o etc */
	struct ctlra_dev_t base;
	/* current value of each controller is stored here */
	float hw_values[CONTROLS_SIZE];
	/* current state of the lights, only flush on dirty */
	uint8_t lights_dirty;
	uint8_t encoder;

	/* LED SIZE is the number of bytes to the device */
#define LED_SIZE 79
	uint8_t lights_interface;
	uint8_t lights[LED_SIZE];
};

static const char *
ni_kontrol_f1_control_get_name(enum ctlra_event_type_t type,
			       uint32_t control)
{
	switch(type) {
	case CTLRA_EVENT_SLIDER:
		if(control >= CONTROL_NAMES_SLIDERS_SIZE)
			return 0;
		return ni_kontrol_f1_names_sliders[control];
	case CTLRA_EVENT_BUTTON:
		if(control >= CONTROL_NAMES_BUTTONS_SIZE)
			return 0;
		return ni_kontrol_f1_names_buttons[control];
	case CTLRA_EVENT_ENCODER:
		if(control == 0)
			return "Encoder";
	default:
		break;
	}
	return 0;
}

static uint32_t ni_kontrol_f1_poll(struct ctlra_dev_t *base)
{
	struct ni_kontrol_f1_t *dev = (struct ni_kontrol_f1_t *)base;
#define BUF_SIZE 1024
	uint8_t buf[BUF_SIZE];

	ctlra_dev_impl_usb_interrupt_read(base, USB_HANDLE_IDX,
					  USB_ENDPOINT_READ, buf, BUF_SIZE);
	return 0;
}


void ni_kontrol_f1_usb_read_cb(struct ctlra_dev_t *base, uint32_t endpoint,
				uint8_t *data, uint32_t size)
{
	struct ni_kontrol_f1_t *dev = (struct ni_kontrol_f1_t *)base;
	uint8_t *buf = data;

	switch(size) {
	case 22: {
		for(uint32_t i = 0; i < SLIDERS_SIZE; i++) {
			int id     = sliders[i].event_id;
			int offset = sliders[i].buf_byte_offset;
			int mask   = sliders[i].mask;

			uint16_t v = *((uint16_t *)&buf[offset]) & mask;
			if(dev->hw_values[i] != v) {
				dev->hw_values[i] = v;
				struct ctlra_event_t event = {
					.type = CTLRA_EVENT_SLIDER,
					.slider  = {
						.id = id,
						.value = v / 4096.f},
				};
				struct ctlra_event_t *e = {&event};
				dev->base.event_func(&dev->base, 1, &e,
						     dev->base.event_func_userdata);
			}
		}

		/* encoder: uses 0xff bits, result is same with just 0xf,
		 * so simplify the implementation to just 0xf */
		int enc_new = buf[5] & 0xf;
		if(dev->encoder != enc_new) {
			int dir = ctlra_dev_encoder_wrap_16(enc_new,
							    dev->encoder);
			dev->encoder = enc_new;

			struct ctlra_event_t event = {
				.type = CTLRA_EVENT_ENCODER,
				.encoder  = {
					.id = 0,
				},
			};
			event.encoder.delta = dir;
			struct ctlra_event_t *e = {&event};
			dev->base.event_func(&dev->base, 1, &e,
					     dev->base.event_func_userdata);
		}

		/* Grid */
		static uint8_t grid_masks[] = {
			0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01,
			0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01
		};
		for(uint32_t i = 0; i < GRID_SIZE; i++) {
			int offset = 1 + i / 8;
			int mask   = grid_masks[i];

			uint16_t v = *((uint16_t *)&buf[offset]) & mask;
			int value_idx = SLIDERS_SIZE + BUTTONS_SIZE + i;
			if(dev->hw_values[value_idx] != v) {
				dev->hw_values[value_idx] = v;
				struct ctlra_event_t event = {
					.type = CTLRA_EVENT_GRID,
					.grid  = {
						.id = 0,
						.flags = CTLRA_EVENT_GRID_FLAG_BUTTON,
						.pos = i,
						.pressed = v > 0,
					},
				};
				struct ctlra_event_t *e = {&event};
				dev->base.event_func(&dev->base, 1, &e,
						     dev->base.event_func_userdata);
			}
		}

		for(uint32_t i = 0; i < BUTTONS_SIZE; i++) {
			int id     = buttons[i].event_id;
			int offset = buttons[i].buf_byte_offset;
			int mask   = buttons[i].mask;

			uint16_t v = *((uint16_t *)&buf[offset]) & mask;
			int value_idx = SLIDERS_SIZE + i;

			if(dev->hw_values[value_idx] != v) {
				dev->hw_values[value_idx] = v;

				struct ctlra_event_t event = {
					.type = CTLRA_EVENT_BUTTON,
					.button  = {
						.id = id,
						.pressed = v > 0},
				};
				struct ctlra_event_t *e = {&event};
				dev->base.event_func(&dev->base, 1, &e,
						     dev->base.event_func_userdata);
			}
		}
		break;
		}
	}
}

static void ni_kontrol_f1_light_set(struct ctlra_dev_t *base,
				    uint32_t light_id,
				    uint32_t light_status)
{
	struct ni_kontrol_f1_t *dev = (struct ni_kontrol_f1_t *)base;
	int ret;

	/* zero-th byte seems useless, so skip it :) */
	light_id += 1;

#define NUM_LED_IN (23 + 16 + 4)
	if(!dev || light_id > NUM_LED_IN)
		return;

	/* Lights:
	 * 0 ..15: digit displays, 8 is * in the top left.
	 * 16..23 Browse, Size, Type, Reverse, Shift, capture, quant, sync
	 * 24..71 BRG, BRG, BRG for all pads, top left -> top right ... 
	 * (72,73), (74,75) .. 79. RIGHT lowest buttons, in pairs
	 */
	/* brighness only, direct mapping */
	uint32_t bright = (light_status >> 24) & 0x7F;
	if(light_id < 24) {
		dev->lights[light_id] = bright;
		goto fin;
	};
	/* pad buttons, 24 .. 71 */
	if(light_id >= 24 && light_id < 40) {
		uint32_t r      = (light_status >> 16) & 0x7F;
		uint32_t g      = (light_status >>  8) & 0x7F;
		uint32_t b      = (light_status >>  0) & 0x7F;
		/* amend ID to skip over red/green bytes per pad */
		light_id += (light_id - 24) * 2;
		/* Pads are BRG ordered */
		dev->lights[light_id  ] = b;
		dev->lights[light_id+1] = r;
		dev->lights[light_id+2] = g;
		goto fin;
	}
	/* lowest buttons "double up" on bytes */
	if(light_id >= 40) {
		int idx = 72 + (light_id - 40) * 2;
		dev->lights[idx  ] = bright;
		dev->lights[idx+1] = bright;
	}

fin:
	dev->lights_dirty = 1;
}

void
ni_kontrol_f1_light_flush(struct ctlra_dev_t *base, uint32_t force)
{
	struct ni_kontrol_f1_t *dev = (struct ni_kontrol_f1_t *)base;
	if(!dev->lights_dirty && !force)
		return;

	dev->lights_interface = 0x80;
	uint8_t *data = &dev->lights_interface;

	dev->lights[0] = 0x80;
	int ret = ctlra_dev_impl_usb_interrupt_write(base, USB_HANDLE_IDX,
						     USB_ENDPOINT_WRITE,
						     data, 81);
	if(ret < 0) {
		//base->usb_xfer_counts[USB_XFER_ERROR]++;
	}
}

static int32_t
ni_kontrol_f1_disconnect(struct ctlra_dev_t *base)
{
	struct ni_kontrol_f1_t *dev = (struct ni_kontrol_f1_t *)base;

	/* Turn off all lights */
	memset(&dev->lights[1], 0, sizeof(dev->lights));
	dev->lights[0] = 0x80;
	if(!base->banished)
		ni_kontrol_f1_light_flush(base, 1);

	ctlra_dev_impl_usb_close(base);
	free(dev);
	return 0;
}

struct ctlra_dev_info_t ctlra_ni_kontrol_f1_info;

struct ctlra_dev_t *
ctlra_ni_kontrol_f1_connect(ctlra_event_func event_func, void *userdata,
			    void *future)
{
	(void)future;
	struct ni_kontrol_f1_t *dev = calloc(1, sizeof(struct ni_kontrol_f1_t));
	if(!dev)
		goto fail;

	int err = ctlra_dev_impl_usb_open(&dev->base,
					  CTLRA_DRIVER_VENDOR,
					  CTLRA_DRIVER_DEVICE);
	if(err) {
		free(dev);
		return 0;
	}

	err = ctlra_dev_impl_usb_open_interface(&dev->base, USB_INTERFACE_ID, USB_HANDLE_IDX);
	if(err) {
		free(dev);
		return 0;
	}

	dev->base.info = ctlra_ni_kontrol_f1_info;

	dev->base.poll = ni_kontrol_f1_poll;
	dev->base.disconnect = ni_kontrol_f1_disconnect;
	dev->base.light_set = ni_kontrol_f1_light_set;
	dev->base.light_flush = ni_kontrol_f1_light_flush;
	dev->base.usb_read_cb = ni_kontrol_f1_usb_read_cb;

	dev->base.event_func = event_func;
	dev->base.event_func_userdata = userdata;

	return (struct ctlra_dev_t *)dev;
fail:
	free(dev);
	return 0;
}

struct ctlra_dev_info_t ctlra_ni_kontrol_f1_info = {
	.vendor    = "Native Instruments",
	.device    = "Kontrol F1",
	.vendor_id = CTLRA_DRIVER_VENDOR,
	.device_id = CTLRA_DRIVER_DEVICE,
	.size_x    = 120,
	.size_y    = 294,

	/* TODO: expose info */
	.control_count[CTLRA_EVENT_BUTTON] = BUTTONS_SIZE,
	.control_info[CTLRA_EVENT_BUTTON] = buttons_info,
	.control_count[CTLRA_EVENT_SLIDER] = SLIDERS_SIZE,
	.control_info[CTLRA_EVENT_SLIDER] = sliders_info,
#if 0
	.control_count[CTLRA_FEEDBACK_ITEM] = FEEDBACK_SIZE,
	.control_info[CTLRA_FEEDBACK_ITEM] = feedback_info,
#endif

	.control_count[CTLRA_EVENT_GRID] = 1,
	.grid_info[0] = {
		.rgb = 1,
		.velocity = 1,
		.pressure = 1,
		.x = 4,
		.y = 4,
		.info = {
			.x =   8,
			.y = 166,
			.w = 100,
			.h =  86,
			/* start light id */
			.params[0] = 23,
			/* end light id */
			.params[1] = 38,
		}
	},

	.get_name = ni_kontrol_f1_control_get_name,
};

CTLRA_DEVICE_REGISTER(ni_kontrol_f1)
