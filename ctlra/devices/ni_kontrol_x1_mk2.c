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

#include "ni_kontrol_x1_mk2.h"
#include "impl.h"

#define CTLRA_DRIVER_VENDOR (0x17cc)
#define CTLRA_DRIVER_DEVICE (0x1220)
#define USB_INTERFACE_ID   (0x0)
#define USB_HANDLE_IDX     (0x0)
#define USB_ENDPOINT_READ  (0x81)
#define USB_ENDPOINT_WRITE (0x01)

#define IFACE_Ox81 (1)
#define LEDS_P_DIG (8)
#define NUM_DIGITS (6)
#define NUM_DIGIT_LEDS (LEDS_P_DIG * NUM_DIGITS)
#define TOUCHSTRIP (42)
#define IFACE_Ox81_TOTAL (IFACE_Ox81 + NUM_DIGIT_LEDS + TOUCHSTRIP)

/* This struct is a generic struct to identify hw controls */
struct ni_kontrol_x1_mk2_ctlra_t {
	int event_id;
	int buf_byte_offset;
	uint32_t mask;
};

static const char *ni_kontrol_x1_mk2_slider_names[] = {
	"FX 1 Knob (Left)",
	"FX 2 Knob (Left)",
	"FX 3 Knob (Left)",
	"FX 4 Knob (Left)",
	"FX 1 Knob (Right)",
	"FX 2 Knob (Right)",
	"FX 3 Knob (Right)",
	"FX 4 Knob (Right)",
	"Touchstrip (Position)",
};
#define SLIDER_SIZE (sizeof(ni_kontrol_x1_mk2_slider_names) /\
			    sizeof(ni_kontrol_x1_mk2_slider_names[0]))

#define DIAL_CENTER (CTLRA_ITEM_DIAL | CTLRA_ITEM_CENTER_NOTCH)
static struct ctlra_item_info_t sliders_info[] = {
	/* left */
	{.x = 32, .y =  22, .w = 16,  .h = 16, .flags = DIAL_CENTER},
	{.x = 32, .y =  49, .w = 16,  .h = 16, .flags = DIAL_CENTER},
	{.x = 32, .y =  76, .w = 16,  .h = 16, .flags = DIAL_CENTER},
	{.x = 32, .y = 104, .w = 16,  .h = 16, .flags = DIAL_CENTER},
	/* right */
	{.x = 69, .y =  22, .w = 16,  .h = 16, .flags = DIAL_CENTER},
	{.x = 69, .y =  49, .w = 16,  .h = 16, .flags = DIAL_CENTER},
	{.x = 69, .y =  76, .w = 16,  .h = 16, .flags = DIAL_CENTER},
	{.x = 69, .y = 104, .w = 16,  .h = 16, .flags = DIAL_CENTER},
	/* touchstrip */
	{.x =  8, .y = 184, .w = 104,  .h = 10, .flags = CTLRA_ITEM_FADER},
};

static const char *ni_kontrol_x1_mk2_encoder_names[] = {
	/* Encoders */
	"Encoder Rotate (Middle)",
	"Encoder Rotate (Left)",
	"Encoder Rotate (Right)",
};
#define ENCODER_SIZE (sizeof(ni_kontrol_x1_mk2_encoder_names) /\
			    sizeof(ni_kontrol_x1_mk2_encoder_names[0]))

static const char *ni_kontrol_x1_mk2_button_names[] = {
	/* Buttons */
	"FX 1 Button (Left)",
	"FX 2 Button (Left)",
	"FX 3 Button (Left)",
	"FX 4 Button (Left)",
	"FX 1 Button (Right)",
	"FX 2 Button (Right)",
	"FX 3 Button (Right)",
	"FX 4 Button (Right)",
	"FX Select 1 (Left)",
	"FX Select 2 (Left)",
	"FX Select 1 (Right)",
	"FX Select 2 (Right)",
	"Left Arrow",
	"Shift",
	"Right Arrow",
	"Encoder Press (Right)",
	"Right Hotcue 1",
	"Right Hotcue 2",
	"Right Hotcue 3",
	"Right Hotcue 4",
	"Right Flux",
	"Right Sync",
	"Right Cue",
	"Right Play",
	"Left Hotcue 1",
	"Left Hotcue 2",
	"Left Hotcue 3",
	"Left Hotcue 4",
	"Left Flux",
	"Left Sync",
	"Left Cue",
	"Left Play",
	"Encoder Touch (Right)",
	"Encoder Touch (Middle)",
	"Encoder Touch (Left)",
	"Encoder Press (Middle)",
	"Encoder Press (Left)",
};
#define BUTTON_SIZE (sizeof(ni_kontrol_x1_mk2_button_names) /\
			     sizeof(ni_kontrol_x1_mk2_button_names[0]))

#define BTN (CTLRA_ITEM_BUTTON | CTLRA_ITEM_LED_INTENSITY | CTLRA_ITEM_HAS_FB_ID)
#define BTN_COL (BTN | CTLRA_ITEM_LED_COLOR)
#define DEF_COL .colour = 0xff000000
#define ORG_COL .colour = 0xffff5100
static struct ctlra_item_info_t buttons_info[] = {
	/* Left F1 to F4 vertial */
	{.x =  8, .y =  28, .w = 16,  .h = 8, .flags = BTN, ORG_COL, .fb_id = 0},
	{.x =  8, .y =  55, .w = 16,  .h = 8, .flags = BTN, ORG_COL, .fb_id = 1},
	{.x =  8, .y =  82, .w = 16,  .h = 8, .flags = BTN, ORG_COL, .fb_id = 2},
	{.x =  8, .y = 108, .w = 16,  .h = 8, .flags = BTN, ORG_COL, .fb_id = 3},
	/* Right F1 to F2 vertical */
	{.x = 95, .y =  28, .w = 16,  .h = 8, .flags = BTN, ORG_COL, .fb_id = 4},
	{.x = 95, .y =  55, .w = 16,  .h = 8, .flags = BTN, ORG_COL, .fb_id = 5},
	{.x = 95, .y =  82, .w = 16,  .h = 8, .flags = BTN, ORG_COL, .fb_id = 6},
	{.x = 95, .y = 108, .w = 16,  .h = 8, .flags = BTN, ORG_COL, .fb_id = 7},
	/* fx select left to right */
	{.x =  11, .y = 129, .w = 8, .h = 8, .flags = BTN, ORG_COL, .fb_id = 8},
	{.x =  25, .y = 129, .w = 8, .h = 8, .flags = BTN, ORG_COL, .fb_id = 9},
	{.x =  88, .y = 129, .w = 8, .h = 8, .flags = BTN, ORG_COL, .fb_id = 14},
	{.x = 102, .y = 129, .w = 8, .h = 8, .flags = BTN, ORG_COL, .fb_id = 15},
	/* left, shift right */
	{.x = 37, .y = 163, .w = 8, .h = 8, .flags = BTN, .colour = 0xff0000ff, .fb_id = 16},
	{.x = 51, .y = 163, .w = 16, .h = 8, .flags = BTN,.colour = 0xff000000, .fb_id = 17},
	{.x = 74, .y = 163, .w = 8, .h = 8, .flags = BTN, .colour = 0xff0000ff, .fb_id = 18},
	/* TODO: encoder press right */
	{.x = 88, .y = 154, .w = 22, .h = 22, .flags = CTLRA_ITEM_BUTTON},
	/* right hotcues 1,2  3,4 */
	{.x = 71, .y = 206, .w = 16, .h = 8, .flags = BTN_COL, .fb_id = 21},
	{.x = 96, .y = 206, .w = 16, .h = 8, .flags = BTN_COL, .fb_id = 22},
	{.x = 71, .y = 223, .w = 16, .h = 8, .flags = BTN_COL, .fb_id = 25},
	{.x = 96, .y = 223, .w = 16, .h = 8, .flags = BTN_COL, .fb_id = 26},
	/* right flux, sync, cue play */
	{.x = 71, .y = 239, .w = 16, .h = 12, .flags = BTN, .colour = 0xff0000ff, .fb_id = 29},
	{.x = 96, .y = 239, .w = 16, .h = 12, .flags = BTN, .colour = 0xff0000ff, .fb_id = 30},
	{.x = 71, .y = 256, .w = 16, .h = 12, .flags = BTN, .colour = 0xff0000ff, .fb_id = 33},
	{.x = 96, .y = 256, .w = 16, .h = 12, .flags = BTN, .colour = 0xff00ff00, .fb_id = 34},
	/* left hotcues 1,2  3,4 */
	{.x =  8, .y = 206, .w = 16, .h = 8, .flags = BTN_COL, .fb_id = 19},
	{.x = 33, .y = 206, .w = 16, .h = 8, .flags = BTN_COL, .fb_id = 20},
	{.x =  8, .y = 223, .w = 16, .h = 8, .flags = BTN_COL, .fb_id = 23},
	{.x = 33, .y = 223, .w = 16, .h = 8, .flags = BTN_COL, .fb_id = 24},
	/* left flux, sync, cue play */
	{.x =  8, .y = 239, .w = 16, .h = 12, .flags = BTN, .colour = 0xff0000ff, .fb_id = 27},
	{.x = 33, .y = 239, .w = 16, .h = 12, .flags = BTN, .colour = 0xff0000ff, .fb_id = 28},
	{.x =  8, .y = 256, .w = 16, .h = 12, .flags = BTN, .colour = 0xff0000ff, .fb_id = 31},
	{.x = 33, .y = 256, .w = 16, .h = 12, .flags = BTN, .colour = 0xff00ff00, .fb_id = 32},
	/* enc touch, Right, Mid, Left */
	{.x = 88, .y = 154, .w = 22, .h = 22, .flags = CTLRA_ITEM_BUTTON},
	{.x = 47, .y = 133, .w = 22, .h = 22, .flags = CTLRA_ITEM_BUTTON},
	{.x = 10, .y = 154, .w = 22, .h = 22, .flags = CTLRA_ITEM_BUTTON},
	/* enc press middle, left */
	{.x = 47, .y = 133, .w = 22, .h = 22, .flags = CTLRA_ITEM_BUTTON},
	{.x = 10, .y = 154, .w = 22, .h = 22, .flags = CTLRA_ITEM_BUTTON},
};

#define CONTROL_NAMES_SIZE (SLIDER_SIZE_SIZE + \
			    ENCODER_SIZE +\
			    BUTTON_SIZE)

static const struct ni_kontrol_x1_mk2_ctlra_t sliders[] = {
	{NI_KONTROL_X1_MK2_SLIDER_LEFT_FX_1    ,  1, UINT32_MAX},
	{NI_KONTROL_X1_MK2_SLIDER_LEFT_FX_2    ,  3, UINT32_MAX},
	{NI_KONTROL_X1_MK2_SLIDER_LEFT_FX_3    ,  5, UINT32_MAX},
	{NI_KONTROL_X1_MK2_SLIDER_LEFT_FX_4    ,  7, UINT32_MAX},
	{NI_KONTROL_X1_MK2_SLIDER_RIGHT_FX_1   ,  9, UINT32_MAX},
	{NI_KONTROL_X1_MK2_SLIDER_RIGHT_FX_2   , 11, UINT32_MAX},
	{NI_KONTROL_X1_MK2_SLIDER_RIGHT_FX_3   , 13, UINT32_MAX},
	{NI_KONTROL_X1_MK2_SLIDER_RIGHT_FX_4   , 15, UINT32_MAX},
};
#define SLIDERS_SIZE (sizeof(sliders) / sizeof(sliders[0]))

static const struct ni_kontrol_x1_mk2_ctlra_t buttons[] = {
	/* Top left buttons */
	{NI_KONTROL_X1_MK2_BTN_LEFT_FX_1 , 19, 0x80},
	{NI_KONTROL_X1_MK2_BTN_LEFT_FX_2 , 19, 0x40},
	{NI_KONTROL_X1_MK2_BTN_LEFT_FX_3 , 19, 0x20},
	{NI_KONTROL_X1_MK2_BTN_LEFT_FX_4 , 19, 0x10},
	/* Top right buttons */
	{NI_KONTROL_X1_MK2_BTN_RIGHT_FX_1, 19, 0x08},
	{NI_KONTROL_X1_MK2_BTN_RIGHT_FX_2, 19, 0x04},
	{NI_KONTROL_X1_MK2_BTN_RIGHT_FX_3, 19, 0x02},
	{NI_KONTROL_X1_MK2_BTN_RIGHT_FX_4, 19, 0x01},
	/* Smaller square FX buttons in screen area */
	{NI_KONTROL_X1_MK2_BTN_LEFT_FX_SELECT1 , 20, 0x80},
	{NI_KONTROL_X1_MK2_BTN_LEFT_FX_SELECT2 , 20, 0x40},
	{NI_KONTROL_X1_MK2_BTN_RIGHT_FX_SELECT1, 20, 0x20},
	{NI_KONTROL_X1_MK2_BTN_RIGHT_FX_SELECT2, 20, 0x10},
	/* Arrow / Shift buttons between encoders */
	{NI_KONTROL_X1_MK2_BTN_LEFT_ARROW         , 20, 0x08},
	{NI_KONTROL_X1_MK2_BTN_SHIFT              , 20, 0x04},
	{NI_KONTROL_X1_MK2_BTN_RIGHT_ARROW        , 20, 0x02},
	{NI_KONTROL_X1_MK2_BTN_ENCODER_RIGHT_PRESS, 20, 0x01},
	/* Right lower btn controls */
	{NI_KONTROL_X1_MK2_BTN_RIGHT_1   , 21, 0x80},
	{NI_KONTROL_X1_MK2_BTN_RIGHT_2   , 21, 0x40},
	{NI_KONTROL_X1_MK2_BTN_RIGHT_3   , 21, 0x20},
	{NI_KONTROL_X1_MK2_BTN_RIGHT_4   , 21, 0x10},
	{NI_KONTROL_X1_MK2_BTN_RIGHT_FLUX, 21, 0x08},
	{NI_KONTROL_X1_MK2_BTN_RIGHT_SYNC, 21, 0x04},
	{NI_KONTROL_X1_MK2_BTN_RIGHT_CUE , 21, 0x02},
	{NI_KONTROL_X1_MK2_BTN_RIGHT_PLAY, 21, 0x01},
	/* Left lower btn controls */
	{NI_KONTROL_X1_MK2_BTN_LEFT_1   , 22, 0x80},
	{NI_KONTROL_X1_MK2_BTN_LEFT_2   , 22, 0x40},
	{NI_KONTROL_X1_MK2_BTN_LEFT_3   , 22, 0x20},
	{NI_KONTROL_X1_MK2_BTN_LEFT_4   , 22, 0x10},
	{NI_KONTROL_X1_MK2_BTN_LEFT_FLUX, 22, 0x08},
	{NI_KONTROL_X1_MK2_BTN_LEFT_SYNC, 22, 0x04},
	{NI_KONTROL_X1_MK2_BTN_LEFT_CUE , 22, 0x02},
	{NI_KONTROL_X1_MK2_BTN_LEFT_PLAY, 22, 0x01},
	/* Encoder movement and touch */
	{NI_KONTROL_X1_MK2_BTN_ENCODER_RIGHT_TOUCH, 23, 0x10},
	{NI_KONTROL_X1_MK2_BTN_ENCODER_MID_TOUCH  , 23, 0x08},
	{NI_KONTROL_X1_MK2_BTN_ENCODER_LEFT_TOUCH , 23, 0x04},
	{NI_KONTROL_X1_MK2_BTN_ENCODER_MID_PRESS  , 23, 0x02},
	{NI_KONTROL_X1_MK2_BTN_ENCODER_LEFT_PRESS , 23, 0x01},
};
#define BUTTONS_SIZE (sizeof(buttons) / sizeof(buttons[0]))

#define CONTROLS_SIZE (SLIDERS_SIZE + BUTTONS_SIZE)

/* Multi-colour lights take multiple bytes for colour, but only one
 * enum value - hence lights size is > led count */
#define LIGHTS_SIZE (NI_KONTROL_X1_MK2_LED_COUNT+16)

struct ni_kontrol_x1_mk2_t {
	/* base handles usb i/o etc */
	struct ctlra_dev_t base;
	/* current value of each controller is stored here */
	float hw_values[CONTROLS_SIZE];

	/* Encoders */
	uint8_t encoder_values[3];
	uint32_t touchstrip_value;

	/* current state of the lights, only flush on dirty */
	uint8_t lights_dirty;

	uint8_t lights_interface;
	uint8_t lights[LIGHTS_SIZE];
	uint8_t lights_81[IFACE_Ox81_TOTAL];
};

static const char *
ni_kontrol_x1_mk2_control_get_name(enum ctlra_event_type_t type,
				   uint32_t control)
{
	uint16_t num_of_type[CTLRA_EVENT_T_COUNT];
	num_of_type[CTLRA_EVENT_SLIDER]  = SLIDER_SIZE;
	num_of_type[CTLRA_EVENT_BUTTON]  = BUTTON_SIZE;
	num_of_type[CTLRA_EVENT_ENCODER] = ENCODER_SIZE;

	uint16_t n = num_of_type[type];
	if(control >= n)
		return 0;

	const char **name_by_type[CTLRA_EVENT_T_COUNT];
	name_by_type[CTLRA_EVENT_SLIDER]  = ni_kontrol_x1_mk2_slider_names;
	name_by_type[CTLRA_EVENT_BUTTON]  = ni_kontrol_x1_mk2_button_names;
	name_by_type[CTLRA_EVENT_ENCODER] = ni_kontrol_x1_mk2_encoder_names;

	return name_by_type[type][control];
}

static uint32_t
ni_kontrol_x1_mk2_poll(struct ctlra_dev_t *base)
{
	struct ni_kontrol_x1_mk2_t *dev = (struct ni_kontrol_x1_mk2_t *)base;
	uint8_t buf[1024];

	/* calls into usb_read_cb below if data is available */
	ctlra_dev_impl_usb_interrupt_read(base, USB_HANDLE_IDX,
					  USB_ENDPOINT_READ, buf, 1024);

	return 0;
}

void ni_kontrol_x1_mk2_usb_read_cb(struct ctlra_dev_t *base, uint32_t endpoint,
				uint8_t *data, uint32_t size)
{
	struct ni_kontrol_x1_mk2_t *dev = (struct ni_kontrol_x1_mk2_t *)base;
	uint8_t *buf = data;
	switch(size) {
	case 31: {
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

		/* Handle Encoders */
		struct ctlra_event_t event = {
			.type = CTLRA_EVENT_ENCODER,
			.encoder = {
				.flags = CTLRA_EVENT_ENCODER_FLAG_INT,
				.delta = 0,
			},
		};
		struct ctlra_event_t *e = {&event};
		int8_t enc[3];
		enc[0] = ((data[17] & 0x0f)     ) & 0xf;
		enc[1] = ((data[17] & 0xf0) >> 4) & 0xf;
		enc[2] =  (data[18] & 0x0f);
		for(int i = 0; i < 3; i++) {
			int8_t cur = dev->encoder_values[i];
			if(enc[i] != cur) {
				int dir = ctlra_dev_encoder_wrap_16(enc[i], cur);
				event.encoder.delta = dir;
				event.encoder.id =
					NI_KONTROL_X1_MK2_BTN_ENCODER_MID_ROTATE + i;
				dev->base.event_func(&dev->base, 1, &e,
						     dev->base.event_func_userdata);
				/* update cached value */
				dev->encoder_values[i] = enc[i];
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

		/* Handle touchstrip */
		uint16_t v = (buf[28] << 8) | buf[27];
		struct ctlra_event_t te = {
			.type = CTLRA_EVENT_SLIDER,
			.slider  = {
				.id = NI_KONTROL_X1_MK2_SLIDER_TOUCHSTRIP,
				.value = v / 1023.f},
		};
		struct ctlra_event_t *te2 = {&te};
		/* v == 0 informs not touched, but on the device tested it
		 * happens frequently while just slideing, so no event
		 * is sent when 0 is the value. */
		if(dev->touchstrip_value != v && v != 0) {
			dev->base.event_func(&dev->base, 1, &te2,
					     dev->base.event_func_userdata);
			dev->touchstrip_value = v;
		}

		break;
		}
	}
}

static void
ni_kontrol_x1_mk2_light_set(struct ctlra_dev_t *base,
			    uint32_t light_id,
			    uint32_t light_status)
{
	struct ni_kontrol_x1_mk2_t *dev = (struct ni_kontrol_x1_mk2_t *)base;
	int ret;

	if(!dev || light_id >= NI_KONTROL_X1_MK2_LED_COUNT)
		return;

	/* write brighness to all LEDs */
	uint32_t bright = (light_status >> 24) & 0x7F;

	uint32_t idx = light_id;

	/* Range of lights here support RGB */
	if(light_id >= NI_KONTROL_X1_MK2_LED_LEFT_HOTCUE_1 &&
	   light_id < NI_KONTROL_X1_MK2_LED_LEFT_FLUX) {
		uint32_t r      = (light_status >> 16) & 0xFF;
		uint32_t g      = (light_status >>  8) & 0xFF;
		uint32_t b      = (light_status >>  0) & 0xFF;

		int ngt = idx - NI_KONTROL_X1_MK2_LED_LEFT_HOTCUE_1;
		idx += ngt * 2;

		dev->lights[idx  ] = r;
		dev->lights[idx+1] = g;
		dev->lights[idx+2] = b;
	} else {
		uint32_t inc = (light_id >= NI_KONTROL_X1_MK2_LED_LEFT_FLUX) * 16;
		idx += inc;
		dev->lights[idx] = bright;
	}

	/* TODO: 7 digit displays, and touch strip leds to lights_81 */

	dev->lights_dirty = 1;
}

static void
ni_kontrol_x1_mk2_light_flush(struct ctlra_dev_t *base, uint32_t force)
{
	struct ni_kontrol_x1_mk2_t *dev = (struct ni_kontrol_x1_mk2_t *)base;
	if(!dev->lights_dirty && !force)
		return;

	uint8_t *data = &dev->lights_interface;
	dev->lights_interface = 0x80;
	const uint32_t size = LIGHTS_SIZE + 1;
	int ret = ctlra_dev_impl_usb_interrupt_write(base,
					   USB_HANDLE_IDX,
					   USB_ENDPOINT_WRITE,
					   data, size);

	dev->lights_81[0] = 0x81;
	ret = ctlra_dev_impl_usb_interrupt_write(base,
					   USB_HANDLE_IDX,
					   USB_ENDPOINT_WRITE,
					   dev->lights_81,
					   IFACE_Ox81_TOTAL);
}

void ni_kontrol_x1_mk2_feedback_digits(struct ctlra_dev_t *base,
				       uint32_t feedback_id,
				       float value)
{
	struct ni_kontrol_x1_mk2_t *dev = (struct ni_kontrol_x1_mk2_t *)base;
	static const uint8_t digits[] = {
		0b00111111, /* 0 */
		0b00110000,
		0b01011011,
		0b01111001,
		0b01110100,
		0b01101101, /* 5 */
		0b01101111,
		0b00111000,
		0b01111111,
		0b01111100,
	};

	int display_id;
	switch(feedback_id) {
	case 0: /* left */
		display_id = 0;
		break;
	case 1: /* right */
		display_id = 3 * 8;
		break;
	default:
		CTLRA_WARN(base->ctlra_context, "%s Invalid id", __func__);
		return;
	}

	int neg = 0;
	if(value < 0) {
		value = -value;
		neg = 1;
	}

	uint32_t v = value;
	int t = v;
	int32_t darray[3] = {0};
	for(int i = 0; i < 3; i++) {
		if (t != 0) {
			int r = t % 10;
			int s = s + r;
			t = t / 10;
			darray[2-i] = r;
		}
	}

	int d0_zero = 0;
	for(int d = 0; d < 3; d++) {
		int n = darray[d] % 10;
		uint8_t fin_dig = digits[n] |
			((neg && d == 0) ? 0b10000001 : 0);
		for(int i = 0; i < 8; i++) {
			int idx = (1 + display_id) + d * 8 + 7 - i;
			uint8_t lit = (1 << i) & fin_dig;
			/* poor mans "dont show pre-zeros" */
			if(d == 0 && n == 0) {
				lit = 0;
				d0_zero = 1;
			}
			if(d0_zero && d == 1 && n == 0) {
				lit = 0;
			}
			dev->lights_81[idx] = lit ? 0xff : 0x1;
		}
	}
}

static int32_t
ni_kontrol_x1_mk2_disconnect(struct ctlra_dev_t *base)
{
	struct ni_kontrol_x1_mk2_t *dev = (struct ni_kontrol_x1_mk2_t *)base;

	/* Turn off all lights */
	memset(dev->lights, 0, NI_KONTROL_X1_MK2_LED_COUNT);
	memset(dev->lights_81, 0, sizeof(dev->lights_81));
	if(!base->banished)
		ni_kontrol_x1_mk2_light_flush(base, 1);

	ctlra_dev_impl_usb_close(base);
	free(dev);
	dev = 0x0;
	return 0;
}

struct ctlra_dev_info_t ctlra_ni_kontrol_x1_mk2_info;

struct ctlra_dev_t *
ctlra_ni_kontrol_x1_mk2_connect(ctlra_event_func event_func,
				void *userdata, void *future)
{
	(void)future;
	struct ni_kontrol_x1_mk2_t *dev =
		calloc(1, sizeof(struct ni_kontrol_x1_mk2_t));
	if(!dev)
		return 0;

	int err = ctlra_dev_impl_usb_open(&dev->base, CTLRA_DRIVER_VENDOR,
					  CTLRA_DRIVER_DEVICE);
	if(err) {
		free(dev);
		return 0;
	}

	err = ctlra_dev_impl_usb_open_interface(&dev->base,
						USB_INTERFACE_ID,
						USB_HANDLE_IDX);
	if(err) {
		free(dev);
		return 0;
	}

	dev->base.info = ctlra_ni_kontrol_x1_mk2_info;

	dev->base.poll = ni_kontrol_x1_mk2_poll;
	dev->base.disconnect = ni_kontrol_x1_mk2_disconnect;
	dev->base.light_set = ni_kontrol_x1_mk2_light_set;
	dev->base.light_flush = ni_kontrol_x1_mk2_light_flush;
	dev->base.usb_read_cb = ni_kontrol_x1_mk2_usb_read_cb;
	dev->base.feedback_digits = ni_kontrol_x1_mk2_feedback_digits;

	dev->base.event_func = event_func;
	dev->base.event_func_userdata = userdata;

	const uint8_t leds_on = 0x1;
	int j;
	for(j = 1; j < (LEDS_P_DIG * NUM_DIGITS) + 1; j++)
		dev->lights_81[j] = leds_on;

	/* touchstrip */
	for(; j < IFACE_Ox81_TOTAL; j += 2) {
		dev->lights_81[j  ] = leds_on; /* orange */
		dev->lights_81[j+1] = 0x0; /* blue */
	}

	return (struct ctlra_dev_t *)dev;
}

struct ctlra_dev_info_t ctlra_ni_kontrol_x1_mk2_info = {
	.vendor    = "Native Instruments",
	.device    = "Kontrol X1 Mk2",
	.vendor_id = CTLRA_DRIVER_VENDOR,
	.device_id = CTLRA_DRIVER_DEVICE,
	.size_x    = 120,
	.size_y    = 294,

	.control_count[CTLRA_EVENT_SLIDER] = SLIDER_SIZE,
	.control_info[CTLRA_EVENT_SLIDER] = sliders_info,
	.control_count[CTLRA_EVENT_BUTTON] = BUTTON_SIZE,
	.control_info[CTLRA_EVENT_BUTTON] = buttons_info,
#if 0
	.control_count[CTLRA_EVENT_ENCODER] = ENCODER_SIZE,
	//.control_count[CTLRA_FEEDBACK_ITEM] = FEEDBACK_SIZE,
	.control_info[CTLRA_FEEDBACK_ITEM] = feedback_info,
#endif

	.get_name = ni_kontrol_x1_mk2_control_get_name,
};

CTLRA_DEVICE_REGISTER(ni_kontrol_x1_mk2)
