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

#include "impl.h"

#define CTLRA_DRIVER_VENDOR (0x256f)
#define CTLRA_DRIVER_DEVICE (0xc632)
#define USB_INTERFACE_ID   (0x00)
#define USB_HANDLE_IDX     (0x0)
#define USB_ENDPOINT_READ  (0x81)

/* 6-dof movement broken down into 12 sliders */
static const char *spacemouse_names_sliders[] = {
	"Move Left",
	"Move Right",
	"Move Foward",
	"Move Back",
	"Move Up",
	"Move Down",
	"Rotate Forward",
	"Rotate Backward",
	"Rotate Right",
	"Rotate Left",
	"Rotate Anti-Clockwise",
	"Rotate Clockwise",
};
#define CONTROL_NAMES_SLIDERS_SIZE (sizeof(spacemouse_names_sliders) /\
				    sizeof(spacemouse_names_sliders[0]))

static const char *spacemouse_names_buttons[] = {
	"Menu",
	/* left keys */
	"Alt",
	"Ctrl",
	"Shift",
	"Esc",
	/* numbers */
	"1",
	"2",
	"3",
	"4",
	/* right keys */
	"Quick [ ]",
	"Quick [T]",
	"Quick [R]",
	"Quick [F]",
	"Quick Middle",
	"Fit"
};
#define CONTROL_NAMES_BUTTONS_SIZE (sizeof(spacemouse_names_buttons) /\
				    sizeof(spacemouse_names_buttons[0]))

/* This struct is a generic struct to identify hw controls */
struct spacemouse_ev_t {
	uint8_t event_id;
	uint8_t offset;
	uint8_t mask;
};
static const struct spacemouse_ev_t buttons[] = {
	{ 0, 1, 0x01}, /* menu */
	{ 1, 3, 0x80}, /* alt */
	{ 2, 4, 0x02}, /* ctrl */
	{ 3, 4, 0x01}, /* shift */
	{ 4, 3, 0x40}, /* esc */
	{ 5, 2, 0x10}, /* 1 */
	{ 6, 2, 0x20}, /* 2 */
	{ 7, 2, 0x40}, /* 3 */
	{ 8, 2, 0x80}, /* 4 */
	{ 9, 2, 0x01}, /* Quick [ ] */
	{10, 1, 0x04}, /* Quick [T] */
	{11, 1, 0x10}, /* Quick [R] */
	{12, 1, 0x20}, /* Quick [F] */
	{13, 4, 0x04}, /* Quick Middle */
	{14, 1, 0x02}, /* Fit */
};
#define BUTTONS_SIZE (sizeof(buttons) / sizeof(buttons[0]))

#define CONTROLS_SIZE (6)

/* Represents the the hardware device */
struct spacemouse_t {
	struct ctlra_dev_t base;
	int16_t hw_values[CONTROLS_SIZE];
	uint8_t buttons[BUTTONS_SIZE];
};

static const char *
spacemouse_control_get_name(enum ctlra_event_type_t type,
			    uint32_t control)
{
	switch(type) {
	case CTLRA_EVENT_SLIDER:
		if(control >= CONTROL_NAMES_SLIDERS_SIZE)
			return 0;
		return spacemouse_names_sliders[control];
	case CTLRA_EVENT_BUTTON:
		if(control >= CONTROL_NAMES_BUTTONS_SIZE)
			return 0;
		return spacemouse_names_buttons[control];
	default:
		break;
	}
	return 0;
}

static uint32_t
spacemouse_poll(struct ctlra_dev_t *base)
{
	struct spacemouse_t *dev = (struct spacemouse_t *)base;
#define BUF_SIZE 32
	uint8_t buf[BUF_SIZE];
	ctlra_dev_impl_usb_interrupt_read(base, USB_HANDLE_IDX,
					  USB_ENDPOINT_READ, buf,
					  BUF_SIZE);
	return 0;
}

void
spacemouse_usb_read_cb(struct ctlra_dev_t *base, uint32_t endpoint,
		       uint8_t *data, uint32_t size)
{
	struct spacemouse_t *dev = (struct spacemouse_t *)base;
	uint8_t *buf = data;

	switch(size) {
	case 7:
		for(int i = 0; i < BUTTONS_SIZE; i++) {
			int p = buf[buttons[i].offset] & buttons[i].mask;
			if(p == dev->buttons[i])
				continue;
			dev->buttons[i] = p;

			struct ctlra_event_t event = {
				.type = CTLRA_EVENT_BUTTON,
				.button  = {
					.id = i,
					.pressed = p
				},
			};
			struct ctlra_event_t *e = {&event};
			dev->base.event_func(&dev->base, 1, &e,
					     dev->base.event_func_userdata);
		}
		break;
	case 13:
		for(int i = 0; i < CONTROLS_SIZE; i++) {
			int off = i * 2;
			int16_t v = buf[off + 1] | (buf[off + 2] << 8);
			if(v == dev->hw_values[i])
				continue;

			dev->hw_values[i] = v;
			int neg = v < 0;
			if(neg)
				v *= -1;

			struct ctlra_event_t event = {
				.type = CTLRA_EVENT_SLIDER,
				.slider = {
					.id = (off - neg) + 1,
					.value = v / 350.f},
			};
			struct ctlra_event_t *e = {&event};
			dev->base.event_func(&dev->base, 1, &e,
					     dev->base.event_func_userdata);
		}
		break;
	default: break;
	}
}

static void
spacemouse_light_set(struct ctlra_dev_t *base,
				 uint32_t light_id,
				 uint32_t light_status)
{
	struct spacemouse_t *dev = (struct spacemouse_t *)base;
	(void)dev;
	(void)light_id;
	(void)light_status;
}

static void
spacemouse_light_flush(struct ctlra_dev_t *base, uint32_t force)
{
	struct spacemouse_t *dev = (struct spacemouse_t *)base;
	(void)dev;
	(void)force;
}

static int32_t
spacemouse_disconnect(struct ctlra_dev_t *base)
{
	struct spacemouse_t *dev = (struct spacemouse_t *)base;
	ctlra_dev_impl_usb_close(base);
	free(dev);
	return 0;
}

struct ctlra_dev_info_t ctlra_spacemouse_info;

struct ctlra_dev_t *
ctlra_spacemouse_connect(ctlra_event_func event_func, void *userdata,
			    void *future)
{
	(void)future;
	struct spacemouse_t *dev = calloc(1, sizeof(struct spacemouse_t));
	if(!dev)
		goto fail;

	dev->base.info.control_count[CTLRA_EVENT_SLIDER] = 6;
	dev->base.info.control_count[CTLRA_EVENT_BUTTON] = BUTTONS_SIZE;
	dev->base.info.get_name = spacemouse_control_get_name;

	int err = ctlra_dev_impl_usb_open(&dev->base,
					  CTLRA_DRIVER_VENDOR,
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

	dev->base.info = ctlra_spacemouse_info;
	dev->base.poll = spacemouse_poll;
	dev->base.disconnect = spacemouse_disconnect;
	dev->base.light_set = spacemouse_light_set;
	dev->base.light_flush = spacemouse_light_flush;
	dev->base.usb_read_cb = spacemouse_usb_read_cb;

	dev->base.event_func = event_func;
	dev->base.event_func_userdata = userdata;

	return (struct ctlra_dev_t *)dev;
fail:
	free(dev);
	return 0;
}

struct ctlra_dev_info_t ctlra_spacemouse_info = {
	.vendor    = "3DConnexion",
	.device    = "SpaceMouse Pro",
	.vendor_id = CTLRA_DRIVER_VENDOR,
	.device_id = CTLRA_DRIVER_DEVICE,
	.size_x    = 120,
	.size_y    = 294,

	.get_name = spacemouse_control_get_name,
};

CTLRA_DEVICE_REGISTER(spacemouse)
