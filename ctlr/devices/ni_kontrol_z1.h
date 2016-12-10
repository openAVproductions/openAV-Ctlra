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

#ifndef OPENAV_CTLR_NI_KONTROL_Z1_H
#define OPENAV_CTLR_NI_KONTROL_Z1_H

enum NI_KONTROL_Z1_LEDS {
	/* Left vol LEDs, first 5 blue, last 2 orange */
	NI_KONTROL_Z1_LED_LEVEL_L1 = 0,
	NI_KONTROL_Z1_LED_LEVEL_L2 = 1,
	NI_KONTROL_Z1_LED_LEVEL_L3 = 2,
	NI_KONTROL_Z1_LED_LEVEL_L4 = 3,
	NI_KONTROL_Z1_LED_LEVEL_L5 = 4,
	NI_KONTROL_Z1_LED_LEVEL_L6 = 5,
	NI_KONTROL_Z1_LED_LEVEL_L7 = 6,

	/* Right vol LEDs, first 5 blue, last 2 orange */
	NI_KONTROL_Z1_LED_LEVEL_R1 = 7,
	NI_KONTROL_Z1_LED_LEVEL_R2 = 8,
	NI_KONTROL_Z1_LED_LEVEL_R3 = 9,
	NI_KONTROL_Z1_LED_LEVEL_R4 = 10,
	NI_KONTROL_Z1_LED_LEVEL_R5 = 11,
	NI_KONTROL_Z1_LED_LEVEL_R6 = 12,
	NI_KONTROL_Z1_LED_LEVEL_R7 = 13,

	/* Misc */
	NI_KONTROL_Z1_LED_CUE_A = 14,
	NI_KONTROL_Z1_LED_CUE_B = 15,
	NI_KONTROL_Z1_LED_L_FX_ON_ORANGE = 16,
	NI_KONTROL_Z1_LED_L_FX_ON_BLUE = 17,
	NI_KONTROL_Z1_LED_MODE = 18,
	NI_KONTROL_Z1_LED_R_FX_ON_ORANGE = 19,
	NI_KONTROL_Z1_LED_R_FX_ON_BLUE = 20,
	NI_KONTROL_Z1_LED_COUNT
};

enum NI_KONTROL_Z1_SLIDERS {
	NI_KONTROL_Z1_SLIDER_LEFT_GAIN = 0,
	NI_KONTROL_Z1_SLIDER_LEFT_EQ_HIGH = 1,
	NI_KONTROL_Z1_SLIDER_LEFT_EQ_MID = 2,
	NI_KONTROL_Z1_SLIDER_LEFT_EQ_LOW = 3,
	NI_KONTROL_Z1_SLIDER_LEFT_FILTER = 4,
	NI_KONTROL_Z1_SLIDER_RIGHT_GAIN = 5,
	NI_KONTROL_Z1_SLIDER_RIGHT_EQ_HIGH = 6,
	NI_KONTROL_Z1_SLIDER_RIGHT_EQ_MID = 7,
	NI_KONTROL_Z1_SLIDER_RIGHT_EQ_LOW = 8,
	NI_KONTROL_Z1_SLIDER_RIGHT_FILTER = 9,
	NI_KONTROL_Z1_SLIDER_CUE_MIX = 10,
	NI_KONTROL_Z1_SLIDER_LEFT_FADER = 11,
	NI_KONTROL_Z1_SLIDER_RIGHT_FADER = 12,
	NI_KONTROL_Z1_SLIDER_CROSS_FADER = 13,
};

static const char *ni_kontrol_z1_slider_names[] = {
	"Gain (Left)",
	"Eq High (Left)",
	"Eq Mid (Left)",
	"Eq Low (Left)",
	"Filter (Left)",
	"Gain (Right)",
	"Eq High (Right)",
	"Eq Mid (Right)",
	"Eq Low (Right)",
	"Filter (Right)",
	"Cue Mix",
	"Fader (Left)",
	"Fader (Right)",
	"Crossfader",
};

enum NI_KONTROL_Z1_BTNS {
	NI_KONTROL_Z1_BTN_CUE_A = 0,
	NI_KONTROL_Z1_BTN_CUE_B = 1,
	NI_KONTROL_Z1_BTN_MODE = 2,
	NI_KONTROL_Z1_BTN_FX_ON_L = 3,
	NI_KONTROL_Z1_BTN_FX_ON_R = 4,
	NI_KONTROL_Z1_BTN_COUNT
};

static const char *ni_kontrol_z1_btn_names[] = {
	"Headphones Cue A",
	"Headphones Cue B",
	"Mode",
	"Filter On (Left)",
	"Filter On (Right)",
};

#endif /* OPENAV_CTLR_NI_KONTROL_Z1_H */

