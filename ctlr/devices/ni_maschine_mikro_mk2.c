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

#include "ni_maschine_mikro_mk2.h"
#include "../device_impl.h"

#define NI_VENDOR             (0x17cc)
#define NI_MASCHINE_MIKRO_MK2 (0x1200)
#define USB_INTERFACE_ID      (0x00)
#define USB_ENDPOINT_READ     (0x81)
#define USB_ENDPOINT_WRITE    (0x01)

/* This struct is a generic struct to identify hw controls */
struct ni_maschine_mikro_mk2_ctlr_t {
	int event_id;
	int buf_byte_offset;
	uint32_t mask;
};

static const char *ni_maschine_mikro_mk2_control_names[] = {
	/* Buttons */
	"Restart",
	"Arrow (Left)",
	"Arrow (Right)",
	"Grid",
	"Play",
	"Record",
	"Erase",
	"Shift",
	"Group",
	"Browse",
	"Sampling",
	"Note Repeat",
	"Encoder Press",
	"F1",
	"F2",
	"F3",
	"Control",
	"Nav",
	"Nav Arrow (Left)",
	"Nav Arrow (Right)",
	"Main",
	"Scene",
	"Pattern",
	"Pad Mode",
	"View",
	"Duplicate",
	"Select",
	"Solo",
	"Mute",
	/* Encoder */
	"Encoder Rotate",
};
#define CONTROL_NAMES_SIZE (sizeof(ni_maschine_mikro_mk2_control_names) /\
			    sizeof(ni_maschine_mikro_mk2_control_names[0]))

static const struct ni_maschine_mikro_mk2_ctlr_t buttons[] = {
	{NI_MASCHINE_MIKRO_MK2_BTN_RESTART    , 1, 0x80},
	{NI_MASCHINE_MIKRO_MK2_BTN_LEFT_ARROW , 1, 0x40},
	{NI_MASCHINE_MIKRO_MK2_BTN_RIGHT_ARROW, 1, 0x20},
	{NI_MASCHINE_MIKRO_MK2_BTN_GRID       , 1, 0x10},
	{NI_MASCHINE_MIKRO_MK2_BTN_PLAY  , 1, 0x08},
	{NI_MASCHINE_MIKRO_MK2_BTN_RECORD, 1, 0x04},
	{NI_MASCHINE_MIKRO_MK2_BTN_ERASE , 1, 0x02},
	{NI_MASCHINE_MIKRO_MK2_BTN_SHIFT , 1, 0x01},

	{NI_MASCHINE_MIKRO_MK2_BTN_GROUP      , 2, 0x80},
	{NI_MASCHINE_MIKRO_MK2_BTN_BROWSE     , 2, 0x40},
	{NI_MASCHINE_MIKRO_MK2_BTN_SAMPLING   , 2, 0x20},
	{NI_MASCHINE_MIKRO_MK2_BTN_NOTE_REPEAT, 2, 0x10},

	{NI_MASCHINE_MIKRO_MK2_BTN_ENCODER_PRESS, 2, 0x08},
	/* unused
	{NI_MASCHINE_MIKRO_MK2_BTN_ , 2, 0x0},
	{NI_MASCHINE_MIKRO_MK2_BTN_ , 2, 0x0},
	{NI_MASCHINE_MIKRO_MK2_BTN_ , 2, 0x0},
	*/

	{NI_MASCHINE_MIKRO_MK2_BTN_F1       , 3, 0x80},
	{NI_MASCHINE_MIKRO_MK2_BTN_F2       , 3, 0x40},
	{NI_MASCHINE_MIKRO_MK2_BTN_F3       , 3, 0x20},
	{NI_MASCHINE_MIKRO_MK2_BTN_CONTROL  , 3, 0x10},
	{NI_MASCHINE_MIKRO_MK2_BTN_NAV      , 3, 0x08},
	{NI_MASCHINE_MIKRO_MK2_BTN_NAV_LEFT , 3, 0x04},
	{NI_MASCHINE_MIKRO_MK2_BTN_NAV_RIGHT, 3, 0x02},
	{NI_MASCHINE_MIKRO_MK2_BTN_MAIN     , 3, 0x01},

	{NI_MASCHINE_MIKRO_MK2_BTN_SCENE    , 4, 0x80},
	{NI_MASCHINE_MIKRO_MK2_BTN_PATTERN  , 4, 0x40},
	{NI_MASCHINE_MIKRO_MK2_BTN_PAD_MODE , 4, 0x20},
	{NI_MASCHINE_MIKRO_MK2_BTN_VIEW     , 4, 0x10},
	{NI_MASCHINE_MIKRO_MK2_BTN_DUPLICATE, 4, 0x08},
	{NI_MASCHINE_MIKRO_MK2_BTN_SELECT   , 4, 0x04},
	{NI_MASCHINE_MIKRO_MK2_BTN_SOLO     , 4, 0x02},
	{NI_MASCHINE_MIKRO_MK2_BTN_MUTE     , 4, 0x01},
};
#define BUTTONS_SIZE (sizeof(buttons) / sizeof(buttons[0]))

static const struct ni_maschine_mikro_mk2_ctlr_t encoders[] = {
	{NI_MASCHINE_MIKRO_MK2_BTN_ENCODER_ROTATE, 5, 0x0F},
};
#define ENCODERS_SIZE (sizeof(encoders) / sizeof(encoders[0]))

#define CONTROLS_SIZE (BUTTONS_SIZE + ENCODERS_SIZE)

/* Represents the the hardware device */
struct ni_maschine_mikro_mk2_t {
	/* base handles usb i/o etc */
	struct ctlr_dev_t base;
	/* current value of each controller is stored here */
	float hw_values[CONTROLS_SIZE];
	/* current state of the lights, only flush on dirty */
	uint8_t lights_dirty;
#warning FIXME: lights array size
	uint8_t lights[200];
};

static const char *
ni_maschine_mikro_mk2_control_get_name(struct ctlr_dev_t *base,
			       uint32_t control_id)
{
	struct ni_maschine_mikro_mk2_t *dev = (struct ni_maschine_mikro_mk2_t *)base;
	if(control_id < CONTROL_NAMES_SIZE)
		return ni_maschine_mikro_mk2_control_names[control_id];
	return 0;
}

static uint32_t ni_maschine_mikro_mk2_poll(struct ctlr_dev_t *base)
{
	struct ni_maschine_mikro_mk2_t *dev = (struct ni_maschine_mikro_mk2_t *)base;
	uint8_t buf[1024];
	int32_t nbytes;

	do {
		int handle_idx = 0;
		nbytes = ctlr_dev_impl_usb_read(base, handle_idx,
						buf, 1024);
		if(nbytes == 0)
			return 0;

		switch(nbytes) {
		case 64: {
				for(int i = 0; i < 16; i++) {
					uint16_t v = *((uint16_t *)&buf[i*2]);
					//printf("%02x ", v);
				}
				//printf("\n");
			}
			break;
		case 6: {
			for(uint32_t i = 0; i < ENCODERS_SIZE; i++) {
#if 0
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
#endif
			}
			for(uint32_t i = 0; i < BUTTONS_SIZE; i++) {
				int id     = buttons[i].event_id;
				int offset = buttons[i].buf_byte_offset;
				int mask   = buttons[i].mask;

				uint16_t v = *((uint16_t *)&buf[offset]) & mask;
#warning check this is correct - removed SLIDER_SIZE
				int value_idx = i;

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

static void ni_maschine_mikro_mk2_light_set(struct ctlr_dev_t *base,
				    uint32_t light_id,
				    uint32_t light_status)
{
	struct ni_maschine_mikro_mk2_t *dev = (struct ni_maschine_mikro_mk2_t *)base;
	int ret;

	if(!dev || light_id > NI_MASCHINE_MIKRO_MK2_LED_COUNT)
		return;

	/* write brighness to all LEDs */
	uint32_t bright = (light_status >> 24) & 0x7F;
	dev->lights[light_id] = bright;

#if 0
	switch(light_id)
	{
	case 1 ... 8: /* fallthrough */
	case 10 ... 30:
		dev->light_buf[light_id] = (status >> 24) & 0x7F;
		break;
	case 9:
		dev->light_buf[light_id  ] = ((status >> 16) & 0xFF) / 2;
		dev->light_buf[light_id+1] = ((status >>  8) & 0xFF) / 2;
		dev->light_buf[light_id+2] = ((status >>  0) & 0xFF) / 2;
		break;
	case 31 ... 47:
		dev->light_buf[light_id  ] = ((status >> 16) & 0xFF) / 2;
		dev->light_buf[light_id+1] = ((status >>  8) & 0xFF) / 2;
		dev->light_buf[light_id+2] = ((status >>  0) & 0xFF) / 2;
		break;
	}
#endif

	dev->lights_dirty = 1;
}

void
ni_maschine_mikro_mk2_light_flush(struct ctlr_dev_t *base, uint32_t force)
{
	struct ni_maschine_mikro_mk2_t *dev = (struct ni_maschine_mikro_mk2_t *)base;
	if(!dev->lights_dirty && !force)
		return;

	memset(dev->lights, 0xff, 128);
	dev->lights[0] = 0x80;

	int ret = ctlr_dev_impl_usb_write(base, 0,
					  dev->lights,
					  80);
	if(ret < 0)
		printf("%s write failed!\n", __func__);
}

static int32_t
ni_maschine_mikro_mk2_disconnect(struct ctlr_dev_t *base)
{
	struct ni_maschine_mikro_mk2_t *dev = (struct ni_maschine_mikro_mk2_t *)base;

	/* Turn off all lights */
	ni_maschine_mikro_mk2_light_set(base, 1, 0);
	ni_maschine_mikro_mk2_light_flush(base, 1);

	free(dev);
	return 0;
}

struct ctlr_dev_t *
ni_maschine_mikro_mk2_connect(ctlr_event_func event_func,
				  void *userdata, void *future)
{
	(void)future;
	struct ni_maschine_mikro_mk2_t *dev = calloc(1, sizeof(struct ni_maschine_mikro_mk2_t));
	if(!dev)
		goto fail;

	snprintf(dev->base.info.vendor, sizeof(dev->base.info.vendor),
		 "%s", "Native Instruments");
	snprintf(dev->base.info.device, sizeof(dev->base.info.device),
		 "%s", "Maschine Mikro Mk2");

	int err = ctlr_dev_impl_usb_open((struct ctlr_dev_t *)dev,
					 NI_VENDOR, NI_MASCHINE_MIKRO_MK2,
					 USB_INTERFACE_ID, 0);
	if(err) {
		free(dev);
		return 0;
	}

	dev->base.poll = ni_maschine_mikro_mk2_poll;
	dev->base.disconnect = ni_maschine_mikro_mk2_disconnect;
	dev->base.light_set = ni_maschine_mikro_mk2_light_set;
	dev->base.control_get_name = ni_maschine_mikro_mk2_control_get_name;
	dev->base.light_flush = ni_maschine_mikro_mk2_light_flush;

	dev->base.event_func = event_func;
	dev->base.event_func_userdata = userdata;

	return (struct ctlr_dev_t *)dev;
fail:
	free(dev);
	return 0;
}

