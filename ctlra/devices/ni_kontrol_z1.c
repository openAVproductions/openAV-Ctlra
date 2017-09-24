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

#include "ni_kontrol_z1.h"
#include "impl.h"

#define NI_VENDOR          (0x17cc)
#define NI_KONTROL_Z1      (0x1210)
#define USB_HANDLE_IDX     (0x0)
#define USB_INTERFACE_ID   (0x03)
#define USB_ENDPOINT_READ  (0x82)
#define USB_ENDPOINT_WRITE (0x02)

/* This struct is a generic struct to identify hw controls */
struct ni_kontrol_z1_ctlra_t {
	int event_id;
	int buf_byte_offset;
	uint32_t mask;
};

static const char *ni_kontrol_z1_names_sliders[] = {
	"Gain (L)",
	"Eq High (L)",
	"Eq Mid (L)",
	"Eq Low (L)",
	"Filter (L)",
	"Gain (R)",
	"Eq High (R)",
	"Eq Mid (R)",
	"Eq Low (R)",
	"Filter (R)",
	"Cue Mix",
	"Fader (L)",
	"Fader (R)",
	"Crossfader",
};
#define CONTROL_NAMES_SLIDERS_SIZE (sizeof(ni_kontrol_z1_names_sliders) /\
				    sizeof(ni_kontrol_z1_names_sliders[0]))

#define Z1_DIAL (CTLRA_ITEM_DIAL)
#define Z1_DIAL_CENTER (CTLRA_ITEM_DIAL | CTLRA_ITEM_CENTER_NOTCH)
static struct ctlra_item_info_t sliders_info[] = {
	/* left top gain, hi, mid, low, filter */
	{.x = 14, .y = 24, .w = 15,  .h = 15, .flags = Z1_DIAL},
	{.x = 12, .y = 50, .w = 18,  .h = 18, .flags = Z1_DIAL_CENTER},
	{.x = 12, .y = 72, .w = 18,  .h = 18, .flags = Z1_DIAL_CENTER},
	{.x = 12, .y =103, .w = 18,  .h = 18, .flags = Z1_DIAL_CENTER},
	{.x = 10, .y =132, .w = 22,  .h = 22, .flags = Z1_DIAL_CENTER},
	/* right top gain, hi, mid, low, filter */
	{.x = 92, .y = 24, .w = 15,  .h = 15, .flags = Z1_DIAL},
	{.x = 90, .y = 50, .w = 18,  .h = 18, .flags = Z1_DIAL_CENTER},
	{.x = 90, .y = 72, .w = 18,  .h = 18, .flags = Z1_DIAL_CENTER},
	{.x = 90, .y =103, .w = 18,  .h = 18, .flags = Z1_DIAL_CENTER},
	{.x = 88, .y =132, .w = 22,  .h = 22, .flags = Z1_DIAL_CENTER},
	/* cue */
	{.x = 52, .y = 92, .w = 15,  .h = 15, .flags = Z1_DIAL_CENTER},
	/* fader left, right */
	{.x = 10, .y =185, .w = 24,  .h = 56, .flags = CTLRA_ITEM_FADER},
	{.x = 88, .y =185, .w = 24,  .h = 56, .flags = CTLRA_ITEM_FADER},
	/* crossfader */
	{.x = 33, .y =248, .w = 56,  .h = 22, .flags = CTLRA_ITEM_FADER},
};

static const char *ni_kontrol_z1_names_buttons[] = {
	"A",
	"B",
	"Mode",
	"On (L)",
	"On (R)",
};
#define CONTROL_NAMES_BUTTONS_SIZE (sizeof(ni_kontrol_z1_names_buttons) /\
				    sizeof(ni_kontrol_z1_names_buttons[0]))
#define CONTROL_NAMES_SIZE (CONTROL_NAMES_SLIDERS_SIZE + \
			    CONTROL_NAMES_BUTTONS_SIZE)

#define Z1_BTN (CTLRA_ITEM_BUTTON | CTLRA_ITEM_LED_INTENSITY | CTLRA_ITEM_HAS_FB_ID)
#define Z1_BTN_COL (Z1_BTN | CTLRA_ITEM_LED_COLOR)
static struct ctlra_item_info_t buttons_info[] = {
	{.x = 44, .y = 120, .w = 8,  .h = 8, .flags = Z1_BTN, .fb_id = NI_KONTROL_Z1_LED_CUE_A},
	{.x = 68, .y = 120, .w = 8,  .h = 8, .flags = Z1_BTN, .fb_id = NI_KONTROL_Z1_LED_CUE_B},
	{.x = 53, .y = 165, .w = 18, .h = 8, .flags = Z1_BTN, .fb_id = NI_KONTROL_Z1_LED_MODE},
	{.x = 13, .y = 165, .w = 18, .h = 8, .flags = Z1_BTN_COL, .fb_id = NI_KONTROL_Z1_LED_FX_ON_LEFT},
	{.x = 90, .y = 165, .w = 18, .h = 8, .flags = Z1_BTN_COL, .fb_id = NI_KONTROL_Z1_LED_FX_ON_RIGHT},
};

static const struct ni_kontrol_z1_ctlra_t sliders[] = {
	/* Left */
	{NI_KONTROL_Z1_SLIDER_LEFT_GAIN    ,  1, UINT32_MAX},
	{NI_KONTROL_Z1_SLIDER_LEFT_EQ_HIGH ,  3, UINT32_MAX},
	{NI_KONTROL_Z1_SLIDER_LEFT_EQ_MID  ,  5, UINT32_MAX},
	{NI_KONTROL_Z1_SLIDER_LEFT_EQ_LOW  ,  7, UINT32_MAX},
	{NI_KONTROL_Z1_SLIDER_LEFT_FILTER  ,  9, UINT32_MAX},
	{NI_KONTROL_Z1_SLIDER_RIGHT_GAIN   , 11, UINT32_MAX},
	{NI_KONTROL_Z1_SLIDER_RIGHT_EQ_HIGH, 13, UINT32_MAX},
	{NI_KONTROL_Z1_SLIDER_RIGHT_EQ_MID , 15, UINT32_MAX},
	{NI_KONTROL_Z1_SLIDER_RIGHT_EQ_LOW , 17, UINT32_MAX},
	{NI_KONTROL_Z1_SLIDER_RIGHT_FILTER , 19, UINT32_MAX},
	{NI_KONTROL_Z1_SLIDER_CUE_MIX      , 21, UINT32_MAX},
	{NI_KONTROL_Z1_SLIDER_LEFT_FADER   , 23, UINT32_MAX},
	{NI_KONTROL_Z1_SLIDER_RIGHT_FADER  , 25, UINT32_MAX},
	{NI_KONTROL_Z1_SLIDER_CROSS_FADER  , 27, UINT32_MAX},
};
#define SLIDERS_SIZE (sizeof(sliders) / sizeof(sliders[0]))

static const struct ni_kontrol_z1_ctlra_t buttons[] = {
	{NI_KONTROL_Z1_BTN_CUE_A  , 29, 0x10},
	{NI_KONTROL_Z1_BTN_CUE_B  , 29, 0x1},
	{NI_KONTROL_Z1_BTN_MODE   , 29, 0x2},
	{NI_KONTROL_Z1_BTN_FX_ON_L, 29, 0x4},
	{NI_KONTROL_Z1_BTN_FX_ON_R, 29, 0x8},
};
#define BUTTONS_SIZE (sizeof(buttons) / sizeof(buttons[0]))

#define CONTROLS_SIZE (SLIDERS_SIZE + BUTTONS_SIZE)

/* feedback items */
static struct ctlra_item_info_t feedback_info[] = {
	/* level meter A */
	{	.x = 50, .y = 185, .w = 3, .h = 56, .flags = CTLRA_ITEM_FB_LED_STRIP,
		.params = {0, 7, 5, 0},},
	/* level meter B */
	{	.x = 70, .y = 185, .w = 3, .h = 56, .flags = CTLRA_ITEM_FB_LED_STRIP,
		.params = {7,14, 5, 0},},
};
#define FEEDBACK_SIZE (sizeof(feedback_info) / sizeof(feedback_info[0]))

static const char *ni_kontrol_z1_names_feedback[] = {
	"Meter A",
	"Meter B",
};
#define CONTROL_NAMES_FEEDBACK_SIZE (sizeof(ni_kontrol_z1_names_feedback) /\
				    sizeof(ni_kontrol_z1_names_feedback[0]))


/* Represents the the hardware device */
struct ni_kontrol_z1_t {
	/* base handles usb i/o etc */
	struct ctlra_dev_t base;
	/* current value of each controller is stored here */
	float hw_values[CONTROLS_SIZE];
	/* current state of the lights, only flush on dirty */
	uint8_t lights_dirty;

	uint8_t lights_interface;
	uint8_t lights[NI_KONTROL_Z1_LED_COUNT];
};

static const char *
ni_kontrol_z1_control_get_name(enum ctlra_event_type_t type,
			       uint32_t control)
{
	switch(type) {
	case CTLRA_EVENT_SLIDER:
		if(control >= CONTROL_NAMES_SLIDERS_SIZE)
			return 0;
		return ni_kontrol_z1_names_sliders[control];
	case CTLRA_EVENT_BUTTON:
		if(control >= CONTROL_NAMES_BUTTONS_SIZE)
			return 0;
		return ni_kontrol_z1_names_buttons[control];
	case CTLRA_FEEDBACK_ITEM:
		if(control >= CONTROL_NAMES_FEEDBACK_SIZE)
			return 0;
		return ni_kontrol_z1_names_feedback[control];
	default:
		break;
	}

	return 0;
}

static uint32_t
ni_kontrol_z1_poll(struct ctlra_dev_t *base)
{
	struct ni_kontrol_z1_t *dev = (struct ni_kontrol_z1_t *)base;
#define BUF_SIZE 1024
	uint8_t buf[BUF_SIZE];

	ctlra_dev_impl_usb_interrupt_read(base, USB_HANDLE_IDX,
					  USB_ENDPOINT_READ,
					  buf, BUF_SIZE);
	return 0;
}

static void ni_kontrol_z1_light_set(struct ctlra_dev_t *base,
				    uint32_t light_id,
				    uint32_t light_status);

static void ni_kontrol_z1_light_flush(struct ctlra_dev_t *base,
				      uint32_t force);

void ni_kontrol_z1_usb_read_cb(struct ctlra_dev_t *base, uint32_t endpoint,
				uint8_t *data, uint32_t size)
{
	struct ni_kontrol_z1_t *dev = (struct ni_kontrol_z1_t *)base;
	uint8_t *buf = data;
	switch(size) {
	case 30: {
		for(uint32_t i = 0; i < SLIDERS_SIZE; i++) {
			int id     = i;
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
		for(uint32_t i = 0; i < BUTTONS_SIZE; i++) {
			int id     = i;
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
	ni_kontrol_z1_light_flush(&dev->base, 1);
}

static inline void
ni_kontrol_z1_light_set(struct ctlra_dev_t *base, uint32_t light_id,
			uint32_t light_status)
{
	struct ni_kontrol_z1_t *dev = (struct ni_kontrol_z1_t *)base;
	int ret;

	if(!dev || light_id >= NI_KONTROL_Z1_LED_COUNT ||
	   light_id == NI_KONTROL_Z1_LED_FX_ON_LEFT +1 ||
	   light_id == NI_KONTROL_Z1_LED_FX_ON_RIGHT+1)
		return;

	/* write brighness to all LEDs */
	uint32_t bright = (light_status >> 24) & 0x7F;
	dev->lights[light_id] = bright;

	/* FX ON buttons have orange and blue */
	if(light_id == NI_KONTROL_Z1_LED_FX_ON_LEFT ||
	   light_id == NI_KONTROL_Z1_LED_FX_ON_RIGHT) {
		uint32_t r      = (light_status >> 16) & 0xFF;
		uint32_t b      = (light_status >>  0) & 0xFF;
		dev->lights[light_id  ] = r;
		dev->lights[light_id+1] = b;
	}

	dev->lights_dirty = 1;
}

static void
ni_kontrol_z1_feedback_set(struct ctlra_dev_t *base, uint32_t fb_id,
			   float value)
{
	if(fb_id > 1)
		return;

	int id = (fb_id == 1);
	for(int i = 0; i < 7; i++) {
		ni_kontrol_z1_light_set(base, id * 7 + i,
					i < (value * 7) ?
					0xff000000 : 0x0);
	}
}

void
ni_kontrol_z1_light_flush(struct ctlra_dev_t *base, uint32_t force)
{
	struct ni_kontrol_z1_t *dev = (struct ni_kontrol_z1_t *)base;
	if(!dev->lights_dirty && !force)
		return;
	/* technically interface 3, testing showed 0 works but 3 doesnt */
	uint8_t *data = &dev->lights_interface;
	dev->lights_interface = 0x80;

	int ret = ctlra_dev_impl_usb_interrupt_write(base, USB_HANDLE_IDX,
						     USB_ENDPOINT_WRITE,
						     data,
						     NI_KONTROL_Z1_LED_COUNT+1);
	if(ret < 0) {
		//printf("%s write failed!\n", __func__);
	}
}

static int32_t
ni_kontrol_z1_disconnect(struct ctlra_dev_t *base)
{
	struct ni_kontrol_z1_t *dev = (struct ni_kontrol_z1_t *)base;

	/* Turn off all lights */
	memset(dev->lights, 0, NI_KONTROL_Z1_LED_COUNT);
	if(!base->banished)
		ni_kontrol_z1_light_flush(base, 1);

	ctlra_dev_impl_usb_close(base);
	free(dev);
	return 0;
}

struct ctlra_dev_info_t ctlra_ni_kontrol_z1_info;

struct ctlra_dev_t *
ctlra_ni_kontrol_z1_connect(ctlra_event_func event_func,
				  void *userdata, void *future)
{
	(void)future;
	struct ni_kontrol_z1_t *dev = calloc(1, sizeof(struct ni_kontrol_z1_t));
	if(!dev)
		goto fail;

#if 0
	snprintf(dev->base.info.vendor, sizeof(dev->base.info.vendor),
		 "%s", "Native Instruments");
	snprintf(dev->base.info.device, sizeof(dev->base.info.device),
		 "%s", "Kontrol Z1");

	dev->base.info.control_count[CTLRA_EVENT_BUTTON] = BUTTONS_SIZE;
	dev->base.info.control_count[CTLRA_EVENT_SLIDER] = SLIDERS_SIZE;
	dev->base.info.control_info[CTLRA_EVENT_BUTTON] = &buttons_info,
	dev->base.info.control_info[CTLRA_EVENT_SLIDER] = &sliders_info,
	dev->base.info.get_name = ni_kontrol_z1_control_get_name;
#else
	dev->base.info = ctlra_ni_kontrol_z1_info;
#endif

	int err = ctlra_dev_impl_usb_open(&dev->base,
					 NI_VENDOR, NI_KONTROL_Z1);
	if(err) {
		free(dev);
		return 0;
	}

	err = ctlra_dev_impl_usb_open_interface(&dev->base,
					 USB_INTERFACE_ID, USB_HANDLE_IDX);
	if(err) {
		free(dev);
		return 0;
	}

	dev->base.poll = ni_kontrol_z1_poll;
	dev->base.disconnect = ni_kontrol_z1_disconnect;
	dev->base.light_set = ni_kontrol_z1_light_set;
	dev->base.feedback_set = ni_kontrol_z1_feedback_set;
	dev->base.light_flush = ni_kontrol_z1_light_flush;
	dev->base.usb_read_cb = ni_kontrol_z1_usb_read_cb;

	dev->base.event_func = event_func;
	dev->base.event_func_userdata = userdata;

	return (struct ctlra_dev_t *)dev;
fail:
	free(dev);
	return 0;
}


struct ctlra_dev_info_t ctlra_ni_kontrol_z1_info = {
	.vendor    = "Native Instruments",
	.device    = "Kontrol Z1",
	.vendor_id = NI_VENDOR,
	.device_id = NI_KONTROL_Z1,
	.size_x    = 120,
	.size_y    = 294,

	.control_count[CTLRA_EVENT_BUTTON] = BUTTONS_SIZE,
	.control_count[CTLRA_EVENT_SLIDER] = SLIDERS_SIZE,
	.control_count[CTLRA_FEEDBACK_ITEM] = FEEDBACK_SIZE,

	.control_info[CTLRA_EVENT_BUTTON] = buttons_info,
	.control_info[CTLRA_EVENT_SLIDER] = sliders_info,
	.control_info[CTLRA_FEEDBACK_ITEM] = feedback_info,

	.get_name = ni_kontrol_z1_control_get_name,
};

#define CTLRA_DRIVER_VENDOR NI_VENDOR
#define CTLRA_DRIVER_DEVICE NI_KONTROL_Z1
CTLRA_DEVICE_REGISTER(ni_kontrol_z1)
