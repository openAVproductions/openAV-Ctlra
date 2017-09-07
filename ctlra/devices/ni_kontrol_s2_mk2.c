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

#include "ni_kontrol_s2_mk2.h"
#include "impl.h"

#define NI_VENDOR          (0x17cc)
#define NI_KONTROL_S2_MK2  (0x1320)
#define USB_HANDLE_IDX     (0x0)
#define USB_INTERFACE_ID   (0x03)
#define USB_ENDPOINT_READ  (0x83)
#define USB_ENDPOINT_WRITE (0x02)

/* This struct is a generic struct to identify hw controls */
struct ni_kontrol_s2_mk2_ctlra_t {
	int event_id;
	int buf_byte_offset;
	uint32_t mask;
};

static const char *ni_kontrol_s2_mk2_names_encoders[] = {
	"Jog Wheel (L)",
	"Jog Wheel (R)",
	"Left Encoder (L)",
	"Right Encoder (L)",
	"Browse Encoder",
	"Left Encoder (R)",
	"Right Encoder (R)",
	"Gain Encoder (L)",
	"Gain Encoder (R)",
};
#define CONTROL_NAMES_ENCODERS_SIZE (sizeof(ni_kontrol_s2_mk2_names_encoders) /\
				    sizeof(ni_kontrol_s2_mk2_names_encoders[0]))

static const char *ni_kontrol_s2_mk2_names_sliders[] = {
	"Crossfader",
	"Pitch (L)",
	"Pitch (R)",
	"Cue Mix",
	"Remix",
	"Main Level",
	"Level (Back panel)",
	"Fader (L)",
	"Fader (R)",
	"FX Dry/Wet (L)",
	"FX 1 (L)",
	"FX 2 (L)",
	"FX 3 (L)",
	"FX Dry/Wet (R)",
	"FX 1 (R)",
	"FX 2 (R)",
	"FX 3 (R)",
	"EQ HI (L)",
	"EQ MID (L)",
	"EQ LOW (L)",
	"EQ HI (R)",
	"EQ MID (R)",
	"EQ LOW (R)",
};
#define CONTROL_NAMES_SLIDERS_SIZE (sizeof(ni_kontrol_s2_mk2_names_sliders) /\
				    sizeof(ni_kontrol_s2_mk2_names_sliders[0]))

static const char *ni_kontrol_s2_mk2_names_buttons[] = {
	"Play (R)",
	"Cue (R)",
	"Sync (R)",
	"Shift (R)",
	"Cue 4 (R)",
	"Cue 3 (R)",
	"Cue 2 (R)",
	"Cue 1 (R)",

	"Jog Wheel Press (L)",
	"Jog Wheel Press (R)",
	"Main/Booth Switch",
	"Mic Engage",
	"Mixer Cue (R)",
	"Flux (R)",
	"Loop In (R)",
	"Loop Out (R)",

	"Play (L)",
	"Cue (L)",
	"Sync (L)",
	"Shift (L)",
	"Cue 4 (L)",
	"Cue 3 (L)",
	"Cue 2 (L)",
	"Cue 1 (L)",

	"Remix On B",
	"Remix On A",
	"Browse Load B",
	"Browse Load A",
	"Mixer Cue (L)",
	"Flux (L)",
	"Loop In (L)",
	"Loop Out (L)",

	"FX2 Dry/Wet",
	"FX2 3",
	"FX2 2",
	"FX2 1",
	"Gain Press (L)",
	"Gain Press (R)",

	"Mixer FX 2 (R)",
	"Mixer FX 1 (R)",
	"Mixer FX 2 (L)",
	"Mixer FX 1 (L)",
	"FX1 Dry/Wet",
	"FX1 3",
	"FX1 2",
	"FX1 1",

	"Left Encoder Press (L)",
	"Right Encoder Press (L)",
	"Browse Encoder Press",
	"Left Encoder Press (R)",
	"Right Encoder Press (R)",
};
#define CONTROL_NAMES_BUTTONS_SIZE (sizeof(ni_kontrol_s2_mk2_names_buttons) /\
				    sizeof(ni_kontrol_s2_mk2_names_buttons[0]))
#define CONTROL_NAMES_SIZE (CONTROL_NAMES_SLIDERS_SIZE + \
			    CONTROL_NAMES_BUTTONS_SIZE)

static const struct ni_kontrol_s2_mk2_ctlra_t sliders[] = {
	/* Left */
	{NI_KONTROL_S2_MK2_SLIDER_CROSSFADER,  1, UINT32_MAX},
	{NI_KONTROL_S2_MK2_SLIDER_PITCH_L   ,  3, UINT32_MAX},
	{NI_KONTROL_S2_MK2_SLIDER_PITCH_R   ,  5, UINT32_MAX},
	{NI_KONTROL_S2_MK2_SLIDER_CUE_MIX   ,  7, UINT32_MAX},
	{NI_KONTROL_S2_MK2_SLIDER_REMIX     ,  9, UINT32_MAX},
	{NI_KONTROL_S2_MK2_SLIDER_MAIN_LEVEL, 11, UINT32_MAX},
	{NI_KONTROL_S2_MK2_SLIDER_LEVEL     , 13, UINT32_MAX},
	{NI_KONTROL_S2_MK2_SLIDER_FADER_L   , 15, UINT32_MAX},
	{NI_KONTROL_S2_MK2_SLIDER_FADER_R   , 17, UINT32_MAX},
	{NI_KONTROL_S2_MK2_SLIDER_FX_L_DRY  , 19, UINT32_MAX},
	{NI_KONTROL_S2_MK2_SLIDER_FX_L_1    , 21, UINT32_MAX},
	{NI_KONTROL_S2_MK2_SLIDER_FX_L_2    , 23, UINT32_MAX},
	{NI_KONTROL_S2_MK2_SLIDER_FX_L_3    , 25, UINT32_MAX},
	{NI_KONTROL_S2_MK2_SLIDER_FX_L_DRY  , 27, UINT32_MAX},
	{NI_KONTROL_S2_MK2_SLIDER_FX_L_1    , 29, UINT32_MAX},
	{NI_KONTROL_S2_MK2_SLIDER_FX_L_2    , 31, UINT32_MAX},
	{NI_KONTROL_S2_MK2_SLIDER_FX_L_3    , 33, UINT32_MAX},
	{NI_KONTROL_S2_MK2_SLIDER_EQ_L_HI   , 35, UINT32_MAX},
	{NI_KONTROL_S2_MK2_SLIDER_EQ_L_MID  , 37, UINT32_MAX},
	{NI_KONTROL_S2_MK2_SLIDER_EQ_L_LOW  , 39, UINT32_MAX},
	{NI_KONTROL_S2_MK2_SLIDER_EQ_R_HI   , 41, UINT32_MAX},
	{NI_KONTROL_S2_MK2_SLIDER_EQ_R_MID  , 43, UINT32_MAX},
	{NI_KONTROL_S2_MK2_SLIDER_EQ_R_LOW  , 45, UINT32_MAX},
};
#define SLIDERS_SIZE (sizeof(sliders) / sizeof(sliders[0]))

#define DIAL_CENTER (CTLRA_ITEM_DIAL | CTLRA_ITEM_CENTER_NOTCH)
static struct ctlra_item_info_t sliders_info[] = {
	/* crossfader, pitch L, pitch R */
	{.x = 195, .y = 285, .w =  50, .h  = 20, .flags = CTLRA_ITEM_FADER},
	{.x =  10, .y = 230, .w =  20, .h  = 80, .flags = CTLRA_ITEM_FADER},
	{.x = 410, .y = 230, .w =  20, .h  = 80, .flags = CTLRA_ITEM_FADER},
	/* cue mix, remix, main level, level (back panel) */
	{.x = 330, .y = 330, .w =  14, .h  = 14, .flags = DIAL_CENTER},
	{.x = 210, .y = 102, .w =  18, .h  = 18, .flags = DIAL_CENTER},
	{.x = 210, .y =  60, .w =  18, .h  = 18, .flags = CTLRA_ITEM_DIAL},
	{.x = 195, .y =   5, .w =  18, .h  = 18, .flags = CTLRA_ITEM_DIAL},
	/* Fader L, fader R */
	{.x = 170, .y = 220, .w =  20, .h  = 50, .flags = CTLRA_ITEM_FADER},
	{.x = 250, .y = 220, .w =  20, .h  = 50, .flags = CTLRA_ITEM_FADER},
	/* FX L Dry, 1, 2, 3 */
	{.x =  15, .y =  60, .w =  18, .h  = 18, .flags = DIAL_CENTER},
	{.x =  50, .y =  60, .w =  18, .h  = 18, .flags = DIAL_CENTER},
	{.x =  82, .y =  60, .w =  18, .h  = 18, .flags = DIAL_CENTER},
	{.x = 115, .y =  60, .w =  18, .h  = 18, .flags = DIAL_CENTER},
	/* FX R Dry, 1, 2, 3 */
	{.x = 315, .y =  60, .w =  18, .h  = 18, .flags = DIAL_CENTER},
	{.x = 350, .y =  60, .w =  18, .h  = 18, .flags = DIAL_CENTER},
	{.x = 382, .y =  60, .w =  18, .h  = 18, .flags = DIAL_CENTER},
	{.x = 415, .y =  60, .w =  18, .h  = 18, .flags = DIAL_CENTER},
	/* EQ L Hi, Mid, Low */
	{.x = 172, .y = 102, .w =  18, .h  = 18, .flags = DIAL_CENTER},
	{.x = 172, .y = 134, .w =  18, .h  = 18, .flags = DIAL_CENTER},
	{.x = 172, .y = 166, .w =  18, .h  = 18, .flags = DIAL_CENTER},
	/* EQ L Hi, Mid, Low */
	{.x = 250, .y = 102, .w =  18, .h  = 18, .flags = DIAL_CENTER},
	{.x = 250, .y = 134, .w =  18, .h  = 18, .flags = DIAL_CENTER},
	{.x = 250, .y = 166, .w =  18, .h  = 18, .flags = DIAL_CENTER},
};

static const struct ni_kontrol_s2_mk2_ctlra_t buttons[] = {
	{NI_KONTROL_S2_MK2_BTN_DECKB_PLAY , 9, 0x01},
	{NI_KONTROL_S2_MK2_BTN_DECKB_CUE  , 9, 0x02},
	{NI_KONTROL_S2_MK2_BTN_DECKB_SYNC , 9, 0x04},
	{NI_KONTROL_S2_MK2_BTN_DECKB_SHIFT, 9, 0x08},
	{NI_KONTROL_S2_MK2_BTN_DECKB_CUE_4, 9, 0x10},
	{NI_KONTROL_S2_MK2_BTN_DECKB_CUE_3, 9, 0x20},
	{NI_KONTROL_S2_MK2_BTN_DECKB_CUE_2, 9, 0x40},
	{NI_KONTROL_S2_MK2_BTN_DECKB_CUE_1, 9, 0x80},

	{NI_KONTROL_S2_MK2_BTN_DECKA_JOG_PRESS  , 10, 0x01},
	{NI_KONTROL_S2_MK2_BTN_DECKB_JOG_PRESS  , 10, 0x02},
	{NI_KONTROL_S2_MK2_BTN_MAIN_BOOTH_SWITCH, 10, 0x04},
	{NI_KONTROL_S2_MK2_BTN_MIC_ENGAGE       , 10, 0x08},
	{NI_KONTROL_S2_MK2_BTN_DECKB_MIXER_CUE  , 10, 0x10},
	{NI_KONTROL_S2_MK2_BTN_DECKB_FLUX       , 10, 0x20},
	{NI_KONTROL_S2_MK2_BTN_DECKB_LOOP_IN    , 10, 0x40},
	{NI_KONTROL_S2_MK2_BTN_DECKB_LOOP_OUT   , 10, 0x80},

	{NI_KONTROL_S2_MK2_BTN_DECKA_PLAY , 11, 0x01},
	{NI_KONTROL_S2_MK2_BTN_DECKA_CUE  , 11, 0x02},
	{NI_KONTROL_S2_MK2_BTN_DECKA_SYNC , 11, 0x04},
	{NI_KONTROL_S2_MK2_BTN_DECKA_SHIFT, 11, 0x08},
	{NI_KONTROL_S2_MK2_BTN_DECKA_CUE_4, 11, 0x10},
	{NI_KONTROL_S2_MK2_BTN_DECKA_CUE_3, 11, 0x20},
	{NI_KONTROL_S2_MK2_BTN_DECKA_CUE_2, 11, 0x40},
	{NI_KONTROL_S2_MK2_BTN_DECKA_CUE_1, 11, 0x80},

	{NI_KONTROL_S2_MK2_BTN_REMIX_ON_B     , 12, 0x01},
	{NI_KONTROL_S2_MK2_BTN_REMIX_ON_A     , 12, 0x02},
	{NI_KONTROL_S2_MK2_BTN_BROWSE_LOAD_B  , 12, 0x04},
	{NI_KONTROL_S2_MK2_BTN_BROWSE_LOAD_A  , 12, 0x08},
	{NI_KONTROL_S2_MK2_BTN_DECKB_MIXER_CUE, 12, 0x10},
	{NI_KONTROL_S2_MK2_BTN_DECKB_FLUX     , 12, 0x20},
	{NI_KONTROL_S2_MK2_BTN_DECKB_LOOP_IN  , 12, 0x40},
	{NI_KONTROL_S2_MK2_BTN_DECKB_LOOP_OUT , 12, 0x80},

	/* waste */
	/* waste */
	{NI_KONTROL_S2_MK2_BTN_FX2_DRY_WET     , 13, 0x04},
	{NI_KONTROL_S2_MK2_BTN_FX2_3           , 13, 0x08},
	{NI_KONTROL_S2_MK2_BTN_FX2_2           , 13, 0x10},
	{NI_KONTROL_S2_MK2_BTN_FX2_1           , 13, 0x20},
	{NI_KONTROL_S2_MK2_BTN_DECKA_GAIN_PRESS, 13, 0x40},
	{NI_KONTROL_S2_MK2_BTN_DECKA_GAIN_PRESS, 13, 0x80},

	{NI_KONTROL_S2_MK2_BTN_MIXER_B_FX2, 14, 0x01},
	{NI_KONTROL_S2_MK2_BTN_MIXER_B_FX1, 14, 0x02},
	{NI_KONTROL_S2_MK2_BTN_MIXER_A_FX2, 14, 0x04},
	{NI_KONTROL_S2_MK2_BTN_MIXER_A_FX1, 14, 0x08},
	{NI_KONTROL_S2_MK2_BTN_FX2_DRY_WET, 14, 0x10},
	{NI_KONTROL_S2_MK2_BTN_FX2_3      , 14, 0x20},
	{NI_KONTROL_S2_MK2_BTN_FX2_2      , 14, 0x40},
	{NI_KONTROL_S2_MK2_BTN_FX2_1      , 14, 0x80},

	{NI_KONTROL_S2_MK2_BTN_DECKA_LEFT_ENCODER_PRESS , 15, 0x01},
	{NI_KONTROL_S2_MK2_BTN_DECKA_RIGHT_ENCODER_PRESS, 15, 0x02},
	{NI_KONTROL_S2_MK2_BTN_BROWSE_ENCODER_PRESS     , 15, 0x04},
	{NI_KONTROL_S2_MK2_BTN_DECKB_LEFT_ENCODER_PRESS , 15, 0x08},
	{NI_KONTROL_S2_MK2_BTN_DECKB_RIGHT_ENCODER_PRESS, 15, 0x10},
};
#define BUTTONS_SIZE (sizeof(buttons) / sizeof(buttons[0]))

static struct ctlra_item_info_t buttons_info[] = {
	/* R: play cue sync shift */
	{.x = 376, .y = 295, .w = 18, .h = 15, .flags = CTLRA_ITEM_BUTTON},
	{.x = 350, .y = 295, .w = 18, .h = 15, .flags = CTLRA_ITEM_BUTTON},
	{.x = 324, .y = 295, .w = 18, .h = 15, .flags = CTLRA_ITEM_BUTTON},
	{.x = 298, .y = 295, .w = 18, .h = 15, .flags = CTLRA_ITEM_BUTTON},
	/* R: 4, 3, 2, 1 */
	{.x = 376, .y = 268, .w = 18, .h = 15, .flags = CTLRA_ITEM_BUTTON},
	{.x = 350, .y = 268, .w = 18, .h = 15, .flags = CTLRA_ITEM_BUTTON},
	{.x = 324, .y = 268, .w = 18, .h = 15, .flags = CTLRA_ITEM_BUTTON},
	{.x = 298, .y = 268, .w = 18, .h = 15, .flags = CTLRA_ITEM_BUTTON},
	/* Jog wheel Press L, R, Main/Booth Switch, Mic Engage */
	{.x = 142, .y = 210, .w = 18, .h = 14, .flags = CTLRA_ITEM_BUTTON},
	{.x = 280, .y = 210, .w = 18, .h = 14, .flags = CTLRA_ITEM_BUTTON},
	{.x = 164, .y =  10, .w = 24, .h = 14, .flags = CTLRA_ITEM_BUTTON},
	{.x = 380, .y = 350, .w = 18, .h = 18, .flags = CTLRA_ITEM_BUTTON},
	/* R: Mixer Cue, Flux, Loop In, Loop Out */
	{.x = 250, .y = 195, .w = 18, .h = 10, .flags = CTLRA_ITEM_BUTTON},
	{.x = 415, .y = 218, .w = 10, .h = 10, .flags = CTLRA_ITEM_BUTTON},
	{.x = 324, .y = 241, .w = 18, .h = 10, .flags = CTLRA_ITEM_BUTTON},
	{.x = 350, .y = 241, .w = 18, .h = 10, .flags = CTLRA_ITEM_BUTTON},
	/* L: play cue sync shift */
	{.x = 126, .y = 295, .w = 18, .h = 15, .flags = CTLRA_ITEM_BUTTON},
	{.x = 100, .y = 295, .w = 18, .h = 15, .flags = CTLRA_ITEM_BUTTON},
	{.x =  74, .y = 295, .w = 18, .h = 15, .flags = CTLRA_ITEM_BUTTON},
	{.x =  48, .y = 295, .w = 18, .h = 15, .flags = CTLRA_ITEM_BUTTON},
	/* L: 4, 3, 2, 1 */
	{.x = 126, .y = 268, .w = 18, .h = 15, .flags = CTLRA_ITEM_BUTTON},
	{.x = 100, .y = 268, .w = 18, .h = 15, .flags = CTLRA_ITEM_BUTTON},
	{.x =  74, .y = 268, .w = 18, .h = 15, .flags = CTLRA_ITEM_BUTTON},
	{.x =  48, .y = 268, .w = 18, .h = 15, .flags = CTLRA_ITEM_BUTTON},
	/* remix On B, A, Browse Load B, A */
	{.x = 222, .y = 125, .w = 14, .h = 14, .flags = CTLRA_ITEM_BUTTON},
	{.x = 202, .y = 125, .w = 14, .h = 14, .flags = CTLRA_ITEM_BUTTON},
	{.x = 222, .y = 184, .w = 14, .h = 14, .flags = CTLRA_ITEM_BUTTON},
	{.x = 202, .y = 184, .w = 14, .h = 14, .flags = CTLRA_ITEM_BUTTON},
	/* L: Mixer Cue, Flux, Loop In, Loop Out */
	{.x = 170, .y = 195, .w = 18, .h = 10, .flags = CTLRA_ITEM_BUTTON},
	{.x =  15, .y = 218, .w = 10, .h = 10, .flags = CTLRA_ITEM_BUTTON},
	{.x =  74, .y = 241, .w = 18, .h = 10, .flags = CTLRA_ITEM_BUTTON},
	{.x = 100, .y = 241, .w = 18, .h = 10, .flags = CTLRA_ITEM_BUTTON},
	/* FX 2: Dry wet, 3, 2, 1, */
	{.x = 317, .y = 74, .w = 14, .h = 14, .flags = CTLRA_ITEM_BUTTON},
	{.x = 417, .y = 74, .w = 14, .h = 14, .flags = CTLRA_ITEM_BUTTON},
	{.x = 384, .y = 74, .w = 14, .h = 14, .flags = CTLRA_ITEM_BUTTON},
	{.x = 352, .y = 74, .w = 14, .h = 14, .flags = CTLRA_ITEM_BUTTON},
	/* TODO: gain encoders press L, R */
	{.x = 0, .y = 0, .w = 0, .h = 0, .flags = 0},
	{.x = 0, .y = 0, .w = 0, .h = 0, .flags = 0},
	/* Mixer R: FX 2, 1 */
	{.x = 265, .y = 74, .w = 14, .h = 14, .flags = CTLRA_ITEM_BUTTON},
	{.x = 245, .y = 74, .w = 14, .h = 14, .flags = CTLRA_ITEM_BUTTON},
	/* Mixer L: FX 2, 1 */
	{.x = 185, .y = 74, .w = 14, .h = 14, .flags = CTLRA_ITEM_BUTTON},
	{.x = 165, .y = 74, .w = 14, .h = 14, .flags = CTLRA_ITEM_BUTTON},
	/* FX 1: Dry wet, 3, 2, 1, */
	{.x =  17, .y = 74, .w = 14, .h = 14, .flags = CTLRA_ITEM_BUTTON},
	{.x = 117, .y = 74, .w = 14, .h = 14, .flags = CTLRA_ITEM_BUTTON},
	{.x =  84, .y = 74, .w = 14, .h = 14, .flags = CTLRA_ITEM_BUTTON},
	{.x =  54, .y = 74, .w = 14, .h = 14, .flags = CTLRA_ITEM_BUTTON},
	/* TODO: Left deck loop encoder press L, R */
	{.x = 0, .y = 0, .w = 0, .h = 0, .flags = 0},
	{.x = 0, .y = 0, .w = 0, .h = 0, .flags = 0},
	/* TODO: Browse encoder press */
	{.x = 0, .y = 0, .w = 0, .h = 0, .flags = 0},
	/* TODO: Right deck loop encoder press L, R */
	{.x = 0, .y = 0, .w = 0, .h = 0, .flags = 0},
	{.x = 0, .y = 0, .w = 0, .h = 0, .flags = 0},
};

#define CONTROLS_SIZE (SLIDERS_SIZE + BUTTONS_SIZE)

#define ENCODER_COUNT 9

#define ENCODER_NOTCH  (CTLRA_ITEM_ENCODER | CTLRA_ITEM_CENTER_NOTCH)
static struct ctlra_item_info_t encoders_info[] = {
	/* jog left, jog right */
	{.x =  15, .y =  95, .w = 125, .h = 125, .flags = CTLRA_ITEM_ENCODER},
	{.x = 300, .y =  95, .w = 125, .h = 125, .flags = CTLRA_ITEM_ENCODER},
	/* left encoder (Left deck), right encoder (left deck) */
	{.x =  44, .y = 234, .w =  24, .h =  24, .flags = ENCODER_NOTCH},
	{.x = 122, .y = 234, .w =  24, .h =  24, .flags = ENCODER_NOTCH},
	/* browse */
	{.x = 208, .y = 155, .w =  24, .h =  24, .flags = ENCODER_NOTCH},
	/* left encoder (Left deck), right encoder (left deck) */
	{.x = 294, .y = 234, .w =  24, .h =  24, .flags = ENCODER_NOTCH},
	{.x = 372, .y = 234, .w =  24, .h =  24, .flags = ENCODER_NOTCH},
	/* gain left, gain right */
	{.x = 172, .y =  60, .w =  18, .h =  18, .flags = ENCODER_NOTCH},
	{.x = 252, .y =  60, .w =  18, .h =  18, .flags = ENCODER_NOTCH},
};

/* 36 LEDs on top half - all brightness only */
#define LED_COUNT 64
/* 8 cue buttons with RGB each, 8 brightness only */
#define LED_DECK_COUNT (8*3 + 8)

/* Represents the the hardware device */
struct ni_kontrol_s2_mk2_t {
	/* base handles usb i/o etc */
	struct ctlra_dev_t base;
	/* current value of each controller is stored here */
	float hw_values[CONTROLS_SIZE];
	uint8_t jog_wheels[2];
	uint8_t jog_wheels_value[2];
	uint32_t jog_wheels_quadrant[2];
	uint32_t jog_wheels_1024_value[2];
	/* current values for stepped encoders */
	uint8_t encoder_values[ENCODER_COUNT];
	/* current state of the lights, only flush on dirty */
	uint8_t lights_dirty;

	uint8_t lights_interface;
	uint8_t lights[LED_COUNT];

	uint8_t deck_lights_interface;
	uint8_t deck_lights[LED_DECK_COUNT];
};

static const char *
ni_kontrol_s2_mk2_control_get_name(enum ctlra_event_type_t type,
			       uint32_t control)
{
	switch(type) {
	case CTLRA_EVENT_SLIDER:
		if(control >= CONTROL_NAMES_SLIDERS_SIZE)
			return 0;
		return ni_kontrol_s2_mk2_names_sliders[control];
	case CTLRA_EVENT_ENCODER:
		if(control >= CONTROL_NAMES_ENCODERS_SIZE)
			return 0;
		return ni_kontrol_s2_mk2_names_encoders[control];

	case CTLRA_EVENT_BUTTON:
		if(control >= CONTROL_NAMES_BUTTONS_SIZE)
			return 0;
		return ni_kontrol_s2_mk2_names_buttons[control];
	default:
		break;
	}

	return 0;
}

static uint32_t ni_kontrol_s2_mk2_poll(struct ctlra_dev_t *base)
{
	struct ni_kontrol_s2_mk2_t *dev = (struct ni_kontrol_s2_mk2_t *)base;
#define BUF_SIZE 1024
	uint8_t buf[BUF_SIZE];

	int handle_idx = 0;
	ctlra_dev_impl_usb_interrupt_read(base, USB_HANDLE_IDX,
					  USB_ENDPOINT_READ,
					  buf, BUF_SIZE);
	return 0;
}

void ni_kontrol_s2_mk2_usb_read_cb(struct ctlra_dev_t *base, uint32_t endpoint,
				uint8_t *data, uint32_t size)
{
	struct ni_kontrol_s2_mk2_t *dev = (struct ni_kontrol_s2_mk2_t *)base;
	uint8_t *buf = data;

	switch(size) {
	case 17: { /* buttons and jog wheels */
		const uint8_t jog_offset[2] = {1, 5};
		for(int i = 0; i < 2; i++) {
			/* Algorithm works, but can be improved for readability and
			 * efficiency. PRs welcome :) */
			uint8_t new_pos = buf[jog_offset[i]];
			if(new_pos != dev->jog_wheels[i]) {
				/* detect 255 based wrap */
				int32_t diff = new_pos - dev->jog_wheels[i];
				dev->jog_wheels_quadrant[i] -= (diff > 200);
				dev->jog_wheels_quadrant[i] += (diff < -200);

				dev->jog_wheels_value[i] += diff;

				/* calculate 1024 based "full circle" value */
				int nv = (255 * (dev->jog_wheels_quadrant[i] % 4))
					+ dev->jog_wheels_value[i];
				dev->jog_wheels[i] = new_pos;

				int delta_1024 = dev->jog_wheels_1024_value[i] - nv;
				dev->jog_wheels_1024_value[i] = nv;

				/* handle 1024 based value wrap */
				if(delta_1024 > 800)
					delta_1024 -= 1024;
				if(delta_1024 < -800)
					delta_1024 += 1024;

				float delta_01 = delta_1024 / 1024.f;

				struct ctlra_event_t event = {
					.type = CTLRA_EVENT_ENCODER,
					.encoder  = {
						.id = i,
						.flags = CTLRA_EVENT_ENCODER_FLAG_FLOAT,
						.delta_float = -delta_01,
					}
				};
				struct ctlra_event_t *e = {&event};
				dev->base.event_func(&dev->base, 1, &e,
						dev->base.event_func_userdata);
			}
		}

		for(uint32_t i = 0; i < BUTTONS_SIZE; i++) {
			int id     = i;
			int offset = buttons[i].buf_byte_offset;
			int mask   = buttons[i].mask;

			uint16_t v = *((uint16_t *)&buf[offset]) & mask;
			int value_idx = SLIDERS_SIZE + i;

			if(dev->hw_values[value_idx] != v) {
				dev->hw_values[value_idx] = v;

				struct ctlra_event_t event = {
					.type = CTLRA_EVENT_BUTTON,
					.button  = {
						.id = id,
						.pressed = v > 0},
				};
				struct ctlra_event_t *e = {&event};
				dev->base.event_func(&dev->base, 1, &e,
						     dev->base.event_func_userdata);
			}
		}
		} break;

	case 51: { /* sliders dials and pitch */
		struct ctlra_event_t event = {
			.type = CTLRA_EVENT_ENCODER,
			.encoder = {
				.flags = CTLRA_EVENT_ENCODER_FLAG_INT,
			},
		};

		for(uint32_t i = 0; i < ENCODER_COUNT; i++) {
			uint8_t v;
			if(i % 2 == 0)
				v =  buf[1+i/2] & 0x0f;
			else
				v = (buf[1+i/2] & 0xf0) >> 4;

			int m = ctlra_dev_encoder_wrap_16(v, dev->encoder_values[i]);
			/* +2 offset for jog wheels on 0,1 */
			event.encoder.id = i + 2;
			event.encoder.delta = m;

			if(v != dev->encoder_values[i]) {
				struct ctlra_event_t *e = {&event};
				dev->base.event_func(&dev->base, 1, &e,
						     dev->base.event_func_userdata);
				dev->encoder_values[i] = v;
			}
		}

		for(uint32_t i = 0; i < SLIDERS_SIZE; i++) {
			int id     = i;
			int offset = sliders[i].buf_byte_offset;
			int mask   = sliders[i].mask;

			uint16_t v = *((uint16_t *)&buf[offset+4]) & mask;
			if(dev->hw_values[i] != v) {
				dev->hw_values[i] = v;
				struct ctlra_event_t event = {
					.type = CTLRA_EVENT_SLIDER,
					.slider  = {
						.id = id,
						.value = v / 4096.f},
				};
				struct ctlra_event_t *e = {&event};
				dev->base.event_func(&dev->base, 1, &e,
						     dev->base.event_func_userdata);
			}
		}
		break;
		}
	}
}

static void ni_kontrol_s2_mk2_light_set(struct ctlra_dev_t *base,
				    uint32_t light_id,
				    uint32_t light_status)
{
	struct ni_kontrol_s2_mk2_t *dev = (struct ni_kontrol_s2_mk2_t *)base;

	const uint32_t bright = (light_status >> 24) & 0x7F * 2;

	if(light_id < 36)
		dev->lights[light_id] = bright;

	/* 36 to 43 is Cue buttons, RGB each */
	if(light_id >= 36 && light_id < 44) {
		uint32_t over_36 = light_id - 36;
		uint8_t r = (light_status >> 16) & 0xff;
		uint8_t g = (light_status >>  8) & 0xff;
		uint8_t b = (light_status >>  0) & 0xff;
		dev->deck_lights[over_36*2+light_id - 36] = r;
		dev->deck_lights[over_36*2+light_id - 35] = g;
		dev->deck_lights[over_36*2+light_id - 34] = b;
	}
	if(light_id >= 44 && light_id < 52)
		dev->deck_lights[light_id - 36 + 16] = bright;

	dev->lights_dirty = 1;
	return ;
}

void
ni_kontrol_s2_mk2_light_flush(struct ctlra_dev_t *base, uint32_t force)
{
	struct ni_kontrol_s2_mk2_t *dev = (struct ni_kontrol_s2_mk2_t *)base;
	if(!dev->lights_dirty && !force)
		return;
	/* technically interface 3, testing showed 0 works but 3 doesnt */
	uint8_t *data = &dev->lights_interface;

	/* all normal single-colour (brightness) leds */
	dev->lights_interface = 0x80;

	int ret = ctlra_dev_impl_usb_interrupt_write(base, USB_HANDLE_IDX,
						     USB_ENDPOINT_WRITE,
						     data,
						     LED_COUNT+1);
	if(ret < 0) {
		//printf("%s write failed!\n", __func__);
	}

	/* Cue / Remix slots, shift0sync-cue-play for both decks */
	data = &dev->deck_lights_interface;
	dev->deck_lights_interface = 0x81;
	ret = ctlra_dev_impl_usb_interrupt_write(base, USB_HANDLE_IDX,
						     USB_ENDPOINT_WRITE,
						     data,
						     LED_DECK_COUNT+1);
	if(ret < 0) {
		//printf("%s write failed!\n", __func__);
	}

}

static int32_t
ni_kontrol_s2_mk2_disconnect(struct ctlra_dev_t *base)
{
	struct ni_kontrol_s2_mk2_t *dev = (struct ni_kontrol_s2_mk2_t *)base;

	/* Turn off all lights */
	memset(dev->lights     , 0, LED_COUNT);
	memset(dev->deck_lights, 0, LED_DECK_COUNT);
	if(!base->banished)
		ni_kontrol_s2_mk2_light_flush(base, 1);

	ctlra_dev_impl_usb_close(base);
	free(dev);
	return 0;
}

struct ctlra_dev_t *
ctlra_ni_kontrol_s2_mk2_connect(ctlra_event_func event_func,
				  void *userdata, void *future)
{
	(void)future;
	struct ni_kontrol_s2_mk2_t *dev =
		calloc(1, sizeof(struct ni_kontrol_s2_mk2_t));
	if(!dev)
		goto fail;

	snprintf(dev->base.info.vendor, sizeof(dev->base.info.vendor),
		 "%s", "Native Instruments");
	snprintf(dev->base.info.device, sizeof(dev->base.info.device),
		 "%s", "Kontrol S2_MK2");


	dev->base.info.control_count[CTLRA_EVENT_BUTTON] = BUTTONS_SIZE;
	dev->base.info.control_count[CTLRA_EVENT_SLIDER] = SLIDERS_SIZE;

	dev->base.info.control_info[CTLRA_EVENT_SLIDER] = &sliders_info,
	dev->base.info.control_info[CTLRA_EVENT_ENCODER] = &encoders_info,
	dev->base.info.control_info[CTLRA_EVENT_BUTTON] = &buttons_info,
	dev->base.info.get_name = ni_kontrol_s2_mk2_control_get_name;

	int err = ctlra_dev_impl_usb_open(&dev->base,NI_VENDOR,
					  NI_KONTROL_S2_MK2);
	if(err) {
		free(dev);
		return 0;
	}

	err = ctlra_dev_impl_usb_open_interface(&dev->base,
					 USB_INTERFACE_ID, USB_HANDLE_IDX);
	if(err) {
		free(dev);
		return 0;
	}

	dev->base.poll = ni_kontrol_s2_mk2_poll;
	dev->base.disconnect = ni_kontrol_s2_mk2_disconnect;
	dev->base.light_set = ni_kontrol_s2_mk2_light_set;
	dev->base.light_flush = ni_kontrol_s2_mk2_light_flush;
	dev->base.usb_read_cb = ni_kontrol_s2_mk2_usb_read_cb;

	dev->base.event_func = event_func;
	dev->base.event_func_userdata = userdata;

	return (struct ctlra_dev_t *)dev;
fail:
	free(dev);
	return 0;
}

struct ctlra_dev_info_t ctlra_ni_kontrol_s2_mk2_info = {
	.vendor    = "Native Instruments",
	.device    = "Kontrol S2 Mk2",
	.vendor_id = NI_VENDOR,
	.device_id = NI_KONTROL_S2_MK2,
	.size_x    = 440,
	.size_y    = 370, /* includes back and front panels */

	.control_count[CTLRA_EVENT_BUTTON] = BUTTONS_SIZE,
	.control_count[CTLRA_EVENT_SLIDER] = SLIDERS_SIZE,
	.control_count[CTLRA_EVENT_ENCODER] = ENCODER_COUNT,

	.control_info[CTLRA_EVENT_BUTTON] = &buttons_info,
	.control_info[CTLRA_EVENT_SLIDER] = &sliders_info,
	.control_info[CTLRA_EVENT_ENCODER] = &encoders_info,

	.get_name = ni_kontrol_s2_mk2_control_get_name,
};
