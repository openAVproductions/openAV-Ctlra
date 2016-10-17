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

#ifndef OPENAV_CTLR_EVENT
#define OPENAV_CTLR_EVENT

#include <stdint.h>
#include <stdbool.h>

/* Types of events */
enum ctlr_event_id_t {
	CTLR_EVENT_BUTTON = 0,
	CTLR_EVENT_ENCODER,
	CTLR_EVENT_SLIDER,
	CTLR_EVENT_GRID,
};

struct ctlr_dev_t;

/** Represents a button. */
struct ctlr_event_button_t {
	/** The id of the button */
	uint32_t id;
	/** The state of the button */
	bool pressed;
};

/** Represents an endless stepped controller. */
struct ctlr_event_encoder_t {
	/** The id of the encoder */
	uint32_t id;
	/** Positive values indicate clockwise rotation, and vice-versa */
	int32_t delta;
};

/** The event passed around in the API */
struct ctlr_event_t {
	/** The type of this event */
	enum ctlr_event_id_t id;
	/** The device this event originates from */
	struct ctlr_dev_t *dev;

	/** The event data: see demo.c for example on how to unpack */
	union {
		struct ctlr_event_button_t button;
		struct ctlr_event_encoder_t encoder;
	};
};

/** Callback function that is called for each event */
typedef void (*ctlr_event_func)(struct ctlr_dev_t* dev,
				uint32_t num_events,
				struct ctlr_event_t** event,
				void *userdata);

#endif /* OPENAV_CTLR_EVENT */
