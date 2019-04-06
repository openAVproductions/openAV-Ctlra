/*
 * Copyright (c) 2019, OpenAV Productions,
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
#include <math.h>
#include <time.h>
#include <sys/time.h>

#include "impl.h"

#define CTLRA_DRIVER_VENDOR (0x17cc)
#define CTLRA_DRIVER_DEVICE (0x1420)

#define USB_INTERFACE_HID     (0x03)
#define USB_HANDLE_IDX        (0x00)
#define USB_ENDPOINT_READ     (0x83)
#define USB_ENDPOINT_WRITE    (0x02)

#define USB_HANDLE_SCREEN_IDX     (0x1)
#define USB_INTERFACE_SCREEN      (0x4)
#define USB_ENDPOINT_SCREEN_WRITE (0x3)

#define LIGHTS_SIZE (125 + 1)
/* 25 + 16 bytes enough, but padding required for USB message */
#define LIGHTS_PADS_SIZE (80)


/* TODO: Refactor out screen impl, and push to ctlra_ni_screen.h ? */
/* Screen blit commands - no need to have publicly in header */
static const uint8_t header_right[] = {
	0x84, 0x0, 0x1, 0x60,
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
	//uint8_t endpoint; // NOPE: but makes both blink!
	uint8_t header [sizeof(header_left)];
	uint8_t command[sizeof(command)];
	uint16_t pixels [NUM_PX]; // 565 uses 2 bytes per pixel
	uint8_t footer [sizeof(footer)];
};

/* Represents the the hardware device */
struct ni_kontrol_s5_t {
	/* base handles usb i/o etc */
	struct ctlra_dev_t base;
	/* current value of each controller is stored here */
	// TODO: fixme hard coded value, reduce to num hw values in use
	float hw_values[512];
	/* current state of the lights, only flush on dirty */
	uint8_t lights_dirty;
	uint8_t lights_pads_dirty;

	/* Lights endpoint used to transfer with hidapi */
	uint8_t lights_endpoint;
	uint8_t lights[LIGHTS_SIZE];

	uint8_t lights_pads_endpoint;
	uint8_t lights_pads[LIGHTS_PADS_SIZE];

	/* state of the pedal, according to the hardware */
	uint8_t pedal;

	uint8_t encoder_value;
	uint16_t touchstrip_value;

	struct ni_screen_t screen_left;
	struct ni_screen_t screen_right;

	uint64_t canary_post;
};

static const char *
ni_kontrol_s5_control_get_name(enum ctlra_event_type_t type,
                                       uint32_t control_id)
{
	return "Not Implemented";
}

static uint32_t ni_kontrol_s5_poll(struct ctlra_dev_t *base)
{
	struct ni_kontrol_s5_t *dev = (struct ni_kontrol_s5_t *)base;
	uint8_t buf[128];
	ctlra_dev_impl_usb_interrupt_read(base, USB_HANDLE_IDX,
					  USB_ENDPOINT_READ,
					  buf, 128);
	return 0;
}

void
ni_kontrol_s5_light_flush(struct ctlra_dev_t *base, uint32_t force);

void
ni_kontrol_s5_usb_read_cb(struct ctlra_dev_t *base,
				  uint32_t endpoint, uint8_t *data,
				  uint32_t size)
{
	struct ni_kontrol_s5_t *dev =
		(struct ni_kontrol_s5_t *)base;
	static double worst_poll;
	int32_t nbytes = size;

	int count = 0;

	uint8_t *buf = data;

	switch(size) {
	case 30: printf("Button/Enc-turn/Touchstrip\n"); break;
	case 79: printf("Slider/Knob\n"); break;
	};

#if 0
	if (size < 1000)
		printf("USB READ size %d (ep %x)\n", size, endpoint);

	// screen reply - print data
	if(0 && size == 261148) {
		printf("\t");
		for(int i = 0; i < 8; i++)
			printf("%02x ", data[i]);
		printf("\n");
	}
#endif

	return;
}
static void ni_kontrol_s5_light_set(struct ctlra_dev_t *base,
                uint32_t light_id,
                uint32_t light_status)
{
	struct ni_kontrol_s5_t *dev = (struct ni_kontrol_s5_t *)base;
	int ret;

	if(!dev)
		return;

	// TODO: debug the -1, why is it required to get the right size?
	if(light_id > LIGHTS_SIZE)
		return;

	int idx = light_id;

	dev->lights[idx] = light_status;

	return;
}

void
ni_kontrol_s5_light_flush(struct ctlra_dev_t *base, uint32_t force)
{
	struct ni_kontrol_s5_t *dev = (struct ni_kontrol_s5_t *)base;
	if(!dev->lights_dirty && !force)
		return;

	uint32_t ut = 10;

	/* 0x80 == left
	 * 0x81 == right
	 * 0x82 == mixer */
	uint8_t *data = &dev->lights_endpoint;
	dev->lights_endpoint = 0x80;

	/* error handling in USB subsystem */
	ctlra_dev_impl_usb_interrupt_write(base,
					   USB_HANDLE_IDX,
					   USB_ENDPOINT_WRITE,
					   data,
					   LIGHTS_SIZE+1);

	data = &dev->lights_endpoint;
	dev->lights_endpoint = 0x81;
	ctlra_dev_impl_usb_interrupt_write(base,
					   USB_HANDLE_IDX,
					   USB_ENDPOINT_WRITE,
					   data,
					   LIGHTS_SIZE+1);

	data = &dev->lights_endpoint;
	dev->lights_endpoint = 0x82;
	ctlra_dev_impl_usb_interrupt_write(base,
					   USB_HANDLE_IDX,
					   USB_ENDPOINT_WRITE,
					   data,
					   LIGHTS_SIZE+1);

#if 0
	data = &dev->lights_endpoint;
	dev->lights_endpoint = 0x82;
	ctlra_dev_impl_usb_interrupt_write(base,
					   USB_HANDLE_IDX,
					   USB_ENDPOINT_WRITE,
					   data,
					   64);
	usleep(ut);

	/* dummy write */
	ctlra_dev_impl_usb_interrupt_write(base,
					   USB_HANDLE_IDX,
					   USB_ENDPOINT_WRITE,
					   data,
					   64);
#endif
}

static void
kontrol_s5_blit_to_screen(struct ni_kontrol_s5_t *dev, int scr)
{
	void *data = (scr == 1) ? &dev->screen_right : &dev->screen_left;

#if 0
	printf("blit to screen %d\n", scr);
	uint8_t *p = data;
	for(int i = 0; i < 8; i++)
		printf("%02x ", p[i]);
	printf("\n");
#endif

	int ret = ctlra_dev_impl_usb_bulk_write(&dev->base,
						USB_HANDLE_SCREEN_IDX,
						USB_ENDPOINT_SCREEN_WRITE,
						data,
						sizeof(dev->screen_left));
	if(ret < 0)
		printf("%s screen %d write failed!\n", __func__,  scr);
}

int32_t
ni_kontrol_s5_screen_get_data(struct ctlra_dev_t *base,
				uint32_t screen_idx,
				uint8_t **pixels,
				uint32_t *bytes,
				struct ctlra_screen_zone_t *zone,
				uint8_t flush)
{
	struct ni_kontrol_s5_t *dev = (struct ni_kontrol_s5_t *)base;

	if(screen_idx > 1)
		return -1;

	if(flush == 1) {
		kontrol_s5_blit_to_screen(dev, screen_idx);
		return 0;
	}

	*pixels = (uint8_t *)&dev->screen_left.pixels;
	if(screen_idx == 1)
		*pixels = (uint8_t *)&dev->screen_right.pixels;

	*bytes = NUM_PX * 2;

	return 0;
}

static int32_t
ni_kontrol_s5_disconnect(struct ctlra_dev_t *base)
{
	struct ni_kontrol_s5_t *dev = (struct ni_kontrol_s5_t *)base;

	memset(dev->lights, 0x0, LIGHTS_SIZE);
	dev->lights_dirty = 1;
	memset(dev->lights_pads, 0x0, LIGHTS_PADS_SIZE);
	dev->lights_pads_dirty = 1;

	if(!base->banished) {

		ni_kontrol_s5_light_flush(base, 1);
		memset(dev->screen_left.pixels, 0x0,
		       sizeof(dev->screen_left.pixels));
		memset(dev->screen_right.pixels, 0x0,
		       sizeof(dev->screen_right.pixels));
		kontrol_s5_blit_to_screen(dev, 0);
		kontrol_s5_blit_to_screen(dev, 1);
	}

	ctlra_dev_impl_usb_close(base);
	free(dev);
	return 0;
}

struct ctlra_dev_info_t ctlra_ni_kontrol_s5_info;

struct ctlra_dev_t *
ctlra_ni_kontrol_s5_connect(ctlra_event_func event_func,
				    void *userdata, void *future)
{
	(void)future;
	struct ni_kontrol_s5_t *dev =
		calloc(1,sizeof(struct ni_kontrol_s5_t));
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
						USB_INTERFACE_HID,
						USB_HANDLE_IDX);
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

	/* turn on all lights at startup */
	uint8_t brightness = 0xff;
	memset(dev->lights, brightness, LIGHTS_SIZE);
	dev->lights_dirty = 1;
	ni_kontrol_s5_light_flush(&dev->base, 1);

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
	kontrol_s5_blit_to_screen(dev, 0);
	kontrol_s5_blit_to_screen(dev, 1);

	dev->lights_dirty = 1;

	dev->base.info = ctlra_ni_kontrol_s5_info;

	dev->base.poll = ni_kontrol_s5_poll;
	dev->base.usb_read_cb = ni_kontrol_s5_usb_read_cb;
	dev->base.disconnect = ni_kontrol_s5_disconnect;
	dev->base.light_set = ni_kontrol_s5_light_set;
	dev->base.light_flush = ni_kontrol_s5_light_flush;
	dev->base.screen_get_data = ni_kontrol_s5_screen_get_data;

	dev->base.event_func = event_func;
	dev->base.event_func_userdata = userdata;

	return (struct ctlra_dev_t *)dev;
fail:
	free(dev);
	return 0;
}

struct ctlra_dev_info_t ctlra_ni_kontrol_s5_info = {
	.vendor    = "Native Instruments",
	.device    = "Kontrol S5",
	.vendor_id = CTLRA_DRIVER_VENDOR,
	.device_id = CTLRA_DRIVER_DEVICE,

	.get_name = ni_kontrol_s5_control_get_name,
};

CTLRA_DEVICE_REGISTER(ni_kontrol_s5)
