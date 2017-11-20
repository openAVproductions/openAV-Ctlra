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
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#include "ni_maschine_mikro_mk2.h"
#include "impl.h"

#define CTLRA_DRIVER_VENDOR (0x17cc)
#define CTLRA_DRIVER_DEVICE (0x1200)
#define USB_INTERFACE_ID      (0x00)
#define USB_HANDLE_IDX        (0x00)
#define USB_ENDPOINT_READ     (0x81)
#define USB_ENDPOINT_WRITE    (0x01)

#define USE_LIBUSB 1

/* This struct is a generic struct to identify hw controls */
struct ni_maschine_mikro_mk2_ctlra_t {
	int event_id;
	int buf_byte_offset;
	uint32_t mask;
};

static const char *ni_maschine_mikro_mk2_control_names[] = {
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
	"Enc Btn",
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

#define MIKRO_BTN (CTLRA_ITEM_BUTTON | CTLRA_ITEM_LED_INTENSITY | CTLRA_ITEM_HAS_FB_ID)
static struct ctlra_item_info_t buttons_info[] = {
	/* restart -> grid */
	{.x = 20, .y = 138, .w = 18,  .h = 10, .flags = MIKRO_BTN, .colour = 0xff000000, .fb_id = NI_MASCHINE_MIKRO_MK2_LED_RESTART},
	{.x = 45, .y = 138, .w = 18,  .h = 10, .flags = MIKRO_BTN, .colour = 0xff000000, .fb_id = NI_MASCHINE_MIKRO_MK2_LED_LEFT},
	{.x = 70, .y = 138, .w = 18,  .h = 10, .flags = MIKRO_BTN, .colour = 0xff000000, .fb_id = NI_MASCHINE_MIKRO_MK2_LED_RIGHT},
	{.x = 95, .y = 138, .w = 18,  .h = 10, .flags = MIKRO_BTN, .colour = 0xff000000, .fb_id = NI_MASCHINE_MIKRO_MK2_LED_GRID},
	/* play -> shift */
	{.x = 20, .y = 156, .w = 18,  .h = 10, .flags = MIKRO_BTN, .colour = 0xff00ff00, .fb_id = NI_MASCHINE_MIKRO_MK2_LED_PLAY},
	{.x = 45, .y = 156, .w = 18,  .h = 10, .flags = MIKRO_BTN, .colour = 0xffff0000, .fb_id = NI_MASCHINE_MIKRO_MK2_LED_REC},
	{.x = 70, .y = 156, .w = 18,  .h = 10, .flags = MIKRO_BTN, .colour = 0xff000000, .fb_id = NI_MASCHINE_MIKRO_MK2_LED_ERASE},
	{.x = 95, .y = 156, .w = 18,  .h = 10, .flags = MIKRO_BTN, .colour = 0xff000000, .fb_id = NI_MASCHINE_MIKRO_MK2_LED_SHIFT},
	/* group -> note repeat */
	{.x = 20, .y = 107, .w = 18,  .h = 10, .flags = MIKRO_BTN | CTLRA_ITEM_LED_COLOR, .colour = 0xff000000, .fb_id = NI_MASCHINE_MIKRO_MK2_LED_GROUP},
	{.x = 45, .y = 107, .w = 18,  .h = 10, .flags = MIKRO_BTN, .colour = 0xff000000, .fb_id = NI_MASCHINE_MIKRO_MK2_LED_BROWSE},
	{.x = 70, .y = 107, .w = 18,  .h = 10, .flags = MIKRO_BTN, .colour = 0xff000000, .fb_id = NI_MASCHINE_MIKRO_MK2_LED_SAMPLING},
	{.x = 95, .y = 107, .w = 18,  .h = 10, .flags = MIKRO_BTN, .colour = 0xff000000, .fb_id = NI_MASCHINE_MIKRO_MK2_LED_NOTE_REPEAT},
	/* encoder press */
	{.x = 95, .y =  70, .w = 18,  .h = 10, .flags = CTLRA_ITEM_BUTTON},
	/* F1 -> control */
	{.x = 20, .y =  29, .w = 18,  .h =  5, .flags = MIKRO_BTN, .colour = 0xff000000, .fb_id = NI_MASCHINE_MIKRO_MK2_LED_F1},
	{.x = 45, .y =  29, .w = 18,  .h =  5, .flags = MIKRO_BTN, .colour = 0xff000000, .fb_id = NI_MASCHINE_MIKRO_MK2_LED_F2},
	{.x = 70, .y =  29, .w = 18,  .h =  5, .flags = MIKRO_BTN, .colour = 0xff000000, .fb_id = NI_MASCHINE_MIKRO_MK2_LED_F3},
	{.x = 95, .y =  29, .w = 18,  .h =  5, .flags = MIKRO_BTN, .colour = 0xff000000, .fb_id = NI_MASCHINE_MIKRO_MK2_LED_CONTROL},
	/* Nav -> Main */
	{.x = 20, .y =  87, .w = 18,  .h =  5, .flags = MIKRO_BTN, .colour = 0xff000000, .fb_id = NI_MASCHINE_MIKRO_MK2_LED_NAV},
	{.x = 45, .y =  87, .w = 18,  .h =  5, .flags = MIKRO_BTN, .colour = 0xff000000, .fb_id = NI_MASCHINE_MIKRO_MK2_LED_NAV_LEFT},
	{.x = 70, .y =  87, .w = 18,  .h =  5, .flags = MIKRO_BTN, .colour = 0xff000000, .fb_id = NI_MASCHINE_MIKRO_MK2_LED_NAV_RIGHT},
	{.x = 95, .y =  87, .w = 18,  .h =  5, .flags = MIKRO_BTN, .colour = 0xff000000, .fb_id = NI_MASCHINE_MIKRO_MK2_LED_MAIN},
	/* Scene -> Mute */
	{.x =130, .y =  29, .w = 18,  .h = 10, .flags = MIKRO_BTN, .colour = 0xff000000, .fb_id = NI_MASCHINE_MIKRO_MK2_LED_SCENE},
	{.x =130, .y =  47, .w = 18,  .h = 10, .flags = MIKRO_BTN, .colour = 0xff000000, .fb_id = NI_MASCHINE_MIKRO_MK2_LED_PATTERN},
	{.x =130, .y =  66, .w = 18,  .h = 10, .flags = MIKRO_BTN, .colour = 0xff000000, .fb_id = NI_MASCHINE_MIKRO_MK2_LED_PAD_MODE},
	{.x =130, .y =  84, .w = 18,  .h = 10, .flags = MIKRO_BTN, .colour = 0xff000000, .fb_id = NI_MASCHINE_MIKRO_MK2_LED_VIEW},
	{.x =130, .y = 103, .w = 18,  .h = 10, .flags = MIKRO_BTN, .colour = 0xff000000, .fb_id = NI_MASCHINE_MIKRO_MK2_LED_DUPLICATE},
	{.x =130, .y = 121, .w = 18,  .h = 10, .flags = MIKRO_BTN, .colour = 0xff000000, .fb_id = NI_MASCHINE_MIKRO_MK2_LED_SELECT},
	{.x =130, .y = 140, .w = 18,  .h = 10, .flags = MIKRO_BTN, .colour = 0xff000000, .fb_id = NI_MASCHINE_MIKRO_MK2_LED_SOLO},
	{.x =130, .y = 158, .w = 18,  .h = 10, .flags = MIKRO_BTN, .colour = 0xff000000, .fb_id = NI_MASCHINE_MIKRO_MK2_LED_MUTE},
};

static struct ctlra_item_info_t feedback_info[] = {
	/* Screen */
	{.x = 20, .y =  44, .w = 70,  .h = 35, .flags = CTLRA_ITEM_FB_SCREEN},
};
#define FEEDBACK_SIZE (sizeof(feedback_info) / sizeof(feedback_info[0]))

#define ENCODERS_SIZE (1)

#define CONTROLS_SIZE (BUTTONS_SIZE + ENCODERS_SIZE)

#define LIGHTS_SIZE (80)

#define NPADS                  (16)
/* KERNEL_LENGTH must be a power of 2 for masking */
#define KERNEL_LENGTH          (8)
#define KERNEL_MASK            (KERNEL_LENGTH-1)
#define PAD_SENSITIVITY        (650)
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

	/* Store the current encoder value */
	uint8_t encoder_value;
	/* Pressure filtering for note-onset detection */
	uint8_t pad_idx[NPADS];
	uint16_t pads[NPADS];
	uint16_t pad_avg[NPADS];
	uint16_t pad_pressures[NPADS*KERNEL_LENGTH];

	int fd;
};

static const char *
ni_maschine_mikro_mk2_control_get_name(enum ctlra_event_type_t type,
                                       uint32_t control_id)
{
	if(control_id < CONTROL_NAMES_SIZE)
		return ni_maschine_mikro_mk2_control_names[control_id];
	return 0;
}

static uint32_t ni_maschine_mikro_mk2_poll(struct ctlra_dev_t *base)
{
	struct ni_maschine_mikro_mk2_t *dev = (struct ni_maschine_mikro_mk2_t *)base;
	uint8_t buf[1024];
	ctlra_dev_impl_usb_interrupt_read(base, USB_HANDLE_IDX,
					  USB_ENDPOINT_READ,
					  buf, 1024);
	return 0;
}


void
ni_maschine_mikro_mk2_usb_read_cb(struct ctlra_dev_t *base,
				  uint32_t endpoint, uint8_t *data,
				  uint32_t size)
{
	struct ni_maschine_mikro_mk2_t *dev =
		(struct ni_maschine_mikro_mk2_t *)base;
	static double worst_poll;
	int32_t nbytes = size;

	int count = 0;

	do {
		struct timeval tv1, tv2;
		gettimeofday(&tv1, NULL);
#ifndef USE_LIBUSB
		nbytes = read(dev->fd, &buf, sizeof(buf));
		if (nbytes < 0) {
			break;
		}
		gettimeofday(&tv2, NULL);
		double delta =
			(double) (tv2.tv_usec - tv1.tv_usec) / 1000000 +
			(double) (tv2.tv_sec - tv1.tv_sec);
		if(delta > worst_poll) {
			printf ("worst poll %f\n", delta);
			worst_poll = delta;
		}
		uint8_t *data = &buf[1];
#endif
		uint8_t *buf = data;

		switch(nbytes) {
		case 65: {
			int i;
			for (i = 0; i < NPADS; i++) {
				uint16_t new = ((data[i+2] & 0xf) << 8) |
						 data[i+1];

				uint8_t idx =
					dev->pad_idx[i]++ & (KERNEL_LENGTH-1);

				uint16_t total = 0;
				for(int j = 0; j < KERNEL_LENGTH; j++) {
					int idx = i*KERNEL_LENGTH + j;
					total += dev->pad_pressures[idx];
				}

				dev->pad_avg[i] = total / KERNEL_LENGTH;
				dev->pad_pressures[i*KERNEL_LENGTH + idx] = new;

				/*
				const uint16_t trigger_sense = PAD_SENSITIVITY;
				if((new - dev->pad_avg[i]) < trigger_sense)
					break;

				printf("TRIGGER: new %d\tavg %d\n", new,
				       dev->pad_avg[i]);
				dev->pads[i] = new;
				*/

				struct ctlra_event_t event = {
					.type = CTLRA_EVENT_GRID,
					.grid  = {
						.id = 0,
						.flags = CTLRA_EVENT_GRID_FLAG_BUTTON,
						.pos = i,
						.pressed = 1
					},
				};
				struct ctlra_event_t *e = {&event};
				if(dev->pad_avg[i] > 200 && dev->pads[i] == 0) {
					/* detect velocity over limit */
					float velo = (dev->pad_avg[i] - 200) / 200.f;
					float v2 = velo * velo * velo * velo;
					float fin = (velo - v2) * 3;
					fin = fin > 1.0f ? 1.0f : fin;
					fin = fin < 0.0f ? 0.0f : fin;
					//printf("\nfin: %f\tvelo: %f\tv2: %f\n", fin, velo, v2);
					e->grid.pressure = fin;
					dev->base.event_func(&dev->base, 1, &e,
					                     dev->base.event_func_userdata);
					//printf("%d pressed\n", i);
					dev->pads[i] = 2000;
				} else if(dev->pad_avg[i] < 150 && dev->pads[i] > 0) {
					//printf("%d release\n", i);
					dev->pads[i] = 0;
					event.grid.pressed = 0;
					event.grid.pressure = 0.f;
					dev->base.event_func(&dev->base, 1, &e,
					                     dev->base.event_func_userdata);
				}
				break;
			}
		}
		break;
		case 6: {
			/* Encoder */
			struct ctlra_event_t event = {
				.type = CTLRA_EVENT_ENCODER,
				.encoder = {
					.id = NI_MASCHINE_MIKRO_MK2_BTN_ENCODER_ROTATE,
					.flags = CTLRA_EVENT_ENCODER_FLAG_INT,
					.delta = 0,
				},
			};
			struct ctlra_event_t *e = {&event};
			int8_t enc   = ((buf[5] & 0x0f)     ) & 0xf;
			if(enc != dev->encoder_value) {
				int dir = ctlra_dev_encoder_wrap_16(enc, dev->encoder_value);
				event.encoder.delta = dir;
				dev->encoder_value = enc;
				dev->base.event_func(&dev->base, 1, &e,
						     dev->base.event_func_userdata);
			}

			/* Buttons */
			for(uint32_t i = 0; i < BUTTONS_SIZE; i++) {
				int id     = buttons[i].event_id;
				int offset = buttons[i].buf_byte_offset;
				int mask   = buttons[i].mask;

				uint16_t v = *((uint16_t *)&buf[offset]) & mask;
				int value_idx = i;

				if(dev->hw_values[value_idx] != v) {
					//printf("%s %d\n",
					//ni_maschine_mikro_mk2_control_names[i], i);
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

		/* LIBUSB - no re-reading! */
		if(count++ < 10)
			ni_maschine_mikro_mk2_poll(base);
		else
			break;
	} while (nbytes > 0);
}

static void ni_maschine_mikro_mk2_light_set(struct ctlra_dev_t *base,
                uint32_t light_id,
                uint32_t light_status)
{
	struct ni_maschine_mikro_mk2_t *dev = (struct ni_maschine_mikro_mk2_t *)base;
	int ret;

	if(!dev)
		return;
	if(light_id > LIGHTS_SIZE) {
		return;
	}

	/* TODO: can we clean up the light_id handling somehow?
	 * There's a lot of branching / strange math per LED here */
	int idx = light_id;

	/* Group takes up 3 bytes, so add 2 if we're past the group */
	idx += 2 * (light_id > NI_MASCHINE_MIKRO_MK2_LED_GROUP);
	uint32_t r = (light_status >> 16) & 0x7F;
	uint32_t g = (light_status >>  8) & 0x7F;
	uint32_t b = (light_status >>  0) & 0x7F;

	/* Group btn and all pads */
	if(light_id == NI_MASCHINE_MIKRO_MK2_LED_GROUP) {
		dev->lights[ 8] = r;
		dev->lights[ 9] = g;
		dev->lights[10] = b;
	}
	else if (light_id >= NI_MASCHINE_MIKRO_MK2_LED_PAD_1 &&
		 light_id < NI_MASCHINE_MIKRO_MK2_LED_PAD_1 + 16) {
		int pad_id = light_id - NI_MASCHINE_MIKRO_MK2_LED_PAD_1;
		int p = NI_MASCHINE_MIKRO_MK2_LED_PAD_1 + (pad_id*3) + 2;
		dev->lights[p+0] = r;
		dev->lights[p+1] = g;
		dev->lights[p+2] = b;
	} else {
		 if(light_id > NI_MASCHINE_MIKRO_MK2_LED_PAD_1 + 16)
			return;
		/* write brighness to all LEDs */
		uint32_t bright = (light_status >> 24) & 0x7F;
		dev->lights[idx] = bright;
	}

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

	/* error handling in USB subsystem */
	ctlra_dev_impl_usb_interrupt_write(base,
					   USB_HANDLE_IDX,
					   USB_ENDPOINT_WRITE,
					   data,
					   LIGHTS_SIZE + 1);
}

static int32_t
ni_maschine_mikro_mk2_disconnect(struct ctlra_dev_t *base)
{
	struct ni_maschine_mikro_mk2_t *dev = (struct ni_maschine_mikro_mk2_t *)base;

	memset(dev->lights, 0x0, LIGHTS_SIZE);
	if(!base->banished)
		ni_maschine_mikro_mk2_light_flush(base, 1);

	free(dev);
	return 0;
}

struct ctlra_dev_info_t ctlra_ni_maschine_mikro_mk2_info;

struct ctlra_dev_t *
ctlra_ni_maschine_mikro_mk2_connect(ctlra_event_func event_func,
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

#ifdef USE_LIBUSB
	int err = ctlra_dev_impl_usb_open(&dev->base,
					  CTLRA_DRIVER_VENDOR,
					  CTLRA_DRIVER_DEVICE);
	if(err) {
		free(dev);
		return 0;
	}

	err = ctlra_dev_impl_usb_open_interface(&dev->base,
					 USB_INTERFACE_ID, USB_HANDLE_IDX);
	if(err) {
		printf("error opening interface\n");
		free(dev);
		return 0;
	}

	dev->base.usb_read_cb = ni_maschine_mikro_mk2_usb_read_cb;
#else
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <linux/hidraw.h>

	int fd, i, res, found = 0;
	char buf[256];
	struct hidraw_devinfo info;

	for(i = 0; i < 64; i++) {
		const char *device = "/dev/hidraw";
		snprintf(buf, sizeof(buf), "%s%d", device, i);
		fd = open(buf, O_RDWR|O_NONBLOCK); // |O_NONBLOCK
		if(fd < 0)
			continue;

		memset(&info, 0x0, sizeof(info));
		res = ioctl(fd, HIDIOCGRAWINFO, &info);

		if (res < 0) {
			perror("HIDIOCGRAWINFO");
		} else {
			if(info.vendor  == CTLRA_DRIVER_VENDOR &&
			   info.product == CTLRA_DRIVER_DEVICE) {
				found = 1;
				break;
			}
		}
		close(fd);
		/* continue searching next HID dev */
	}

	if(!found) {
		free(dev);
		return 0;
	}

	dev->fd = fd;
#endif

	dev->base.info.control_count[CTLRA_EVENT_BUTTON] =
		CONTROL_NAMES_SIZE - 1; /* -1 is encoder */
	dev->base.info.control_count[CTLRA_EVENT_ENCODER] = 1;
	dev->base.info.control_count[CTLRA_EVENT_GRID] = 1;
	dev->base.info.get_name = ni_maschine_mikro_mk2_control_get_name;

	struct ctlra_grid_info_t *grid = &dev->base.info.grid_info[0];
	grid->rgb = 1;
	grid->velocity = 1;
	grid->pressure = 1;
	grid->x = 4;
	grid->y = 4;

	dev->base.info.vendor_id = CTLRA_DRIVER_VENDOR;
	dev->base.info.device_id = CTLRA_DRIVER_DEVICE;

	dev->base.info = ctlra_ni_maschine_mikro_mk2_info;

	dev->base.poll = ni_maschine_mikro_mk2_poll;
	dev->base.disconnect = ni_maschine_mikro_mk2_disconnect;
	dev->base.light_set = ni_maschine_mikro_mk2_light_set;
	dev->base.light_flush = ni_maschine_mikro_mk2_light_flush;

	dev->base.event_func = event_func;
	dev->base.event_func_userdata = userdata;

	return (struct ctlra_dev_t *)dev;
fail:
	free(dev);
	return 0;
}

struct ctlra_dev_info_t ctlra_ni_maschine_mikro_mk2_info = {
	.vendor    = "Native Instruments",
	.device    = "Maschine Mikro Mk2",
	.vendor_id = CTLRA_DRIVER_VENDOR,
	.device_id = CTLRA_DRIVER_DEVICE,
	.size_x    = 320,
	.size_y    = 195,

	.control_count[CTLRA_EVENT_BUTTON] = BUTTONS_SIZE,
	.control_info [CTLRA_EVENT_BUTTON] = buttons_info,

	.control_count[CTLRA_FEEDBACK_ITEM] = FEEDBACK_SIZE,
	.control_info [CTLRA_FEEDBACK_ITEM] = feedback_info,

	.control_count[CTLRA_EVENT_GRID] = 1,
	.grid_info[0] = {
		.rgb = 1,
		.velocity = 1,
		.pressure = 1,
		.x = 4,
		.y = 4,
		.info = {
			.x = 168,
			.y = 29,
			.w = 138,
			.h = 138,
			/* start light id */
			.params[0] = NI_MASCHINE_MIKRO_MK2_LED_PAD_1,
			/* end light id */
			.params[1] = NI_MASCHINE_MIKRO_MK2_LED_PAD_16,
		}
	},

	.get_name = ni_maschine_mikro_mk2_control_get_name,
};

CTLRA_DEVICE_REGISTER(ni_maschine_mikro_mk2)
