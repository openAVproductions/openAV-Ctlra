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

/* Types of events */
enum ctlr_event_id_t {
	CTLR_EVENT_BUTTON = 0,
	CTLR_EVENT_ENCODER,
	CTLR_EVENT_SLIDER,
	CTLR_EVENT_GRID,
};

struct ctlr_dev_t;

/** Represents a "button" on a ctlr device. */
struct ctlr_event_button_t {
	uint32_t button_id;
	bool pressed;
};

/** Represents an "encoder" on a ctlr device. */
struct ctlr_event_encoder_t {
	uint32_t encoder_id;
	int32_t delta;
};

/** The event passed around in the API */
struct ctlr_event_t {
	/** The type of this event */
	enum ctlr_event_id_t id;
	struct ctlr_dev_t *dev;

	/** The event data */
	union {
		struct ctlr_event_button_t button;
		struct ctlr_event_encoder_t encoder;
	};
};

#endif /* OPENAV_CTLR_EVENT */
