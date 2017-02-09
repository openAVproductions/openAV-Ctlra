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

#define CONTROLS_SIZE 40
#define LIGHTS_SIZE 10

#define AKAI  0x09e8
#define APC40 0x0073

/* Represents the the hardware device */
struct akai_apc_t {
	/* base handles usb i/o etc */
	struct ctlra_dev_t base;
	/* midi i/o */
	struct ctlra_midi_t *midi;
	/* current value of each controller is stored here */
	float hw_values[CONTROLS_SIZE];
	uint8_t lights[LIGHTS_SIZE];
};

static uint32_t akai_apc_poll(struct ctlra_dev_t *base)
{
	struct akai_apc_t *dev = (struct akai_apc_t *)base;

	ctlra_midi_input_poll(dev->midi);

	return 0;
}

int akai_apc_midi_input_cb(uint8_t nbytes, uint8_t * buf, void *ud)
{
	struct akai_apc_t *dev = (struct akai_apc_t *)ud;
	printf("%d : %02x %02x %02x\n", nbytes, buf[0], buf[1], buf[2]);
	if(buf[1] == 0x37) {
		uint8_t out[] = {0x90, 0x37, 1 + 2 * (buf[0] == 0x80)};
		ctlra_midi_output_write(dev->midi, 3, out);
	}
	return 0;
}

static void akai_apc_light_set(struct ctlra_dev_t *base,
				    uint32_t light_id,
				    uint32_t light_status)
{
	struct akai_apc_t *dev = (struct akai_apc_t *)base;
}

void
akai_apc_light_flush(struct ctlra_dev_t *base, uint32_t force)
{
	struct akai_apc_t *dev = (struct akai_apc_t *)base;
}

static int32_t
akai_apc_disconnect(struct ctlra_dev_t *base)
{
	struct akai_apc_t *dev = (struct akai_apc_t *)base;

	/* Turn off all lights */
	memset(dev->lights, 0, sizeof(dev->lights));

	free(dev);
	return 0;
}

struct ctlra_dev_t *
akai_apc_connect(ctlra_event_func event_func,
				  void *userdata, void *future)
{
	(void)future;
	struct akai_apc_t *dev = calloc(1, sizeof(struct akai_apc_t));
	if(!dev)
		goto fail;

	snprintf(dev->base.info.vendor, sizeof(dev->base.info.vendor),
		 "%s", "Akai");
	snprintf(dev->base.info.device, sizeof(dev->base.info.device),
		 "%s", "APC 40");

	dev->midi = ctlra_midi_open("Ctlra APC40", akai_apc_midi_input_cb, dev);
	if(dev->midi == 0) {
		printf("Ctlra: error opening midi i/o\n");
		goto fail;
	}

	dev->base.info.vendor_id = AKAI;
	dev->base.info.device_id = APC40;

	dev->base.poll = akai_apc_poll;
	dev->base.disconnect = akai_apc_disconnect;
	dev->base.light_set = akai_apc_light_set;
	//dev->base.control_get_name = akai_apc_control_get_name;
	dev->base.light_flush = akai_apc_light_flush;

	dev->base.event_func = event_func;
	dev->base.event_func_userdata = userdata;

	return (struct ctlra_dev_t *)dev;
fail:
	free(dev);
	return 0;
}

