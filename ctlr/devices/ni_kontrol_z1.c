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

#include "device_impl.h"

#define NI_VENDOR       0x17cc
#define NI_KONTROL_Z1   0x1210
#define USB_INTERFACE_ID   (0x03)
#define USB_ENDPOINT_READ  (0x82)
#define USB_ENDPOINT_WRITE (0x02)

/* This struct is a generic struct to identify hw controls */
#warning TODO: move name to .h file for application lookup
struct ni_kontrol_z1_ctlr_t {
	char name[29];
	int event_id;
	int buf_byte_offset;
	uint32_t mask;
};

static const struct ni_kontrol_z1_ctlr_t sliders[] = {
	/* Left */
	{"left gain"       , 0,  1, UINT32_MAX},
	{"left eq high"    , 1,  3, UINT32_MAX},
	{"left eq mid"     , 2,  5, UINT32_MAX},
	{"left eq low"     , 3,  7, UINT32_MAX},
	{"left filter"     , 4,  9, UINT32_MAX},
	{"right gain"      , 5, 11, UINT32_MAX},
	{"right eq high"   , 6, 13, UINT32_MAX},
	{"right eq mid"    , 7, 15, UINT32_MAX},
	{"right eq low"    , 8, 17, UINT32_MAX},
	{"right filter"    , 9, 19, UINT32_MAX},
	{"cue mix"         ,10, 21, UINT32_MAX},
	{"left fader"      ,11, 23, UINT32_MAX},
	{"right fader"     ,12, 25, UINT32_MAX},
	{"cross fader"     ,13, 27, UINT32_MAX},
};
#define SLIDERS_SIZE (sizeof(sliders) / sizeof(sliders[0]))

static const struct ni_kontrol_z1_ctlr_t buttons[] = {
	{"headphones cue B", 0, 29, 0x10},
	{"headphones cue B", 1, 29, 0x1},
	{"mode"            , 2, 29, 0x2},
	{"left filter on"  , 3, 29, 0x4},
	{"right filter on" , 4, 29, 0x8},
};
#define BUTTONS_SIZE (sizeof(buttons) / sizeof(buttons[0]))

#define CONTROLS_SIZE (SLIDERS_SIZE + BUTTONS_SIZE)

struct ni_kontrol_z1_controls_t {
	uint8_t waste;
	/* Left mixer chan */
	uint16_t left_gain;
	uint16_t left_high;
	uint16_t left_mid;
	uint16_t left_low;
	uint16_t left_filter;
	/* Right mixer chan */
	uint16_t right_gain;
	uint16_t right_high;
	uint16_t right_mid;
	uint16_t right_low;
	uint16_t right_filter;
	/* Mixer and Misc */
	uint16_t cue_mix;
	uint16_t left_fader;
	uint16_t right_fader;
	uint16_t crossfader;
	/* Buttons */
};

struct ni_kontrol_z1_t {
	/* base handles usb i/o etc */
	struct ctlr_dev_t base;

	/* current value of each controller is stored here */
	float hw_values[CONTROLS_SIZE];

	/* current state of the lights */
	uint8_t lights[22];
};

static uint32_t ni_kontrol_z1_poll(struct ctlr_dev_t *dev);
static int32_t ni_kontrol_z1_disconnect(struct ctlr_dev_t *dev);
static int32_t ni_kontrol_z1_disconnect(struct ctlr_dev_t *dev);
static void ni_kontrol_z1_light_set(struct ctlr_dev_t *dev, uint32_t light_id,
				uint32_t light_status);

struct ctlr_dev_t *ni_kontrol_z1_connect(ctlr_event_func event_func,
				  void *userdata, void *future)
{
	(void)future;
	struct ni_kontrol_z1_t *dev = calloc(1, sizeof(struct ni_kontrol_z1_t));
	if(!dev)
		goto fail;

	int err = ctlr_dev_impl_usb_open((struct ctlr_dev_t *)dev,
					 NI_VENDOR, NI_KONTROL_Z1,
					 USB_INTERFACE_ID, 0);
	if(err) {
		printf("error conencting to Kontrol Z1 controller, is it plugged in?\n");
		return 0;
	}

	dev->base.poll = ni_kontrol_z1_poll;
	dev->base.disconnect = ni_kontrol_z1_disconnect;
	dev->base.light_set = ni_kontrol_z1_light_set;

	dev->base.event_func = event_func;
	dev->base.event_func_userdata = userdata;

	return (struct ctlr_dev_t *)dev;
fail:
	free(dev);
	return 0;
}

static uint32_t ni_kontrol_z1_poll(struct ctlr_dev_t *base)
{
	struct ni_kontrol_z1_t *dev = (struct ni_kontrol_z1_t *)base;
	uint8_t buf[1024];
	uint32_t nbytes;


	do {
#warning refactor this to use the ctlr_dev_t abstraction for I/O
		int r;
		int transferred;
		r = libusb_interrupt_transfer(base->usb_handle,
					      USB_ENDPOINT_READ, buf,
					      1024, &transferred, 1000);
		if (r == LIBUSB_ERROR_TIMEOUT) {
			fprintf(stderr, "timeout\n");
			return 0;
		}
		nbytes = transferred;
		if(nbytes == 0)
			return 0;

		switch(nbytes) {
		case 30: {
			for(uint32_t i = 0; i < SLIDERS_SIZE; i++) {
				const char* name = sliders[i].name;
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
					ni_kontrol_z1_light_set(&dev->base, 0, 0);
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

static int32_t ni_kontrol_z1_disconnect(struct ctlr_dev_t *base)
{
	struct ni_kontrol_z1_t *dev = (struct ni_kontrol_z1_t *)base;
	free(dev);
	return 0;
}

static void ni_kontrol_z1_light_set(struct ctlr_dev_t *base,
				    uint32_t light_id,
				    uint32_t light_status)
{
	uint32_t blink  = (light_status >> 31);
	uint32_t bright = (light_status >> 24) & 0x7F;
	uint32_t r      = (light_status >> 16) & 0xFF;
	uint32_t g      = (light_status >>  8) & 0xFF;
	uint32_t b      = (light_status >>  0) & 0xFF;
#ifdef DEBUG_PRINTS
	printf("%s : dev %p, light %d, status %d\n", __func__, dev,
	       light_id, light_status);
	printf("decoded: blink[%d], bright[%d], r[%d], g[%d], b[%d]\n",
	       blink, bright, r, g, b);
#endif

	const int size = 22;
	uint8_t buf[size];
	memset(buf, 0x0, size);
	for(int i = 0; i < size; i++) {
		buf[0] = 0x80;
		buf[i+1] = 0xFF;
		printf("%d\n", i+1);
		int ret = ctlr_dev_impl_usb_write(base, USB_INTERFACE_ID,
						  USB_ENDPOINT_WRITE,
						  buf, size);
		sleep(1);
	}

}
