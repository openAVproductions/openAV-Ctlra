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

#ifndef OPENAV_CTLRA_EVENT
#define OPENAV_CTLRA_EVENT

#if  0
#include <stdint.h>
#else
typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef unsigned int uint16_t;
typedef int int32_t;
#endif

/* Types of events */
enum ctlra_event_type_t {
	CTLRA_EVENT_BUTTON = 0,
	CTLRA_EVENT_ENCODER,
	CTLRA_EVENT_SLIDER,
	CTLRA_EVENT_GRID,
	/* The number of event types there are */
	CTLRA_EVENT_T_COUNT,
};

struct ctlra_dev_t;

/** Represents a button. */
struct ctlra_event_button_t {
	/** The id of the button */
	uint32_t id;
	/** The state of the button */
	uint8_t pressed;
};

#define CTLRA_EVENT_ENCODER_FLAG_INT   (1<<0)
#define CTLRA_EVENT_ENCODER_FLAG_FLOAT (1<<1)

/** Represents an endless stepped controller. */
struct ctlra_event_encoder_t {
	/** The id of the encoder */
	uint32_t id;
	/** This flag indicates if the encoder is sending an integer or
	 * floating point delta increment.
	 *
	 * "Stepped" rotary encoders (such as those used to navigate
	 * song libraries etc) will use the integer delta, with a single
	 * step being the smallest delta.
	 *
	 * Endless encoders (without notches) use the floating point delta
	 * and the application may interpret the range of the delta as
	 *  1.0f : maximum amount of clockwise rotation possible in one turn (approx 270 deg)
	 * -1.0f : maximum amount of anti-clockwise rotation possible in one turn (approx 270 deg)
	 *
	 * When explicitly scripting support for a controller, the actual
	 * encoder ID itself will already know which type it is - but for
	 * generic handling of any controller, this int/float metadata is
	 * required for correct handling of the values.
	 */
	uint8_t flags;
	union {
		/** Positive values indicate clockwise rotation, and vice-versa */
		int32_t delta;
		float   delta_float;
	};
};

/** Represents a fader or dial with a fixed range of movement. */
struct ctlra_event_slider_t {
	/** The id of the slider */
	uint32_t id;
	/** Absolute position of the control */
	float value;
};

#define CTLRA_EVENT_GRID_FLAG_BUTTON   (1<<0)
#define CTLRA_EVENT_GRID_FLAG_PRESSURE (1<<1)

/** Represents a grid of buttons */
struct ctlra_event_grid_t {
	/** The ID of the grid */
	uint16_t id;
	/** Flags: set if pressure or button, see defines above */
	uint16_t flags;
	/** The position of the square in the grid */
	uint32_t pos;
	/** The pressure component of the grid, range from 0.f to 1.f */
	float pressure;
	/** The state of the button component of the square. Pressed
	 * should only be set once when the state is concidered changed.
	 * This makes handling note-events from a grid easier */
	uint32_t pressed;
};

/** The event passed around in the API */
struct ctlra_event_t {
	/** The type of this event */
	enum ctlra_event_type_t type;

	/** The event data: see examples/ dir for examples of usage */
	union {
		struct ctlra_event_button_t button;
		struct ctlra_event_encoder_t encoder;
		struct ctlra_event_slider_t slider;
		struct ctlra_event_grid_t grid;
	};
};

/** Callback function that is called for event(s) */
typedef void (*ctlra_event_func)(struct ctlra_dev_t* dev,
				uint32_t num_events,
				struct ctlra_event_t** event,
				void *userdata);

/** Callback function that is called to update the feedback on the device,
 * eg for leds, buttons and screens */
typedef void (*ctlra_feedback_func)(struct ctlra_dev_t *dev,
				    void *userdata);

#endif /* OPENAV_CTLRA_EVENT */
