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
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <sys/time.h>

#include "impl.h"

static inline uint64_t rdtsc(void)
{
	union {
		uint64_t tsc_64;
		struct {
			uint32_t lo_32;
			uint32_t hi_32;
		};
	} tsc;

	__asm__ volatile("rdtscp" :
		     "=a" (tsc.lo_32),
		     "=d" (tsc.hi_32));
	return tsc.tsc_64;
}

#define CTLRA_DRIVER_VENDOR (0x17cc)
#define CTLRA_DRIVER_DEVICE (0x1600)
#define USB_INTERFACE_ID      (0x04)
#define USB_HANDLE_IDX        (0x00)
#define USB_ENDPOINT_READ     (0x83)
#define USB_ENDPOINT_WRITE    (0x03)

#define USB_HANDLE_SCREEN_IDX     (0x1)
#define USB_INTERFACE_SCREEN      (0x5)
#define USB_ENDPOINT_SCREEN_WRITE (0x4)

/* This struct is a generic struct to identify hw controls */
struct ni_maschine_mk3_ctlra_t {
	int event_id;
	int buf_byte_offset;
	uint32_t mask;
};

static const char *ni_maschine_mk3_control_names[] = {
	"Enc. Press",
	"Enc. Right",
	"Enc. Up",
	"Enc. Left",
	"Enc. Down",
	"Shift",
	"Top 8",
	"A",
	"B",
	"C",
	"D",
	"E",
	"F",
	"G",
	"H",
	"Notes",
	"Volume",
	"Swing",
	"Tempo",
	"Note Repeat",
	"Lock",
	"Pad Mode",
	"Keyboard",
	"Chords",
	"Step",
	"Fixed Vel.",
	"Scene",
	"Pattern",
	"Events",
	"Variations",
	"Duplicate",
	"Select",
	"Solo",
	"Mute",
	"Pitch",
	"Mod",
	"Perform",
	"Restart",
	"Erase",
	"Tap",
	"Follow",
	"Play",
	"Rec",
	"Stop",
	"Macro",
	"Settings",
	"Arrow Right",
	"Sampling",
	"Mixer",
	"Plug-in",
	"Channel",
	"Arranger",
	"Browser",
	"Arrow Left",
	"File",
	"Auto",
	"Top 1",
	"Top 2",
	"Top 3",
	"Top 4",
	"Top 5",
	"Top 6",
	"Top 7",
	"Enc. Touch",
	"Enc. Touch 8",
	"Enc. Touch 7",
	"Enc. Touch 6",
	"Enc. Touch 5",
	"Enc. Touch 4",
	"Enc. Touch 3",
	"Enc. Touch 2",
	"Enc. Touch 1",
};
#define CONTROL_NAMES_SIZE (sizeof(ni_maschine_mk3_control_names) /\
			    sizeof(ni_maschine_mk3_control_names[0]))

static const struct ni_maschine_mk3_ctlra_t buttons[] = {
	/* encoder */
	{1, 1, 0x01},
	{1, 1, 0x08},
	{1, 1, 0x04},
	{1, 1, 0x20},
	{1, 1, 0x10},
	/* shift, top right above screen */
	{1, 1, 0x40},
	{1, 1, 0x80},
	/* group buttons ABCDEFGH */
	{1, 2, 0x01},
	{1, 2, 0x02},
	{1, 2, 0x04},
	{1, 2, 0x08},
	{1, 2, 0x10},
	{1, 2, 0x20},
	{1, 2, 0x40},
	{1, 2, 0x80},
	/* notes, volume, swing, tempo */
	{1, 3, 0x01},
	{1, 3, 0x02},
	{1, 3, 0x04},
	{1, 3, 0x08},
	/* Note Repeat, Lock */
	{1, 3, 0x10},
	{1, 3, 0x20},
	/* Pad mode, keyboard, chords, step */
	{1, 4, 0x01},
	{1, 4, 0x02},
	{1, 4, 0x04},
	{1, 4, 0x08},
	/* Fixed Vel, Scene, Pattern, Events */
	{1, 4, 0x10},
	{1, 4, 0x20},
	{1, 4, 0x40},
	{1, 4, 0x80},
	/* Variation, Dupliate, Select, Solo, Mute */
	/* 0x1 missing? */
	{1, 5, 0x02},
	{1, 5, 0x04},
	{1, 5, 0x08},
	{1, 5, 0x10},
	{1, 5, 0x20},
	/* Pitch, Mod */
	{1, 5, 0x40},
	{1, 5, 0x80},
	/* Perform, restart, erase, tap */
	{1, 6, 0x01},
	{1, 6, 0x02},
	{1, 6, 0x04},
	{1, 6, 0x08},
	/* Follow, Play, Rec, Stop */
	{1, 6, 0x10},
	{1, 6, 0x20},
	{1, 6, 0x40},
	{1, 6, 0x80},
	/* Macro, settings, > sampling mixer plugin */
	{1, 7, 0x01},
	{1, 7, 0x02},
	{1, 7, 0x04},
	{1, 7, 0x08},
	{1, 7, 0x10},
	{1, 7, 0x20},
	/* Channel, Arranger, Browser, <, File, Auto */
	{1, 8, 0x01},
	{1, 8, 0x02},
	{1, 8, 0x04},
	{1, 8, 0x08},
	{1, 8, 0x10},
	{1, 8, 0x20},
	/* "Top" buttons */
	{1, 9, 0x01},
	{1, 9, 0x02},
	{1, 9, 0x04},
	{1, 9, 0x08},
	{1, 9, 0x10},
	{1, 9, 0x20},
	{1, 9, 0x40},
	/* Encoder Touch */
	{1, 9, 0x80},
	/* Dial touch 8 - 1 */
	{1, 10, 0x01},
	{1, 10, 0x02},
	{1, 10, 0x04},
	{1, 10, 0x08},
	{1, 10, 0x10},
	{1, 10, 0x20},
	{1, 10, 0x40},
	{1, 10, 0x80},
};
#define BUTTONS_SIZE (sizeof(buttons) / sizeof(buttons[0]))

#define MK3_BTN (CTLRA_ITEM_BUTTON | CTLRA_ITEM_LED_INTENSITY | CTLRA_ITEM_HAS_FB_ID)
static struct ctlra_item_info_t buttons_info[] = {
	/* restart -> grid */
	{.x = 21, .y = 138, .w = 18,  .h = 10, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 0},
};

static struct ctlra_item_info_t feedback_info[] = {
	/* Screen */
	//{.x = 20, .y =  44, .w = 70,  .h = 35, .flags = CTLRA_ITEM_FB_SCREEN},
};
#define FEEDBACK_SIZE (sizeof(feedback_info) / sizeof(feedback_info[0]))

static const char *encoder_names[] = {
	"Enc. Turn",
	"Enc. 1 Turn",
	"Enc. 2 Turn",
	"Enc. 3 Turn",
	"Enc. 4 Turn",
	"Enc. 5 Turn",
	"Enc. 6 Turn",
	"Enc. 7 Turn",
	"Enc. 8 Turn",
};

#define ENCODERS_SIZE (9)

#define CONTROLS_SIZE (BUTTONS_SIZE + ENCODERS_SIZE)

#define LIGHTS_SIZE (80)

#define NPADS                  (16)
/* KERNEL_LENGTH must be a power of 2 for masking */
#define KERNEL_LENGTH          (8)
#define KERNEL_MASK            (KERNEL_LENGTH-1)


/* TODO: Refactor out screen impl, and push to ctlra_ni_screen.h ? */
/* Screen blit commands - no need to have publicly in header */
static const uint8_t header_right[] = {
	0x84,  0x0, 0x01, 0x60,
	0x0,  0x0, 0x0,  0x0,
	0x0,  0x0, 0x0,  0x0,
	0x1, 0xe0, 0x1, 0x10,
};
static const uint8_t header_left[] = {
	0x84,  0x0, 0x00, 0x60,
	0x0,  0x0, 0x0,  0x0,
	0x0,  0x0, 0x0,  0x0,
	0x1, 0xe0, 0x1, 0x10,
};
static const uint8_t command[] = {
	/* num_px/2: 0xff00 is the (total_px/2) */
	0x00, 0x0, 0xff, 0x00,
};
static const uint8_t footer[] = {
	0x03, 0x00, 0x00, 0x00,
	0x40, 0x00, 0x00, 0x00
};
/* 565 encoding, hence 2 bytes per px */
#define NUM_PX (480 * 272)
struct ni_screen_t {
	uint8_t header [sizeof(header_left)];
	uint8_t command[sizeof(command)];
	uint16_t pixels [NUM_PX]; // 565 uses 2 bytes per pixel
	uint8_t footer [sizeof(footer)];
};

/* Represents the the hardware device */
struct ni_maschine_mk3_t {
	/* base handles usb i/o etc */
	struct ctlra_dev_t base;
	/* current value of each controller is stored here */
	float hw_values[CONTROLS_SIZE];
	/* current state of the lights, only flush on dirty */
	uint8_t lights_dirty;

	/* Lights endpoint used to transfer with hidapi */
	uint8_t lights_endpoint;
	uint8_t lights[LIGHTS_SIZE];

	uint8_t lights_pads_endpoint;
	uint8_t lights_pads[LIGHTS_SIZE];
	uint8_t pad_colour;

	/* Store the current encoder value */
	uint8_t encoder_value;
	/* Pressure filtering for note-onset detection */
	uint64_t pad_last_msg_time;
	uint16_t pad_hit;
	uint16_t pad_idx[NPADS];
	uint16_t pad_pressures[NPADS*KERNEL_LENGTH];

	struct ni_screen_t screen_left;
	struct ni_screen_t screen_right;
};

static const char *
ni_maschine_mk3_control_get_name(enum ctlra_event_type_t type,
                                       uint32_t control_id)
{
	if(type == CTLRA_EVENT_BUTTON && control_id < CONTROL_NAMES_SIZE)
		return ni_maschine_mk3_control_names[control_id];
	if(type == CTLRA_EVENT_ENCODER && control_id < ENCODERS_SIZE )
		return encoder_names[control_id];
	return 0;
}

static uint32_t ni_maschine_mk3_poll(struct ctlra_dev_t *base)
{
	struct ni_maschine_mk3_t *dev = (struct ni_maschine_mk3_t *)base;
	uint8_t buf[128];
	ctlra_dev_impl_usb_interrupt_read(base, USB_HANDLE_IDX,
					  USB_ENDPOINT_READ,
					  buf, 128);
	return 0;
}

void
ni_maschine_mk3_light_flush(struct ctlra_dev_t *base, uint32_t force);

#if 0
typedef uint16_t elem_type;

static int compare(const void *f1, const void *f2)
{
	return ( *(elem_type*)f1 > *(elem_type*)f2) ? 1 : -1;
}

static elem_type qsort_median(elem_type * array, int n)
{
	qsort(array, n, sizeof(elem_type), compare);
	return array[n/2];
}
#endif

/* ABCDEFGH Pad colour */
static const uint8_t pad_cols[] = {
	0x2a, 0b11101, 0b11000011, 0x5e,
	0b11011,
	0b1111,
	0b1011,
	0b101,
};

void
ni_maschine_mk3_usb_read_cb(struct ctlra_dev_t *base,
				  uint32_t endpoint, uint8_t *data,
				  uint32_t size)
{
	struct ni_maschine_mk3_t *dev =
		(struct ni_maschine_mk3_t *)base;
	static double worst_poll;
	int32_t nbytes = size;

	int count = 0;

	uint8_t *buf = data;

	switch(nbytes) {
	case 81: {
		/* Return of LED state, after update written to device */
#if 0
		int i;
		static uint8_t old[82];
		for(i = 81; i >= 0; i--) {
			printf("%02x ", buf[i]);
		}
		printf("\n");
#endif
		} break;
	case 128: {
		/* ensure a pad message */
		if(buf[0] != 2) {
			printf("128 but not 2\n");
			break;
		}

		uint64_t now = rdtsc();
		printf("delta between msgs = %ld\n",
		       now - dev->pad_last_msg_time);
		dev->pad_last_msg_time = now;

		/* Template event */
		struct ctlra_event_t event = {
			.type = CTLRA_EVENT_GRID,
			.grid  = {
				.id = 0,
				.flags = CTLRA_EVENT_GRID_FLAG_BUTTON,
				.pos = 0,
				.pressed = 1
			},
		};
		struct ctlra_event_t *e = {&event};

		/* pre-process pressed pads into bitmask */
		uint16_t rpt_pressed = 0;
		int flush_lights = 0;
		for(int i = 0; i < 16; i++) {
			/* skip over pressure values */
			uint8_t p = buf[1+i*3];
			/* pad is zero when list of pads has ended */
			if(i > 0 && p == 0)
				break;
			rpt_pressed |= 1 << p;
		}

		printf("rpt %d\n", rpt_pressed);
#if 0
		{
			int pressure = ((buf[2+i*3] & 0xf) << 8) | buf[3+i*3];
			pad_pressed |= (pressure > 200) << p;

			uint8_t idx = dev->pad_idx[i]++ & KERNEL_MASK;
			uint16_t total = 0;
			for(int j = 0; j < KERNEL_LENGTH; j++) {
				int idx = i*KERNEL_LENGTH + j;
				total += dev->pad_pressures[idx];
			}
			dev->pad_pressures[i*KERNEL_LENGTH + idx] = pressure;

			uint16_t med = qsort_median(&dev->pad_pressures[i*KERNEL_LENGTH],
						    KERNEL_LENGTH);
		}

		for(int i = 0; i < 16; i++) {
			/* already pressed */
			if(dev->pad_hit & (1 << i)) {
				dev->pad_hit &= ~(1 << i);
				dev->lights_pads[25+i] = 0;
				flush_lights = 1;
				continue;
			}

			dev->pad_hit |= (1 << i);

			/* rotate grid to match order on device (but zero
			 * based counting instead of 1 based). */
			event.grid.pos = (3-(i/4))*4 + (i%4);

			event.grid.pressed = ((1 << i)) > 0;
			dev->base.event_func(&dev->base, 1, &e,
					     dev->base.event_func_userdata);
			dev->lights_pads[25+i] = dev->pad_colour * event.grid.pressed;
			flush_lights = 1;
		}
		if(flush_lights)
			ni_maschine_mk3_light_flush(&dev->base, 1);
#endif

#if 1
#else
		/* alternative implementation using bitmasks for loop */
		uint16_t delta = old_pressed ^ pad_pressed;
		const uint32_t count = __builtin_popcount(delta);
		for(int i = 0; i < count; i++) {
			int p = ffs(delta);
			event.grid.pos = p - 1;
			event.grid.pressed = (pad_pressed & (1 << p));
			printf("pop: %d, ffs %d, press = %d, pressed %02x\n",
			       i, p, pad_pressed & (1<<p), pad_pressed);
			dev->base.event_func(&dev->base, 1, &e,
					     dev->base.event_func_userdata);
		}
#endif

	} break;
	case 42: {
		/* Buttons */
		for(uint32_t i = 0; i < BUTTONS_SIZE; i++) {
			int id     = buttons[i].event_id;
			int offset = buttons[i].buf_byte_offset;
			int mask   = buttons[i].mask;

			uint16_t v = *((uint16_t *)&buf[offset]) & mask;
			int value_idx = i;

			if(dev->hw_values[value_idx] != v) {
				dev->hw_values[value_idx] = v;

				struct ctlra_event_t event = {
					.type = CTLRA_EVENT_BUTTON,
					.button  = {
						.id = i,
						.pressed = v > 0
					},
				};
				struct ctlra_event_t *e = {&event};
				dev->base.event_func(&dev->base, 1, &e,
						     dev->base.event_func_userdata);

				if(i > 6 && i <= 6 + 8) {
					dev->pad_colour   = pad_cols[i-7];
					dev->lights[22+i] = pad_cols[i-7];;
					dev->lights_dirty = 1;
				}
			}
		}

		/* 8 float-style endless encoders under screen */
		for(uint32_t i = 0; i < 8; i++) {
			uint16_t v = *((uint16_t *)&buf[12+i*2]);
			const float value = v / 1000.f;
			const uint8_t idx = BUTTONS_SIZE + i;

			if(dev->hw_values[idx] != value) {
				const float d = (value - dev->hw_values[idx]);
				if(fabsf(d) > 0.7) {
					/* wrap around */
					dev->hw_values[idx] = value;
					continue;
				}
				struct ctlra_event_t event = {
					.type = CTLRA_EVENT_ENCODER,
					.encoder  = {
						.id = i + 1,
						.flags =
							CTLRA_EVENT_ENCODER_FLAG_FLOAT,
						.delta_float = d,
					},
				};
				struct ctlra_event_t *e = {&event};
				dev->base.event_func(&dev->base, 1, &e,
						     dev->base.event_func_userdata);
				dev->hw_values[idx] = value;
			}
		}

		/* Main Encoder */
		struct ctlra_event_t event = {
			.type = CTLRA_EVENT_ENCODER,
			.encoder = {
				.id = 0,
				.flags = CTLRA_EVENT_ENCODER_FLAG_INT,
				.delta = 0,
			},
		};
		struct ctlra_event_t *e = {&event};
		int8_t enc   = buf[11] & 0x0f;
		if(enc != dev->encoder_value) {
			int dir = ctlra_dev_encoder_wrap_16(enc, dev->encoder_value);
			event.encoder.delta = dir;
			dev->encoder_value = enc;
			dev->base.event_func(&dev->base, 1, &e,
					     dev->base.event_func_userdata);
		}
		break;
		} /* case 42: buttons */
	}
}

static void ni_maschine_mk3_light_set(struct ctlra_dev_t *base,
                uint32_t light_id,
                uint32_t light_status)
{
	struct ni_maschine_mk3_t *dev = (struct ni_maschine_mk3_t *)base;
	int ret;

	if(!dev)
		return;
	if(light_id > LIGHTS_SIZE) {
		return;
	}

	int idx = light_id;

	uint32_t r = (light_status >> 16) & 0x7F;
	uint32_t g = (light_status >>  8) & 0x7F;
	uint32_t b = (light_status >>  0) & 0x7F;
	uint32_t bright = (light_status >> 27);
	
	switch(idx) {
	case 5: /* Sampling */
		dev->lights_dirty = r;
		break;
	default:
		/* brighness 2 bits at the start of the uint8_t for the light */
		dev->lights[idx] = bright;
		dev->lights_pads[idx] = bright;
		break;
	};

	dev->lights_dirty = 1;
}

void
ni_maschine_mk3_light_flush(struct ctlra_dev_t *base, uint32_t force)
{
	struct ni_maschine_mk3_t *dev = (struct ni_maschine_mk3_t *)base;
	if(!dev->lights_dirty && !force)
		return;

	uint8_t *data = &dev->lights_endpoint;
	dev->lights_endpoint = 0x80;
#if 0
	static uint16_t c;
	for(int i= 0; i < 1; i++) {
		dev->lights[i] = 0x21 | 0b11;//(0b1101 << ((c++ / 4000)%8));
			//0b00100000;
			//0x2a;
	}

	dev->lights[29] = 0b10000000;
	dev->lights[30] = 0b10000001;
	dev->lights[30] = 0b01000001;
	dev->lights[31] = 0b00101000;
	dev->lights[32] = 0b00010011;
	dev->lights[33] = 0b00001011;
	dev->lights[34] = 0b00000101;
	dev->lights[35] = 0b00001101;
	dev->lights[36] = 0b00011101;


	for(int i = 0; i < 4; i++)
		dev->lights[25+i] = 0b10001 << i;
	for(int i = 0; i < 4; i++) {
		dev->lights[37+i] = 0b1100 << i;
		//printf("%d = %02x\n", i, 0b1100 << i);
	}

	dev->lights[29] = 0b10000000;
	dev->lights[30] = 0b10000001;
	dev->lights[30] = 0b01000001;
	dev->lights[31] = 0b00100011;
	dev->lights[32] = 0b00010011;
	dev->lights[33] = 0b00001011;
	dev->lights[34] = 0b00000101;
	dev->lights[35] = 0b00001101;
	dev->lights[36] = 0b00011101;

	for(int i = 0; i < 16; i++)
		dev->lights[25+i] &= dev->pad_hit[i] > 0 ? (uint8_t)-1 : 0;
#endif
	/* error handling in USB subsystem */
	ctlra_dev_impl_usb_interrupt_write(base,
					   USB_HANDLE_IDX,
					   USB_ENDPOINT_WRITE,
					   data,
					   LIGHTS_SIZE + 1);

	data = &dev->lights_pads_endpoint;
	dev->lights_pads_endpoint = 0x81;
	ctlra_dev_impl_usb_interrupt_write(base,
					   USB_HANDLE_IDX,
					   USB_ENDPOINT_WRITE,
					   data,
					   LIGHTS_SIZE + 1);
}

static void
maschine_mk3_blit_to_screen(struct ni_maschine_mk3_t *dev, int scr)
{
	void *data = (scr == 1) ? &dev->screen_right : &dev->screen_left;

	int ret = ctlra_dev_impl_usb_bulk_write(&dev->base,
						USB_HANDLE_SCREEN_IDX,
						USB_ENDPOINT_SCREEN_WRITE,
						data,
						sizeof(dev->screen_left));
	if(ret < 0)
		printf("%s screen write failed!\n", __func__);
}

int32_t
ni_maschine_mk3_screen_get_data(struct ctlra_dev_t *base,
				      uint8_t **pixels,
				      uint32_t *bytes,
				      uint8_t flush)
{
	struct ni_maschine_mk3_t *dev = (struct ni_maschine_mk3_t *)base;

	if(flush)
		maschine_mk3_blit_to_screen(dev, 0);

	*pixels = (uint8_t *)&dev->screen_left.pixels;
	*bytes = NUM_PX * 2;

	return 0;
}

static int32_t
ni_maschine_mk3_disconnect(struct ctlra_dev_t *base)
{
	struct ni_maschine_mk3_t *dev = (struct ni_maschine_mk3_t *)base;

	memset(dev->lights, 0x0, LIGHTS_SIZE);
	if(!base->banished) {
		ni_maschine_mk3_light_flush(base, 1);
		memset(dev->screen_left.pixels, 0x0,
		       sizeof(dev->screen_left.pixels));
		memset(dev->screen_right.pixels, 0x0,
		       sizeof(dev->screen_right.pixels));
		maschine_mk3_blit_to_screen(dev, 0);
		maschine_mk3_blit_to_screen(dev, 1);
	}

	ctlra_dev_impl_usb_close(base);
	free(dev);
	return 0;
}

struct ctlra_dev_info_t ctlra_ni_maschine_mk3_info;

struct ctlra_dev_t *
ctlra_ni_maschine_mk3_connect(ctlra_event_func event_func,
				    void *userdata, void *future)
{
	(void)future;
	struct ni_maschine_mk3_t *dev =
		calloc(1,sizeof(struct ni_maschine_mk3_t));
	if(!dev)
		goto fail;

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

	err = ctlra_dev_impl_usb_open_interface(&dev->base,
	                                        USB_INTERFACE_SCREEN,
	                                        USB_HANDLE_SCREEN_IDX);
	if(err) {
		printf("%s: failed to open screen usb interface\n", __func__);
		goto fail;
	}

	/* initialize blit mem in driver */
	memcpy(dev->screen_left.header , header_left, sizeof(dev->screen_left.header));
	memcpy(dev->screen_left.command, command, sizeof(dev->screen_left.command));
	memcpy(dev->screen_left.footer , footer , sizeof(dev->screen_left.footer));
	/* right */
	memcpy(dev->screen_right.header , header_right, sizeof(dev->screen_right.header));
	memcpy(dev->screen_right.command, command, sizeof(dev->screen_right.command));
	memcpy(dev->screen_right.footer , footer , sizeof(dev->screen_right.footer));

	/* blit stuff to screen */
	uint8_t col_1 = 0b00010000;
	uint8_t col_2 = 0b11000011;
	uint16_t col = (col_2 << 8) | col_1;

	uint16_t *sl = dev->screen_left.pixels;
	uint16_t *sr = dev->screen_right.pixels;

	for(int i = 0; i < NUM_PX; i++) {
		*sl++ = col;
		*sr++ = col;
	}
	maschine_mk3_blit_to_screen(dev, 0);
	maschine_mk3_blit_to_screen(dev, 1);


	dev->pad_colour = pad_cols[0];
	dev->lights_dirty = 1;

	dev->base.info = ctlra_ni_maschine_mk3_info;

	dev->base.usb_read_cb = ni_maschine_mk3_usb_read_cb;

	dev->base.info.control_count[CTLRA_EVENT_BUTTON] =
		CONTROL_NAMES_SIZE - 1; /* -1 is encoder */
	dev->base.info.control_count[CTLRA_EVENT_ENCODER] = 1;
	dev->base.info.control_count[CTLRA_EVENT_GRID] = 1;
	dev->base.info.get_name = ni_maschine_mk3_control_get_name;

	struct ctlra_grid_info_t *grid = &dev->base.info.grid_info[0];
	grid->rgb = 1;
	grid->velocity = 1;
	grid->pressure = 1;
	grid->x = 4;
	grid->y = 4;

	dev->base.info.vendor_id = CTLRA_DRIVER_VENDOR;
	dev->base.info.device_id = CTLRA_DRIVER_DEVICE;

	dev->base.poll = ni_maschine_mk3_poll;
	dev->base.disconnect = ni_maschine_mk3_disconnect;
	dev->base.light_set = ni_maschine_mk3_light_set;
	dev->base.light_flush = ni_maschine_mk3_light_flush;
	dev->base.screen_get_data = ni_maschine_mk3_screen_get_data;

	dev->base.event_func = event_func;
	dev->base.event_func_userdata = userdata;

	return (struct ctlra_dev_t *)dev;
fail:
	free(dev);
	return 0;
}

struct ctlra_dev_info_t ctlra_ni_maschine_mk3_info = {
	.vendor    = "Native Instruments",
	.device    = "Maschine Mk3",
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
			.params[0] = 0,
			/* end light id */
			.params[1] = 1,
		}
	},

	.get_name = ni_maschine_mk3_control_get_name,
};

CTLRA_DEVICE_REGISTER(ni_maschine_mk3)
