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
#include "../device_impl.h"

#define NI_VENDOR          (0x17cc)
#define NI_KONTROL_F1      (0x1120)
#define USB_INTERFACE_ID   (0x00)
#define USB_ENDPOINT_READ  (0x81)
#define USB_ENDPOINT_WRITE (0x01)

/* This struct is a generic struct to identify hw controls */
struct ni_kontrol_f1_ctlr_t {
	int event_id;
	int buf_byte_offset;
	uint32_t mask;
};

static const char *ni_kontrol_f1_control_names[] = {
	/* Faders / Dials */
	"Gain (Left)",
	"Eq High (Left)",
	"Eq Mid (Left)",
	"Eq Low (Left)",
	"Filter (Left)",
	"Gain (Right)",
	"Eq High (Right)",
	"Eq Mid (Right)",
	"Eq Low (Right)",
	"Filter (Right)",
	"Cue Mix",
	"Fader (Left)",
	"Fader (Right)",
	"Crossfader",
	/* Buttons */
	"Headphones Cue A",
	"Headphones Cue B",
	"Mode",
	"Filter On (Left)",
	"Filter On (Right)",
};
#define CONTROL_NAMES_SIZE (sizeof(ni_kontrol_f1_control_names) /\
			    sizeof(ni_kontrol_f1_control_names[0]))

static const struct ni_kontrol_f1_ctlr_t sliders[] = {
	/* Left */
	{NI_KONTROL_F1_SLIDER_LEFT_GAIN    ,  1, UINT32_MAX},
	{NI_KONTROL_F1_SLIDER_LEFT_EQ_HIGH ,  3, UINT32_MAX},
	{NI_KONTROL_F1_SLIDER_LEFT_EQ_MID  ,  5, UINT32_MAX},
	{NI_KONTROL_F1_SLIDER_LEFT_EQ_LOW  ,  7, UINT32_MAX},
	{NI_KONTROL_F1_SLIDER_LEFT_FILTER  ,  9, UINT32_MAX},
	{NI_KONTROL_F1_SLIDER_RIGHT_GAIN   , 11, UINT32_MAX},
	{NI_KONTROL_F1_SLIDER_RIGHT_EQ_HIGH, 13, UINT32_MAX},
	{NI_KONTROL_F1_SLIDER_RIGHT_EQ_MID , 15, UINT32_MAX},
	{NI_KONTROL_F1_SLIDER_RIGHT_EQ_LOW , 17, UINT32_MAX},
	{NI_KONTROL_F1_SLIDER_RIGHT_FILTER , 19, UINT32_MAX},
	{NI_KONTROL_F1_SLIDER_CUE_MIX      , 21, UINT32_MAX},
	{NI_KONTROL_F1_SLIDER_LEFT_FADER   , 23, UINT32_MAX},
	{NI_KONTROL_F1_SLIDER_RIGHT_FADER  , 25, UINT32_MAX},
	{NI_KONTROL_F1_SLIDER_CROSS_FADER  , 27, UINT32_MAX},
};
#define SLIDERS_SIZE (sizeof(sliders) / sizeof(sliders[0]))

static const struct ni_kontrol_f1_ctlr_t buttons[] = {
	{NI_KONTROL_F1_BTN_CUE_A  , 29, 0x10},
	{NI_KONTROL_F1_BTN_CUE_B  , 29, 0x1},
	{NI_KONTROL_F1_BTN_MODE   , 29, 0x2},
	{NI_KONTROL_F1_BTN_FX_ON_L, 29, 0x4},
	{NI_KONTROL_F1_BTN_FX_ON_R, 29, 0x8},
};
#define BUTTONS_SIZE (sizeof(buttons) / sizeof(buttons[0]))

#define CONTROLS_SIZE (SLIDERS_SIZE + BUTTONS_SIZE)

/* Represents the the hardware device */
struct ni_kontrol_f1_t {
	/* base handles usb i/o etc */
	struct ctlr_dev_t base;
	/* current value of each controller is stored here */
	float hw_values[CONTROLS_SIZE];
	/* current state of the lights, only flush on dirty */
	uint8_t lights_dirty;
	uint8_t lights[NI_KONTROL_F1_LED_COUNT];
};

static const char *
ni_kontrol_f1_control_get_name(struct ctlr_dev_t *base,
			       uint32_t control_id)
{
	struct ni_kontrol_f1_t *dev = (struct ni_kontrol_f1_t *)base;
	if(control_id < CONTROL_NAMES_SIZE)
		return ni_kontrol_f1_control_names[control_id];
	return 0;
}

static uint32_t ni_kontrol_f1_poll(struct ctlr_dev_t *base)
{
	struct ni_kontrol_f1_t *dev = (struct ni_kontrol_f1_t *)base;
	uint8_t buf[1024];
	int32_t nbytes;

	do {
		int handle_idx = 0;
		nbytes = ctlr_dev_impl_usb_xfer(base, handle_idx,
							 USB_ENDPOINT_READ,
							 buf, 1024);
		if(nbytes == 0)
			return 0;

		printf("read %d\n", nbytes);

		return 0;

		switch(nbytes) {
		case 30: {
			for(uint32_t i = 0; i < SLIDERS_SIZE; i++) {
				int id     = sliders[i].event_id;
				int offset = sliders[i].buf_byte_offset;
				int mask   = sliders[i].mask;

				uint16_t v = *((uint16_t *)&buf[offset]) & mask;
				if(dev->hw_values[i] != v) {
					dev->hw_values[i] = v;
					struct ctlr_event_t event = {
						.id = CTLR_EVENT_SLIDER,
						.slider  = {
							.id = id,
							.value = v / 4096.f},
					};
					struct ctlr_event_t *e = {&event};
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

					struct ctlr_event_t event = {
						.id = CTLR_EVENT_BUTTON,
						.button  = {
							.id = id,
							.pressed = v > 0},
					};
					struct ctlr_event_t *e = {&event};
					dev->base.event_func(&dev->base, 1, &e,
							     dev->base.event_func_userdata);
				}
			}
			break;
			}
		}
	} while (nbytes > 0);

	return 0;
}

static void ni_kontrol_f1_light_set(struct ctlr_dev_t *base,
				    uint32_t light_id,
				    uint32_t light_status)
{

	return;


	struct ni_kontrol_f1_t *dev = (struct ni_kontrol_f1_t *)base;
	int ret;

	if(!dev || light_id > NI_KONTROL_F1_LED_COUNT)
		return;

	/* write brighness to all LEDs */
	uint32_t bright = (light_status >> 24) & 0x7F;
	dev->lights[light_id] = bright;
	dev->lights[0] = 0x80;

	/* FX ON buttons have orange and blue */
	if(light_id == NI_KONTROL_F1_LED_FX_ON_LEFT ||
	   light_id == NI_KONTROL_F1_LED_FX_ON_RIGHT) {
		uint32_t r      = (light_status >> 16) & 0xFF;
		uint32_t b      = (light_status >>  0) & 0xFF;
		dev->lights[light_id  ] = r;
		dev->lights[light_id+1] = b;
	}

	dev->lights_dirty = 1;
}

void
ni_kontrol_f1_light_flush(struct ctlr_dev_t *base, uint32_t force)
{
	struct ni_kontrol_f1_t *dev = (struct ni_kontrol_f1_t *)base;
	if(!dev->lights_dirty && !force)
		return;

	int ret = ctlr_dev_impl_usb_xfer(base,
					 USB_INTERFACE_ID,
					 USB_ENDPOINT_WRITE,
					 dev->lights,
					 NI_KONTROL_F1_LED_COUNT);
	if(ret < 0)
		printf("%s write failed!\n", __func__);
}

static int32_t
ni_kontrol_f1_disconnect(struct ctlr_dev_t *base)
{
	struct ni_kontrol_f1_t *dev = (struct ni_kontrol_f1_t *)base;

	/* Turn off all lights */
	memset(&dev->lights[1], 0, NI_KONTROL_F1_LED_COUNT);
	dev->lights[0] = 0x80;
	ni_kontrol_f1_light_set(base, 0, 0);
	ni_kontrol_f1_light_flush(base, 0);

	free(dev);
	return 0;
}

struct ctlr_dev_t *
ni_kontrol_f1_connect(ctlr_event_func event_func,
				  void *userdata, void *future)
{
	(void)future;
	struct ni_kontrol_f1_t *dev = calloc(1, sizeof(struct ni_kontrol_f1_t));
	if(!dev)
		goto fail;

	int err = ctlr_dev_impl_usb_open((struct ctlr_dev_t *)dev,
					 NI_VENDOR, NI_KONTROL_F1,
					 USB_INTERFACE_ID, 0);
	if(err) {
		printf("error conencting to F1 controller, is it plugged in?\n");
		free(dev);
		return 0;
	}

	dev->base.poll = ni_kontrol_f1_poll;
	dev->base.disconnect = ni_kontrol_f1_disconnect;
	dev->base.light_set = ni_kontrol_f1_light_set;
	dev->base.control_get_name = ni_kontrol_f1_control_get_name;
	dev->base.light_flush = ni_kontrol_f1_light_flush;

	dev->base.event_func = event_func;
	dev->base.event_func_userdata = userdata;

	return (struct ctlr_dev_t *)dev;
fail:
	free(dev);
	return 0;
}

