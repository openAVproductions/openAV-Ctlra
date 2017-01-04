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
#define USB_HANDLE_IDX        (0x00)
#define USB_ENDPOINT_READ     (0x81)
#define USB_ENDPOINT_WRITE    (0x01)

/* This struct is a generic struct to identify hw controls */
struct ni_maschine_mikro_mk2_ctlra_t {
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

static const struct ni_maschine_mikro_mk2_ctlra_t buttons[] = {
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

static const struct ni_maschine_mikro_mk2_ctlra_t encoders[] = {
	{NI_MASCHINE_MIKRO_MK2_BTN_ENCODER_ROTATE, 5, 0x0F},
};
#define ENCODERS_SIZE (sizeof(encoders) / sizeof(encoders[0]))

#define CONTROLS_SIZE (BUTTONS_SIZE + ENCODERS_SIZE)

#define LIGHTS_SIZE (80)

#define NPADS                  (16)
/* KERNEL_LENGTH must be a power of 2 for masking */
#define KERNEL_LENGTH          (16)
#define KERNEL_MASK            (KERNEL_LENGTH-1)
#define PAD_SENSITIVITY        (150)
#define PAD_PRESS_BRIGHTNESS   (0.9999f)
#define PAD_RELEASE_BRIGHTNESS (0.25f)

/* Represents the the hardware device */
struct ni_maschine_mikro_mk2_t {
	/* base handles usb i/o etc */
	struct ctlra_dev_t base;
	/* current value of each controller is stored here */
	float hw_values[CONTROLS_SIZE];
	/* current state of the lights, only flush on dirty */
	uint8_t lights_dirty;

	/* Lights endpoint used to transfer with hidapi */
	uint8_t lights_endpoint;
	uint8_t lights[LIGHTS_SIZE];

	/* Pressure filtering for note-onset detection */
	uint8_t  pad_idx;
	uint16_t pads[NPADS]; /* current state */
	uint16_t pad_avg[NPADS]; /* average */
	uint16_t pad_pressures[NPADS*KERNEL_LENGTH]; /* values history */
};

static const char *
ni_maschine_mikro_mk2_control_get_name(const struct ctlra_dev_t *base,
                                       enum ctlra_event_type_t type,
                                       uint32_t control_id)
{
	struct ni_maschine_mikro_mk2_t *dev = (struct ni_maschine_mikro_mk2_t *)base;
	if(control_id < CONTROL_NAMES_SIZE)
		return ni_maschine_mikro_mk2_control_names[control_id];
	return 0;
}

/* * The following code is public domain.  * Algorithm by Torben Mogensen, implementation
by N. Devillard.  * This code in public domain.  */
float torben(float m[], int n)
{
	int i, less, greater, equal;
	float min, max, guess, maxltguess, mingtguess;
	min = max = m[0];
	for (i=1 ; i<n ; i++) {
		if (m[i]<min)
			min=m[i];
		if (m[i]>max)
			max=m[i];
	}
	while (1) {
		guess = (min+max)/2;
		less = 0;
		greater = 0;
		equal = 0;
		maxltguess = min ;
		mingtguess = max ;
		for (i=0; i<n; i++) {
			if (m[i]<guess) {
				less++;
				if (m[i]>maxltguess)
					maxltguess = m[i];
			} else if (m[i]>guess) {
				greater++;
				if (m[i]<mingtguess)
					mingtguess = m[i] ;
			} else
				equal++;
		}
		if (less <= (n+1)/2 && greater <= (n+1)/2)
			break;
		else if (less>greater)
			max = maxltguess;
		else
			min = mingtguess;
	}
	if (less >= (n+1)/2)
		return maxltguess;
	else if (less+equal >= (n+1)/2)
		return guess;
	else
		return mingtguess;
}

static uint32_t ni_maschine_mikro_mk2_poll(struct ctlra_dev_t *base)
{
	struct ni_maschine_mikro_mk2_t *dev = (struct ni_maschine_mikro_mk2_t *)base;
	uint8_t buf[1024];
	int32_t nbytes;

	int iter = 0;
	do {
		nbytes = ctlra_dev_impl_usb_interrupt_read(base, USB_HANDLE_IDX,
		                USB_ENDPOINT_READ,
		                buf, 1024);
		//printf("mm read %d\n", nbytes);
		if(nbytes == 0)
			return 0;

		switch(nbytes) {
		case 65: {
			uint8_t idx = dev->pad_idx++ & KERNEL_MASK;
			int changed = 0;
			for(int i = 0; i < NPADS; i++) {
				uint16_t v = buf[0] | ((buf[1] & 0x0F) << 8);
				if(i == 0) {
					//printf("%04d\n", v);
				}

				uint16_t total = 0;
				for(int j = 0; j < KERNEL_LENGTH; j++)
					total += dev->pad_pressures[i*KERNEL_LENGTH + j];

				dev->pad_pressures[i*KERNEL_LENGTH + idx] = v;

				uint16_t avg = total / KERNEL_LENGTH;
				dev->pad_avg[i] = avg;

				if(avg > PAD_SENSITIVITY && dev->pads[i] == 0) {
					printf("%d pressed, avg = %d\n", i, avg);
					dev->pads[i] = 1;
					changed = 1;
				} else if(avg < PAD_SENSITIVITY && dev->pads[i] > 0) {
					printf("%d released, avg = %d\n", i, avg);
					dev->pads[i] = 0;
					changed = 1;
				}
			}
			if(changed) {
				for(int i = 0; i < NPADS; i++) {
					printf("%04d ", dev->pad_avg[i]);
				}
				printf("\n");
			}
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
					struct ctlra_event_t event = {
						.id = CTLRA_EVENT_SLIDER,
						.slider  = {
							.id = id,
							.value = v / 4096.f
						},
					};
					struct ctlra_event_t *e = {&event};
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
				int value_idx = i;

				if(dev->hw_values[value_idx] != v) {
					//printf("%s %d\n", ni_maschine_mikro_mk2_control_names[i], i);
					dev->hw_values[value_idx] = v;

					struct ctlra_event_t event = {
						.type = CTLRA_EVENT_BUTTON,
						.button  = {
							.id = id,
							.pressed = v > 0
						},
					};
					struct ctlra_event_t *e = {&event};
					dev->base.event_func(&dev->base, 1, &e,
					                     dev->base.event_func_userdata);
				}
			}
			break;
		}
		}
		iter++;
	} while (nbytes > 0 && iter < 2);

	return 0;
}

static void ni_maschine_mikro_mk2_light_set(struct ctlra_dev_t *base,
                uint32_t light_id,
                uint32_t light_status)
{
	struct ni_maschine_mikro_mk2_t *dev = (struct ni_maschine_mikro_mk2_t *)base;
	int ret;

	if(!dev || light_id > sizeof(dev->lights))
		return;

	/* write brighness to all LEDs */
	uint32_t bright = (light_status >> 24) & 0x7F;
	dev->lights[light_id] = bright;

#if 0
	switch(light_id) {
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
ni_maschine_mikro_mk2_light_flush(struct ctlra_dev_t *base, uint32_t force)
{
	struct ni_maschine_mikro_mk2_t *dev = (struct ni_maschine_mikro_mk2_t *)base;
	if(!dev->lights_dirty && !force)
		return;

	uint8_t *data = &dev->lights_endpoint;
	dev->lights_endpoint = 0x80;

	int ret = ctlra_dev_impl_usb_interrupt_write(base, USB_HANDLE_IDX,
	                USB_ENDPOINT_WRITE,
	                data, LIGHTS_SIZE);
	if(ret < 0)
		printf("%s write failed!\n", __func__);
}

static int32_t
ni_maschine_mikro_mk2_disconnect(struct ctlra_dev_t *base)
{
	struct ni_maschine_mikro_mk2_t *dev = (struct ni_maschine_mikro_mk2_t *)base;

	memset(dev->lights, 0x0, LIGHTS_SIZE);
	if(!base->banished)
		ni_maschine_mikro_mk2_light_flush(base, 1);

	ctlra_dev_impl_usb_close(base);
	free(dev);
	return 0;
}

struct ctlra_dev_t *
ni_maschine_mikro_mk2_connect(ctlra_event_func event_func,
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

	int err = ctlra_dev_impl_usb_open((struct ctlra_dev_t *)dev,
	                                  NI_VENDOR, NI_MASCHINE_MIKRO_MK2);
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

	memset(dev->lights, 0xff, sizeof(dev->lights));
	ni_maschine_mikro_mk2_light_flush(&dev->base, 1);

	dev->base.poll = ni_maschine_mikro_mk2_poll;
	dev->base.disconnect = ni_maschine_mikro_mk2_disconnect;
	dev->base.light_set = ni_maschine_mikro_mk2_light_set;
	dev->base.control_get_name = ni_maschine_mikro_mk2_control_get_name;
	dev->base.light_flush = ni_maschine_mikro_mk2_light_flush;

	dev->base.event_func = event_func;
	dev->base.event_func_userdata = userdata;

	return (struct ctlra_dev_t *)dev;
fail:
	free(dev);
	return 0;
}

