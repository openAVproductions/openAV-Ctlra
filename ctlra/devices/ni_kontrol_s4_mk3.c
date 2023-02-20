/*
 * Copyright (c) 2023, OpenAV Productions,
 * Antoine Colombier <ctlra@acolombier.dev>
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
#include <math.h>
#include <time.h>
#include <sys/time.h>

#include "impl.h"

#define USE_SCREEN_ONLY true

#define CTLRA_DRIVER_VENDOR (0x17cc)
#define CTLRA_DRIVER_DEVICE (0x1720)

#define USB_INTERFACE_ID      (0x03)
#define USB_HANDLE_IDX        (0x00)
#define USB_ENDPOINT_READ     (0x83)
#define USB_ENDPOINT_WRITE    (0x03)

#define USB_HANDLE_SCREEN_IDX     (0x1)
#define USB_INTERFACE_SCREEN      (0x4)
#define USB_ENDPOINT_SCREEN_WRITE (0x3)

/* This struct is a generic struct to identify hw controls */
struct ni_kontrol_s4_mk3_ctlra_t {
	int event_id;
	int buf_byte_offset;
	uint32_t mask;
};

static const char *ni_kontrol_s4_mk3_control_names[] = {
};
#define CONTROL_NAMES_SIZE (sizeof(ni_kontrol_s4_mk3_control_names) /\
			    sizeof(ni_kontrol_s4_mk3_control_names[0]))

static const struct ni_kontrol_s4_mk3_ctlra_t buttons[] = {
};
#define BUTTONS_SIZE (sizeof(buttons) / sizeof(buttons[0]))

#define MK3_BTN (CTLRA_ITEM_BUTTON | CTLRA_ITEM_LED_INTENSITY | CTLRA_ITEM_HAS_FB_ID)
#define MK3_BTN_NOFB (CTLRA_ITEM_BUTTON | CTLRA_ITEM_LED_INTENSITY)

static struct ctlra_item_info_t buttons_info[] = {
};

static struct ctlra_item_info_t sliders_info[] = {
};

#define SLIDERS_SIZE (1)

static struct ctlra_item_info_t feedback_info[] = {
	/* Screen */
	{.x =  88, .y =  33, .w = 94,  .h = 54, .flags = CTLRA_ITEM_FB_SCREEN,
		.params[0] = 320, .params[1] = 240},
	{.x = 200, .y =  33, .w = 94,  .h = 54, .flags = CTLRA_ITEM_FB_SCREEN,
		.params[0] = 320, .params[1] = 240},
};
#define FEEDBACK_SIZE (sizeof(feedback_info) / sizeof(feedback_info[0]))

static const char *encoder_names[] = {
};

static struct ctlra_item_info_t encoder_info[] = {
};

#define ENCODERS_SIZE (sizeof(encoder_info) / sizeof(encoder_info[0]))

#define CONTROLS_SIZE (BUTTONS_SIZE + ENCODERS_SIZE)

enum ni_kontrol_s4_mk3_led_color_t {
    OFF		= 0, 
    RED		= 4, 
    CARROT	= 8, 
    ORANGE	= 12,
    HONEY	= 16,
    YELLOW	= 20,
    LIME	= 24,
    GREEN	= 28,
    AQUA	= 32,
    CELESTE	= 36,
    SKY		= 40,
    BLUE	= 44,
    PURPLE	= 48,
    FUSCIA	= 52,
    MAGENTA	= 56,
    AZALEA	= 60,
    SALMON	= 64,
    WHITE	= 68,
};

#define LED_COLOR_ENUM_MASK 0x7e

#define LIGHTS_SIZE (0)

/* KERNEL_LENGTH must be a power of 2 for masking */
#define KERNEL_LENGTH          (8)
#define KERNEL_MASK            (KERNEL_LENGTH-1)


/* TODO: Refactor out screen impl, and push to ctlra_ni_screen.h ? */
/* Screen blit commands - no need to have publicly in header */
static const uint8_t header_right[] = {
	0x84, 	0x0, 	0x01,	0x21, 
	0x0,	0x0, 	0x0,	0x0, 
	0x0, 	0x0, 	0x0, 	0x0, 
	0x1, 	0x40, 	0x0, 	0xf0,
};
static const uint8_t header_left[] = {
	0x84, 	0x0, 	0x0,	0x21, 
	0x0,	0x0, 	0x0,	0x0, 
	0x0, 	0x0, 	0x0, 	0x0, 
	0x1, 	0x40, 	0x0, 	0xf0,
};
static const uint8_t footer[] = {
	0x40, 0x00, 0x00, 0x00
};
/* 565 encoding, hence 2 bytes per px */
#define NUM_PX (320 * 240)
struct ni_screen_t {
	uint8_t header [sizeof(header_left)];
	uint16_t pixels [NUM_PX]; // 565 uses 2 bytes per pixel
	uint8_t footer [sizeof(footer)];
};

/* Represents the the hardware device */
struct ni_kontrol_s4_mk3_t {
	/* base handles usb i/o etc */
	struct ctlra_dev_t base;
	/* current value of each controller is stored here */
	// AG: These appear to spit out some random values initially, which we
	// just record on the first run without passing them on. Same applies
	// to the big encoder and the touchstrip below.
	float hw_values[CONTROLS_SIZE], hw_init[CONTROLS_SIZE];
	/* current state of the lights, only flush on dirty */
	uint8_t lights_dirty;

	/* Lights endpoint used to transfer with hidapi */
	uint8_t lights_endpoint;
	uint8_t lights[LIGHTS_SIZE];

	uint8_t encoder_value, encoder_init;

	struct ni_screen_t screen_left;
	struct ni_screen_t screen_right;
};

static const char *
ni_kontrol_s4_mk3_control_get_name(enum ctlra_event_type_t type,
                                       uint32_t control_id)
{
	if(type == CTLRA_EVENT_BUTTON && control_id < CONTROL_NAMES_SIZE)
		return ni_kontrol_s4_mk3_control_names[control_id];
	if(type == CTLRA_EVENT_ENCODER && control_id < ENCODERS_SIZE)
		return encoder_names[control_id];
	return 0;
}

static uint32_t ni_kontrol_s4_mk3_poll(struct ctlra_dev_t *base)
{
#ifndef USE_SCREEN_ONLY
	struct ni_kontrol_s4_mk3_t *dev = (struct ni_kontrol_s4_mk3_t *)base;
	uint8_t buf[128];
	return ctlra_dev_impl_usb_interrupt_read(base, USB_HANDLE_IDX,
					  USB_ENDPOINT_READ,
					  buf, 128);
#else
	return 0;
#endif
}

void
ni_kontrol_s4_mk3_light_flush(struct ctlra_dev_t *base, uint32_t force);

void
ni_kontrol_s4_mk3_usb_read_cb(struct ctlra_dev_t *base,
				  uint32_t endpoint, uint8_t *data,
				  uint32_t size)
{
	struct ni_kontrol_s4_mk3_t *dev = (struct ni_kontrol_s4_mk3_t *)base;
	int32_t nbytes = size;

	if (nbytes == 0){
		printf("Empty USB packet received.Ignoring.\n");
		return;
	}

	if (nbytes > 4096){
		// Ignoring large packet as they are likely not HID updates
		return;
	}

	int count = 0;

	uint8_t *buf = data;
	uint8_t report_id = data[0];

#ifdef DEBUG_USB_INPUT
	printf("Received USB packet of %d bytes. Report ID is %d\n", nbytes, report_id);
	for (int i = 0; i < nbytes; i++)
	{
		printf("%02X ", buf[i]);
	}
	printf("\n");
#endif

	switch(report_id) {
		case 0x01: // Most components
			break;
		case 0x02: // Mixer component, except effects
			break;
		case 0x03: // Wheels
			break;
	}
}

static void ni_kontrol_s4_mk3_light_set(struct ctlra_dev_t *base,
                uint32_t light_id,
                uint32_t light_status)
{
	struct ni_kontrol_s4_mk3_t *dev = (struct ni_kontrol_s4_mk3_t *)base;
	int ret;

	if (!dev || light_id >= LIGHTS_SIZE || (light_status ^ LED_COLOR_ENUM_MASK) != 0){
		return;
	}
	
	/* normal LEDs */
	switch (light_id){
		default:
		dev->lights[light_id] = light_status;
		break;
	};
	dev->lights_dirty = 1;
}

void
ni_kontrol_s4_mk3_light_flush(struct ctlra_dev_t *base, uint32_t force)
{
#ifndef USE_SCREEN_ONLY
	struct ni_kontrol_s4_mk3_t *dev = (struct ni_kontrol_s4_mk3_t *)base;
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
#endif
}

static int
kontrol_s4_mk3_blit_to_screen(struct ni_kontrol_s4_mk3_t *dev, int scr)
{
	void *data = (scr == 1) ? &dev->screen_right : &dev->screen_left;

	int ret = ctlra_dev_impl_usb_bulk_write(&dev->base,
						USB_HANDLE_SCREEN_IDX,
						USB_ENDPOINT_SCREEN_WRITE,
						data,
						sizeof(dev->screen_left));
	if(ret < 0){
		printf("%s screen write failed!\n", __func__);
		return -1;
	}
	return 0;
}

int32_t
ni_kontrol_s4_mk3_screen_get_data(struct ctlra_dev_t *base,
				uint32_t screen_idx,
				uint8_t **pixels,
				uint32_t *bytes,
				struct ctlra_screen_zone_t *zone,
				uint8_t flush)
{
	struct ni_kontrol_s4_mk3_t *dev = (struct ni_kontrol_s4_mk3_t *)base;

	if(screen_idx > 1)
		return -1;

	if(flush && flush != 1) {
		if (flush != 3){
			fprintf(stderr, "%s: Only full redraw is supported at the moment.\n", __func__);
		}
		flush = 1;
	}


	if(flush == 1) {
		kontrol_s4_mk3_blit_to_screen(dev, screen_idx);
		return 0;
	}

	*pixels = (uint8_t *)&dev->screen_left.pixels;
	if(screen_idx == 1)
		*pixels = (uint8_t *)&dev->screen_right.pixels;

	*bytes = NUM_PX * 2;

	return 0;
}

static int32_t
ni_kontrol_s4_mk3_disconnect(struct ctlra_dev_t *base)
{
	struct ni_kontrol_s4_mk3_t *dev = (struct ni_kontrol_s4_mk3_t *)base;

	memset(dev->lights, 0x0, LIGHTS_SIZE);
	dev->lights_dirty = 1;

	if(!base->banished) {
		ni_kontrol_s4_mk3_light_flush(base, 1);
		memset(dev->screen_left.pixels, 0x0,
		       sizeof(dev->screen_left.pixels));
		memset(dev->screen_right.pixels, 0x0,
		       sizeof(dev->screen_right.pixels));
		kontrol_s4_mk3_blit_to_screen(dev, 0);
		kontrol_s4_mk3_blit_to_screen(dev, 1);
	}

	ctlra_dev_impl_usb_close(base);
	free(dev);
	return 0;
}

struct ctlra_dev_info_t ctlra_ni_kontrol_s4_mk3_info;

struct ctlra_dev_t *
ctlra_ni_kontrol_s4_mk3_connect(ctlra_event_func event_func,
				    void *userdata, void *future)
{
	(void)future;
	struct ni_kontrol_s4_mk3_t *dev =
		calloc(1,sizeof(struct ni_kontrol_s4_mk3_t));
	if(!dev)
		goto fail;

	int err = ctlra_dev_impl_usb_open(&dev->base,
					  CTLRA_DRIVER_VENDOR,
					  CTLRA_DRIVER_DEVICE);
	if(err) {
		free(dev);
		return 0;
	}

#ifndef USE_SCREEN_ONLY
	err = ctlra_dev_impl_usb_open_interface(&dev->base,
					 USB_INTERFACE_ID, USB_HANDLE_IDX);
	if(err) {
		printf("error opening interface\n");
		free(dev);
		return 0;
	}
#endif

	err = ctlra_dev_impl_usb_open_interface(&dev->base,
	                                        USB_INTERFACE_SCREEN,
	                                        USB_HANDLE_SCREEN_IDX);
	if(err) {
		printf("%s: failed to open screen usb interface\n", __func__);
		goto fail;
	}

	/* initialize blit mem in driver */
	memcpy(dev->screen_left.header , header_left, sizeof(dev->screen_left.header));
	memcpy(dev->screen_left.footer , footer , sizeof(dev->screen_left.footer));
	/* right */
	memcpy(dev->screen_right.header , header_right, sizeof(dev->screen_right.header));
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
	if (kontrol_s4_mk3_blit_to_screen(dev, 0) < 0){
		goto fail;
	}
	if (kontrol_s4_mk3_blit_to_screen(dev, 1) < 0){
		goto fail;
	}

	dev->lights_dirty = 1;

	dev->base.info = ctlra_ni_kontrol_s4_mk3_info;

	dev->base.poll = ni_kontrol_s4_mk3_poll;
	dev->base.usb_read_cb = ni_kontrol_s4_mk3_usb_read_cb;
	dev->base.disconnect = ni_kontrol_s4_mk3_disconnect;
	dev->base.light_set = ni_kontrol_s4_mk3_light_set;
	dev->base.light_flush = ni_kontrol_s4_mk3_light_flush;
	dev->base.screen_get_data = ni_kontrol_s4_mk3_screen_get_data;

	dev->base.event_func = event_func;
	dev->base.event_func_userdata = userdata;

	return (struct ctlra_dev_t *)dev;
fail:
	free(dev);
	return 0;
}

struct ctlra_dev_info_t ctlra_ni_kontrol_s4_mk3_info = {
	.vendor    = "Native Instruments",
	.device    = "Kontrol S4 Mk3",
	.vendor_id = CTLRA_DRIVER_VENDOR,
	.device_id = CTLRA_DRIVER_DEVICE,
	.size_x    = 320,
	.size_y    = 240,

	.control_count[CTLRA_EVENT_BUTTON] = BUTTONS_SIZE,
	.control_info [CTLRA_EVENT_BUTTON] = buttons_info,

	.control_count[CTLRA_FEEDBACK_ITEM] = FEEDBACK_SIZE,
	.control_info [CTLRA_FEEDBACK_ITEM] = feedback_info,

	.control_count[CTLRA_EVENT_SLIDER] = SLIDERS_SIZE,
	.control_info [CTLRA_EVENT_SLIDER] = sliders_info,

	.control_count[CTLRA_EVENT_ENCODER] = ENCODERS_SIZE,
	.control_info [CTLRA_EVENT_ENCODER] = encoder_info,

#if 1
	.control_count[CTLRA_EVENT_GRID] = 0,
#endif

	.get_name = ni_kontrol_s4_mk3_control_get_name,
};

CTLRA_DEVICE_REGISTER(ni_kontrol_s4_mk3)
