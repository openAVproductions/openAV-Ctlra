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
	NI_KONTROL_Z1_LED_LEVEL_L1 = 1,
	NI_KONTROL_Z1_LED_LEVEL_L2 = 2,
	NI_KONTROL_Z1_LED_LEVEL_L3 = 3,
	NI_KONTROL_Z1_LED_LEVEL_L4 = 4,
	NI_KONTROL_Z1_LED_LEVEL_L5 = 5,
	NI_KONTROL_Z1_LED_LEVEL_L6 = 6,
	NI_KONTROL_Z1_LED_LEVEL_L7 = 7,

	/* Right vol LEDs, first 5 blue, last 2 orange */
	NI_KONTROL_Z1_LED_LEVEL_R1 = 8,
	NI_KONTROL_Z1_LED_LEVEL_R2 = 9,
	NI_KONTROL_Z1_LED_LEVEL_R3 = 10,
	NI_KONTROL_Z1_LED_LEVEL_R4 = 11,
	NI_KONTROL_Z1_LED_LEVEL_R5 = 12,
	NI_KONTROL_Z1_LED_LEVEL_R6 = 13,
	NI_KONTROL_Z1_LED_LEVEL_R7 = 14,

	NI_KONTROL_Z1_LED_CUE_A = 15,
	NI_KONTROL_Z1_LED_CUE_B = 16,

	/* FX ON supports Orange and Blue colours */
	NI_KONTROL_Z1_LED_FX_ON_LEFT = 17,	/* orange */
	NI_KONTROL_Z1_LED_UNUSED_1 = 18,	/* blue */

	NI_KONTROL_Z1_LED_MODE = 19,

	/* FX ON supports Orange and Blue colours */
	NI_KONTROL_Z1_LED_FX_ON_RIGHT = 20,	/* orange */
	NI_KONTROL_Z1_LED_UNUSED_2 = 21,	/* blue */

	NI_KONTROL_Z1_LED_COUNT,
};

enum NI_KONTROL_Z1_CONTROLS {
	/* Faders and Dials */
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
	/* Buttons */
	NI_KONTROL_Z1_BTN_CUE_A = 14,
	NI_KONTROL_Z1_BTN_CUE_B = 15,
	NI_KONTROL_Z1_BTN_MODE = 16,
	NI_KONTROL_Z1_BTN_FX_ON_L = 17,
	NI_KONTROL_Z1_BTN_FX_ON_R = 18,
	/* Count of all controls */
	NI_KONTROL_Z1_CONTROLS_COUNT
};

#endif /* OPENAV_CTLR_NI_KONTROL_Z1_H */

