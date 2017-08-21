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

#include <avtka/avtka.h>

#define PUGL_PARENT 0x0

static const char *
avtka_control_get_name(enum ctlra_event_type_t type, uint32_t control)
{
	if(type == CTLRA_EVENT_BUTTON && control == 0)
		return "Test Button";
	if(type == CTLRA_EVENT_SLIDER && control == 0)
		return "Test Slider";
	return 0;
}

#define CONTROLS_SIZE 10

#define AVTK    0x01
#define VIRTUAL 0x01

/* Represents the the virtual AVTK UI */
struct avtka_t {
	/* base handles usb i/o etc */
	struct ctlra_dev_t base;
	/* represents the Avtka UI */
	struct avtka_t *a;
	/* current value of each controller is stored here */
	float hw_values[CONTROLS_SIZE];
};

static uint32_t avtka_poll(struct ctlra_dev_t *base)
{
	struct avtka_t *dev = (struct avtka_t *)base;

	avtka_iterate(dev->a);
	/* events can be "sent" to the app from the widget callbacks */

	return 0;
}

static void avtka_light_set(struct ctlra_dev_t *base,
				    uint32_t light_id,
				    uint32_t light_status)
{
	struct avtka_t *dev = (struct avtka_t *)base;
	/* TODO: figure out how to display feedback */
}

void
avtka_light_flush(struct ctlra_dev_t *base, uint32_t force)
{
	struct avtka_t *dev = (struct avtka_t *)base;
}

static int32_t
avtka_disconnect(struct ctlra_dev_t *base)
{
	struct avtka_t *dev = (struct avtka_t *)base;

	free(dev);
	return 0;
}

struct ctlra_dev_t *
ctlra_avtka_connect(ctlra_event_func event_func, void *userdata, void *future)
{
	printf("%s\n", __func__);
	(void)future;
	struct avtka_t *dev = (struct avtka_t *)calloc(1, sizeof(struct avtka_t));
	if(!dev)
		goto fail;

	snprintf(dev->base.info.vendor, sizeof(dev->base.info.vendor),
		 "%s", "OpenAV Avtka");
	snprintf(dev->base.info.device, sizeof(dev->base.info.device),
		 "%s", "Virtual Ctlra");

	dev->base.info.vendor_id = AVTK;
	dev->base.info.device_id = VIRTUAL;

	dev->base.poll = avtka_poll;
	dev->base.disconnect = avtka_disconnect;
	dev->base.light_set = avtka_light_set;
	dev->base.info.get_name = avtka_control_get_name;

	dev->base.event_func = event_func;
	dev->base.event_func_userdata = userdata;

	/* initialize the Avtka UI */
	struct avtka_opts_t opts = {
		.w = 360,
		.h = 240
	};
	char name[64];
	snprintf(name, sizeof(name), "%s:%s", dev->base.info.vendor,
		 dev->base.info.device);
	struct avtka_t *a = avtka_create(name, &opts);

	struct avtka_item_opts_t item = {
		.name = "Dial 1",
		.x = 10, .y = 10, .w = 50, .h = 50,
		.draw = AVTKA_DRAW_DIAL,
		.interact = AVTKA_INTERACT_CLICK,
	};
	uint32_t button1 = avtka_item_create(a, &item);
	item.x = 70;
	snprintf(item.name, sizeof(item.name), "Dial 2");
	uint32_t button2 = avtka_item_create(a, &item);
	printf("items created %d and %d\n", button1, button2);

	/* pass in back-pointer to ctlra_dev_t class for sending events */
	dev->a = a;


	return (struct ctlra_dev_t *)dev;
fail:
	free(dev);
	return 0;
}
