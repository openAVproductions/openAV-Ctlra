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

#define NI_VENDOR          (0x17cc)
#define NI_KONTROL_X1_MK2  (0x1220)
#define USB_INTERFACE_ID   (0x0)
#define USB_HANDLE_IDX     (0x0)
#define USB_ENDPOINT_READ  (0x81)
#define USB_ENDPOINT_WRITE (0x01)

/* This struct is a generic struct to identify hw controls */
struct ni_kontrol_x1_mk2_ctlra_t {
	int event_id;
	int buf_byte_offset;
	uint32_t mask;
};

static const char *ni_kontrol_x1_mk2_control_names[] = {
	/* Faders / Dials */
	"FX 1 Knob (Left)",
	"FX 2 Knob (Left)",
	"FX 3 Knob (Left)",
	"FX 4 Knob (Left)",
	"FX 1 Knob (Right)",
	"FX 2 Knob (Right)",
	"FX 3 Knob (Right)",
	"FX 4 Knob (Right)",
	/* Encoders */
	"Encoder Rotate (Middle)",
	"Encoder Rotate (Left)",
	"Encoder Rotate (Right)",
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
	"Encoder Right Press",
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
	"Touchstrip",
};
#define CONTROL_NAMES_SIZE (sizeof(ni_kontrol_x1_mk2_control_names) /\
			    sizeof(ni_kontrol_x1_mk2_control_names[0]))

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

static const struct ni_kontrol_x1_mk2_ctlra_t encoders[] = {
	{NI_KONTROL_X1_MK2_BTN_ENCODER_MID_ROTATE  , 29, 0x10},
};
#define ENCODERS_SIZE (sizeof(encoders) / sizeof(encoders[0]))

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

	/* current state of the lights, only flush on dirty */
	uint8_t lights_dirty;

	uint8_t lights_interface;
	uint8_t lights[LIGHTS_SIZE];
};

static const char *
ni_kontrol_x1_mk2_control_get_name(const struct ctlra_dev_t *base,
			       enum ctlra_event_type_t type,
			       uint32_t control_id)
{
	struct ni_kontrol_x1_mk2_t *dev = (struct ni_kontrol_x1_mk2_t *)base;
	if(control_id < CONTROL_NAMES_SIZE)
		return ni_kontrol_x1_mk2_control_names[control_id];
	return 0;
}

static uint32_t
ni_kontrol_x1_mk2_poll(struct ctlra_dev_t *base)
{
	struct ni_kontrol_x1_mk2_t *dev = (struct ni_kontrol_x1_mk2_t *)base;
	uint8_t buf[1024];
	int32_t nbytes;

	int handle_idx = 0;
	nbytes = ctlra_dev_impl_usb_interrupt_read(base, USB_HANDLE_IDX,
						   USB_ENDPOINT_READ,
						   buf, 1024);
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
		dev->base.event_func(&dev->base, 1, &te2,
				     dev->base.event_func_userdata);

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
	int ret = ctlra_dev_impl_usb_interrupt_write(base, USB_HANDLE_IDX,
						     USB_ENDPOINT_WRITE,
						     data, LIGHTS_SIZE+1);
	if(ret < 0)
		printf("%s write failed!\n", __func__);
}

static int32_t
ni_kontrol_x1_mk2_disconnect(struct ctlra_dev_t *base)
{
	struct ni_kontrol_x1_mk2_t *dev = (struct ni_kontrol_x1_mk2_t *)base;

	/* Turn off all lights */
	memset(&dev->lights[1], 0, NI_KONTROL_X1_MK2_LED_COUNT);
	dev->lights[0] = 0x80;
	if(!base->banished)
		ni_kontrol_x1_mk2_light_flush(base, 1);

	ctlra_dev_impl_usb_close(base);
	free(dev);
	dev = 0x0;
	return 0;
}

struct ctlra_dev_t *ni_kontrol_x1_mk2_connect(ctlra_event_func event_func,
				  void *userdata, void *future)
{
	(void)future;
	struct ni_kontrol_x1_mk2_t *dev = calloc(1, sizeof(struct ni_kontrol_x1_mk2_t));
	if(!dev)
		return 0;

	snprintf(dev->base.info.vendor, sizeof(dev->base.info.vendor),
		 "%s", "Native Instruments");
	snprintf(dev->base.info.device, sizeof(dev->base.info.device),
		 "%s", "Kontrol X2 Mk2");

	int err = ctlra_dev_impl_usb_open(&dev->base, NI_VENDOR, NI_KONTROL_X1_MK2);
	if(err) {
		free(dev);
		return 0;
	}

	err = ctlra_dev_impl_usb_open_interface(&dev->base, USB_INTERFACE_ID, USB_HANDLE_IDX);
	if(err) {
		free(dev);
		return 0;
	}

	dev->base.poll = ni_kontrol_x1_mk2_poll;
	dev->base.disconnect = ni_kontrol_x1_mk2_disconnect;
	dev->base.light_set = ni_kontrol_x1_mk2_light_set;
	dev->base.control_get_name = ni_kontrol_x1_mk2_control_get_name;
	dev->base.light_flush = ni_kontrol_x1_mk2_light_flush;
	dev->base.usb_read_cb = ni_kontrol_x1_mk2_usb_read_cb;

	dev->base.event_func = event_func;
	dev->base.event_func_userdata = userdata;

	return (struct ctlra_dev_t *)dev;
}

