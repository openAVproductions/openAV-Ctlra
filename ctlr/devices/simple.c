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

#ifndef OPENAV_CTLR_SIMPLE
#define OPENAV_CTLR_SIMPLE

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "device_impl.h"

struct simple_t {
	struct ctlr_dev_t base;
	uint32_t event_counter;
};

static uint32_t simple_poll(struct ctlr_dev_t *dev);
static int32_t simple_disconnect(struct ctlr_dev_t *dev);

/* replay a button press/release event on every poll. Static event
 * is held here, and fed to application in poll() */
static struct ctlr_event_t events[] = {
	{.id = CTLR_EVENT_BUTTON , .button  = {.id = 0, .pressed = 1},},
	{.id = CTLR_EVENT_BUTTON , .button  = {.id = 0, .pressed = 0},},
	{.id = CTLR_EVENT_ENCODER, .encoder = {.id = 0, .delta =  1},},
	{.id = CTLR_EVENT_ENCODER, .encoder = {.id = 0, .delta = -1},},
	{.id = CTLR_EVENT_GRID, .grid = {.id = 0, .flags = 0x3,
						.pos = 3, .pressure = 0.5f,
						.pressed = 1 },
	}
};
#define NUM_EVENTS (sizeof(events) / sizeof(events[0]))

struct ctlr_dev_t *simple_connect(ctlr_event_func event_func,
				  void *userdata, void *future)
{
	(void)future;
	struct simple_t *dev = calloc(1, sizeof(struct simple_t));
	if(!dev)
		goto fail;

	dev->base.poll = simple_poll;
	dev->base.disconnect = simple_disconnect;
	dev->base.event_func = event_func;
	dev->base.event_func_userdata = userdata;

	printf("%s\n", __func__);
	return (struct ctlr_dev_t *)dev;
fail:
	free(dev);
	return 0;
}

static uint32_t simple_poll(struct ctlr_dev_t *base)
{
	struct simple_t *dev = (struct simple_t *)base;
	struct ctlr_event_t *e[] = {&events[dev->event_counter]};

	dev->event_counter = (dev->event_counter + 1) % NUM_EVENTS;
	dev->base.event_func(base, 1, e, dev->base.event_func_userdata);

	return 0;
}

static int32_t simple_disconnect(struct ctlr_dev_t *base)
{
	struct simple_t *dev = (struct simple_t *)base;
	printf("%s\n", __func__);
	free(dev);
	return 0;
}

#endif /* OPENAV_CTLR_SIMPLE */
