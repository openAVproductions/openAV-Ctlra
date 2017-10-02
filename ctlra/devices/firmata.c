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

#include "firmata.h"

#define CTLRA_DRIVER_VENDOR (0x0)
#define CTLRA_DRIVER_DEVICE (0x0)

#define CONTROLS_MAX 64
#define LIGHTS_MAX 256

struct firmata_t {
	/* base handles usb i/o etc */
	struct ctlra_dev_t base;

	/* firmata instance */
	t_firmata *firmata;
	/* current value of each controller is stored here */
	int hw_values[CONTROLS_MAX];
	/* current state of the lights, only flush on dirty */
	uint8_t lights_dirty;
	uint8_t lights[LIGHTS_MAX];
};

static uint32_t
firmata_poll(struct ctlra_dev_t *base)
{
	struct firmata_t *dev = (struct firmata_t *)base;

	int ret = firmata_pull(dev->firmata);
	if(ret < 0) {
		printf("banished = 1\n");
		ctlra_dev_impl_banish(base);
	}

#if 0
	for(int i = 0; i < ret; i++)
		printf("parse %d = %d\n", i, dev->firmata->parse_buff[i]);
	for(int i = 14; i < 15; i++)
		printf("%d: %d\n", i, dev->firmata->pins[i].value);
#endif

	for(int i = 0; i < 1; i++) {
		int pin = 14 + i;
		int v = dev->firmata->pins[pin].value;
		if(dev->hw_values[i] != v) {
			struct ctlra_event_t event = {
				.type = CTLRA_EVENT_SLIDER,
				.slider  = {
					.id = i,
					.value = v / 1024.f},
			};
			struct ctlra_event_t *e = {&event};
			dev->base.event_func(&dev->base, 1, &e,
					     dev->base.event_func_userdata);
			dev->hw_values[i] = v;
		}
	}

	return 0;
}

static inline void
firmata_light_set(struct ctlra_dev_t *base, uint32_t light_id,
			uint32_t light_status)
{
	struct firmata_t *dev = (struct firmata_t *)base;
	if(!dev || light_id >= LIGHTS_MAX)
		return;

	/* write brighness to all LEDs */
	uint32_t bright = (light_status >> 24) & 0x7F;
	dev->lights[light_id] = bright;

	dev->lights_dirty = 1;
}

void
firmata_light_flush(struct ctlra_dev_t *base, uint32_t force)
{
	struct firmata_t *dev = (struct firmata_t *)base;
	if(!dev->lights_dirty && !force)
		return;
	/* write here */
}

static int32_t
firmata_disconnect(struct ctlra_dev_t *base)
{
	struct firmata_t *dev = (struct firmata_t *)base;

	/* Turn off all lights */
	memset(dev->lights, 0, LIGHTS_MAX);
	if(!base->banished)
		firmata_light_flush(base, 1);

	printf("free firmata instance\n");

	free(dev);
	return 0;
}

struct ctlra_dev_info_t ctlra_firmata_info;

struct ctlra_dev_t *
ctlra_firmata_connect(ctlra_event_func event_func,
				  void *userdata, void *future)
{
	(void)future;
	struct firmata_t *dev = calloc(1, sizeof(struct firmata_t));
	if(!dev)
		goto fail;

	/* initialize the firmata instance */
	/* TODO: make the device ttyXXXY dynamic */
	dev->firmata = firmata_new("/dev/ttyUSB0");
	if(!dev->firmata)
		goto fail;

	/* TODO: how to make this be more elegant? */
	int32_t timeout = 100000000;
	while (!dev->firmata->isReady && timeout) {
		int p = firmata_pull(dev->firmata);
		timeout--;
	}

	dev->base.info = ctlra_firmata_info;

	dev->base.poll = firmata_poll;
	dev->base.disconnect = firmata_disconnect;
	dev->base.light_set = firmata_light_set;
	dev->base.light_flush = firmata_light_flush;

	dev->base.event_func = event_func;
	dev->base.event_func_userdata = userdata;

	return (struct ctlra_dev_t *)dev;
fail:
	free(dev);
	return 0;
}


struct ctlra_dev_info_t ctlra_firmata_info = {
	.vendor    = "Firmata",
	.device    = "Firmware Interface",
	.vendor_id = CTLRA_DRIVER_VENDOR,
	.device_id = CTLRA_DRIVER_DEVICE,

	.control_count[CTLRA_EVENT_SLIDER] = 1,
};

CTLRA_DEVICE_REGISTER(firmata)
