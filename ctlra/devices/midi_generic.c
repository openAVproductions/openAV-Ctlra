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
#include "midi.h"

#define CONTROLS_SIZE 512
#define LIGHTS_SIZE 512

#define GENERIC 0x1

#define CTLRA_DRIVER_VENDOR GENERIC
#define CTLRA_DRIVER_DEVICE GENERIC

/* Represents the the hardware device */
struct midi_generic_t {
	/* base handles usb i/o etc */
	struct ctlra_dev_t base;
	/* midi i/o */
	struct ctlra_midi_t *midi;
};

static uint32_t
midi_generic_poll(struct ctlra_dev_t *base)
{
	struct midi_generic_t *dev = (struct midi_generic_t *)base;
	ctlra_midi_input_poll(dev->midi);
	return 0;
}

int
midi_generic_midi_input_cb(uint8_t nbytes, uint8_t * buf, void *ud)
{
	struct midi_generic_t *dev = (struct midi_generic_t *)ud;

	switch(buf[0] & 0xf0) {
	case 0x90: /* Note On */
	case 0x80: /* Note Off */ {
		struct ctlra_event_t event = {
			.type = CTLRA_EVENT_BUTTON,
			.button  = {
				.id = buf[1],
				.pressed = buf[0] >= 0x90,
				.has_pressure = 1,
				.pressure = buf[2] / 127.f,
			},
		};
		struct ctlra_event_t *e = {&event};
		dev->base.event_func(&dev->base, 1, &e,
				     dev->base.event_func_userdata);
		} break;

	case 0xb0: /* control change */ {
		struct ctlra_event_t event = {
			.type = CTLRA_EVENT_SLIDER,
			.slider  = {
				.id = buf[1],
				.value = buf[2] / 127.f
			},
		};
		struct ctlra_event_t *e = {&event};
		dev->base.event_func(&dev->base, 1, &e,
				     dev->base.event_func_userdata);
		}
		break;
	};

	return 0;
}

static const char *
midi_generic_control_get_name(enum ctlra_event_type_t type,
			      uint32_t control)
{
	return "generic";
}

static void midi_generic_light_set(struct ctlra_dev_t *base,
				    uint32_t light_id,
				    uint32_t light_status)
{
	struct midi_generic_t *dev = (struct midi_generic_t *)base;

	uint8_t out[3];

	if(light_id == -1) {
		out[2] = (light_status >> 16) & 0xff;
		out[1] = (light_status >>  8) & 0xff;
		out[0] = (light_status >>  0) & 0xff;
	} else {
		uint8_t b3 = 0;
		b3 |= light_status >> 24;
		b3 |= light_status >> 16;
		b3 |= light_status >>  8;

		out[0] = 0x90;
		out[1] = light_id;
		out[2] = b3;
	}

	ctlra_midi_output_write(dev->midi, 3, out);
}

void
midi_generic_light_flush(struct ctlra_dev_t *base, uint32_t force)
{
	struct midi_generic_t *dev = (struct midi_generic_t *)base;
}

static int32_t
midi_generic_disconnect(struct ctlra_dev_t *base)
{
	struct midi_generic_t *dev = (struct midi_generic_t *)base;
	ctlra_midi_destroy(dev->midi);
	free(dev);
	return 0;
}

struct ctlra_dev_info_t ctlra_midi_generic_info;

struct ctlra_dev_t *
ctlra_midi_generic_connect(ctlra_event_func event_func, void *userdata,
			   void *future)
{
	(void)future;
	struct midi_generic_t *dev = calloc(1, sizeof(struct midi_generic_t));
	if(!dev)
		goto fail;

	dev->midi = ctlra_midi_open("Ctlra Generic",
				    midi_generic_midi_input_cb, dev);
	if(dev->midi == 0) {
		printf("Ctlra: error opening midi i/o\n");
		goto fail;
	}

	dev->base.info = ctlra_midi_generic_info;

	dev->base.poll = midi_generic_poll;
	dev->base.disconnect = midi_generic_disconnect;
	dev->base.light_set = midi_generic_light_set;
	dev->base.light_flush = midi_generic_light_flush;

	dev->base.event_func = event_func;
	dev->base.event_func_userdata = userdata;

	return (struct ctlra_dev_t *)dev;
fail:
	free(dev);
	return NULL;
}


struct ctlra_dev_info_t ctlra_midi_generic_info = {
	.vendor    = "OpenAV",
	.device    = "Midi Generic",
	.vendor_id = CTLRA_DRIVER_VENDOR,
	.device_id = CTLRA_DRIVER_DEVICE,
	.size_x    = 512,
	.size_y    = 512,
	.get_name =  midi_generic_control_get_name,
};

CTLRA_DEVICE_REGISTER(midi_generic)
