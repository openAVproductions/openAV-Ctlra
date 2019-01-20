/*
 * Copyright (c) 2018, OpenAV Productions,
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
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>

#include "impl.h"

#define CTLRA_DRIVER_VENDOR (0x17cc)
#define CTLRA_DRIVER_DEVICE (0xbaff)

/* Note that interface 0 is used in an "alt setting" mode */
#define USB_INTERFACE_ID      (0x00)
#define USB_HANDLE_IDX        (0x00)
#define USB_ENDPOINT_READ     (0x84)
#define USB_ENDPOINT_WRITE    (0x03)

/* This struct is a generic struct to identify hw controls */
struct ni_kontrol_s4mk1_ctlra_t {
	int buf_byte_offset;
	uint32_t mask;
};

static const char *ni_kontrol_s4mk1_control_names[] = {
	"1 (L)",
	"Shift (L)",
	"2 (L)",
	"Sync (L)",
	"3 (L)",
	"Cue (L)",
	"4 (L)",
	"Play (L)",

	"Deck Switch (L)",
	"Sample 1 (L)",
	"In (L)",
	"Sample 2 (L)",
	"Out (L)",
	"Sample 3 (L)",
	"Load (L)",
	"Sample 4 (L)",

	"Offset Up (L)",
	"Offset Down (L)",
	"Size Encoder Press (L)",
	"Move Encoder Press (L)",

	"Size",
	"Rec",
	"Undo",
	"Play",

	"Browse",
	"Snap",
	"Master",
	"Quant",

	"Cue (C)",
	"Cue (A)",
	"Cue (B)",
	"Cue (D)",

	"Browse Encoder Press",

	"Offset Up (R)",
	"Offset Down (R)",
	"Size Encoder Press (R)",
	"Move Encoder Press (R)",

	"Deck Switch (R)",
	"Sample 1 (R)",
	"In (R)",
	"Sample 2 (R)",
	"Out(R)",
	"Sample 3 (R)",
	"Load (R)",
	"Sample 4 (R)",

	"1 (R)",
	"Shift (R)",
	"2 (R)",
	"Sync (R)",
	"3 (R)",
	"Cue (R)",
	"4 (R)",
	"Play (R)",

	"Fx Power (L)",
	"Fx 1 (L)",
	"Fx 2 (L)",
	"Fx 3 (L)",

	"Fx Mode (L)",
	"Gain Encoder Press (C)",
	"Gain Encoder Press (A)",

	"Fx 1 (C)",
	"Fx 2 (C)",
	"Fx 1 (A)",
	"Fx 2 (A)",

	"Fx 1 (B)",
	"Fx 2 (B)",
	"Fx 1 (D)",
	"Fx 2 (D)",

	"Thru/USB (D)",
	"Phono (C)",
	"Phono (D)",

	"Fx Power (R)",
	"Fx 1 (R)",
	"Fx 2 (R)",
	"Fx 3 (R)",

	"Fx Mode (R)",
	"Gain Encoder Press (B)",
	"Gain Encoder Press (D)",
};
#define CONTROL_NAMES_SIZE (sizeof(ni_kontrol_s4mk1_control_names) /\
			    sizeof(ni_kontrol_s4mk1_control_names[0]))

static const struct ni_kontrol_s4mk1_ctlra_t buttons[] = {
	/* Deck A: */
	/* 1, shift, 2, sync */
	{0, 1 << 0},
	{0, 1 << 1},
	{0, 1 << 2},
	{0, 1 << 3},
	/* 3, cue, 4, play */
	{0, 1 << 4},
	{0, 1 << 5},
	{0, 1 << 6},
	{0, 1 << 7},

	/* deck switch, sample >, in, > */
	{1, 1 << 0},
	{1, 1 << 1},
	{1, 1 << 2},
	{1, 1 << 3},
	/* out, > , load,  > */
	{1, 1 << 4},
	{1, 1 << 5},
	{1, 1 << 6},
	{1, 1 << 7},

	/* Offset Up, Offset Down, Size Enc Press, Move Enc Press */
	{2, 1 << 0},
	{2, 1 << 1},
	{2, 1 << 2},
	{2, 1 << 3},
	/* Unused? value always shows 0x90 in read bytes
	{2, 1 << 4},
	{2, 1 << 5},
	{2, 1 << 6},
	{2, 1 << 7},
	 */

	/* Size, Rec, Undo, Play */
	{3, 1 << 0},
	{3, 1 << 1},
	{3, 1 << 2},
	{3, 1 << 3},
	/* Browse btn, Snap, Master, Quant */
	{3, 1 << 4},
	{3, 1 << 5},
	{3, 1 << 6},
	{3, 1 << 7},

	/* Cues: C A B D*/
	{4, 1 << 0},
	{4, 1 << 1},
	{4, 1 << 2},
	{4, 1 << 3},
	/* Browse Encoder Press */
	{4, 1 << 4},
	/* unused?
	{4, 1 << 5},
	{4, 1 << 6},
	{4, 1 << 7},
	*/

	/* Deck R: */
	/* Offset Up, Offset Down, Size Encoder Press, Move Enc Press */
	{5, 1 << 0},
	{5, 1 << 1},
	{5, 1 << 2},
	{5, 1 << 3},
	/* unused?
	{5, 1 << 4},
	{5, 1 << 5},
	{5, 1 << 6},
	{5, 1 << 7},
	*/

	/* Deck Switch, Samples 1, In, Samp 2 */
	{6, 1 << 0},
	{6, 1 << 1},
	{6, 1 << 2},
	{6, 1 << 3},
	/* Out, Samp 3, Load, Samp 4 */
	{6, 1 << 4},
	{6, 1 << 5},
	{6, 1 << 6},
	{6, 1 << 7},

	/* 1, shift, 2, sync */
	{7, 1 << 0},
	{7, 1 << 1},
	{7, 1 << 2},
	{7, 1 << 3},
	/* 3, cue, 4, play */
	{7, 1 << 4},
	{7, 1 << 5},
	{7, 1 << 6},
	{7, 1 << 7},

	/* 1 Unused?
	{8, 1 << 0},
	*/
	/* FX 1 on/off btn, 1, 2 */
	{8, 1 << 1},
	{8, 1 << 2},
	{8, 1 << 3},
	/* Mode, Gain Encoder Press (C), Gain Enc Press (A) */
	{8, 1 << 4},
	{8, 1 << 5},
	{8, 1 << 6},
	{8, 1 << 7},

	/* Fx 1 (C), Fx 2 (C), Fx 1 (A), Fx 2 (A) */
	{9, 1 << 0},
	{9, 1 << 1},
	{9, 1 << 2},
	{9, 1 << 3},
	/* Fx 1 (B), Fx 2 (B), Fx 1 (D), Fx 2 (D) */
	{9, 1 << 4},
	{9, 1 << 5},
	{9, 1 << 6},
	{9, 1 << 7},

	/* Back panel switches: thru channel D, phono C, phono D */
	/* Unused first 4 bits */
	{10, 1 << 5},
	{10, 1 << 6},
	{10, 1 << 7},

	/* 1 Unused?
	{11, 1 << 0},
	*/
	/* FX 1 on/off btn, 1, 2 */
	{11, 1 << 1},
	{11, 1 << 2},
	{11, 1 << 3},
	/* Mode, Gain Encoder Press (C), Gain Enc Press (A) */
	{11, 1 << 4},
	{11, 1 << 5},
	{11, 1 << 6},
	{11, 1 << 7},
};
#define BUTTONS_SIZE (sizeof(buttons) / sizeof(buttons[0]))

#define MK3_BTN (CTLRA_ITEM_BUTTON | CTLRA_ITEM_LED_INTENSITY | CTLRA_ITEM_HAS_FB_ID)
static struct ctlra_item_info_t buttons_info[] = {
	/* TODO: represent encoders better in the UI */
	/* encoder press, right up left down */
	{.x = 21, .y = 138, .w = 18,  .h = 10, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 0},
	{.x = 21, .y = 138, .w = 18,  .h = 10, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 0},
	{.x = 21, .y = 138, .w = 18,  .h = 10, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 0},
	{.x = 21, .y = 138, .w = 18,  .h = 10, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 0},
	{.x = 21, .y = 138, .w = 18,  .h = 10, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 0},
	/* shift, top-right above screen (mad ordering..) */
	{.x =  94, .y = 268, .w = 24, .h = 14, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 44},
	{.x = 278, .y =  10, .w = 24, .h =  8, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 19},
	/* group ABCDEFGH */
	{.x =  9, .y = 210, .w = 24,  .h = 14, .flags = MK3_BTN, .colour = 0xffffffff, .fb_id = 29},
	{.x = 37, .y = 210, .w = 24,  .h = 14, .flags = MK3_BTN, .colour = 0xffffffff, .fb_id = 30},
	{.x = 65, .y = 210, .w = 24,  .h = 14, .flags = MK3_BTN, .colour = 0xffffffff, .fb_id = 31},
	{.x = 94, .y = 210, .w = 24,  .h = 14, .flags = MK3_BTN, .colour = 0xffffffff, .fb_id = 32},
	{.x =  9, .y = 230, .w = 24,  .h = 14, .flags = MK3_BTN, .colour = 0xffffffff, .fb_id = 33},
	{.x = 37, .y = 230, .w = 24,  .h = 14, .flags = MK3_BTN, .colour = 0xffffffff, .fb_id = 34},
	{.x = 65, .y = 230, .w = 24,  .h = 14, .flags = MK3_BTN, .colour = 0xffffffff, .fb_id = 35},
	{.x = 94, .y = 230, .w = 24,  .h = 14, .flags = MK3_BTN, .colour = 0xffffffff, .fb_id = 36},
	/* notes volume swing tempo */
	{.x = 94, .y = 170, .w = 24,  .h = 8, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 28},
	{.x = 65, .y = 126, .w = 24,  .h = 8, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 20},
	{.x = 65, .y = 137, .w = 24,  .h = 8, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 21},
	{.x = 65, .y = 149, .w = 24,  .h = 8, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 23},
	/* Note Repeat, Lock */
	{.x = 93, .y = 126, .w = 24, .h = 20, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 22},
	{.x = 93, .y = 149, .w = 24, .h =  8, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 24},
	/* Pad mode, keyboard, chords, step */
	{.x = 169, .y = 126, .w = 32, .h = 8, .flags = MK3_BTN, .colour = 0xffffffff, .fb_id = 46},
	{.x = 205, .y = 126, .w = 32, .h = 8, .flags = MK3_BTN, .colour = 0xffffffff, .fb_id = 47},
	{.x = 241, .y = 126, .w = 32, .h = 8, .flags = MK3_BTN, .colour = 0xffffffff, .fb_id = 48},
	{.x = 277, .y = 126, .w = 32, .h = 8, .flags = MK3_BTN, .colour = 0xffffffff, .fb_id = 49},
	/* Fixed Vel, Scene, Pattern, Events */
	{.x = 137, .y = 126, .w = 24, .h =  8, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 45},
	{.x = 137, .y = 140, .w = 24, .h = 14, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 50},
	{.x = 137, .y = 158, .w = 24, .h = 14, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 51},
	{.x = 137, .y = 176, .w = 24, .h = 14, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 52},
	/* Variation, Dupliate, Select, Solo, Mute */
	{.x = 137, .y = 194, .w = 24, .h = 14, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 53},
	{.x = 137, .y = 212, .w = 24, .h = 14, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 54},
	{.x = 137, .y = 230, .w = 24, .h = 14, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 55},
	{.x = 137, .y = 248, .w = 24, .h = 14, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 56},
	{.x = 137, .y = 266, .w = 24, .h = 14, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 57},
	/* Pitch, Mod, Perform */
	{.x =  9, .y = 170, .w = 24, .h = 8, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 0},
	{.x = 37, .y = 170, .w = 24, .h = 8, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 26},
	{.x = 65, .y = 170, .w = 24, .h = 8, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 27},
	/* restart, erase, tap, follow */
	{.x =  9, .y = 250, .w = 24, .h = 8, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 37},
	{.x = 37, .y = 250, .w = 24, .h = 8, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 38},
	{.x = 65, .y = 250, .w = 24, .h = 8, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 39},
	{.x = 93, .y = 250, .w = 24, .h = 8, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 40},
	/* Play, Rec, Stop */
	{.x =  9, .y = 268, .w = 24, .h = 14, .flags = MK3_BTN, .colour = 0xff00ff00, .fb_id = 41},
	{.x = 37, .y = 268, .w = 24, .h = 14, .flags = MK3_BTN, .colour = 0xffff0000, .fb_id = 42},
	{.x = 65, .y = 268, .w = 24, .h = 14, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 43},
	/* Macro, settings, > sampling mixer plugin */
	{.x = 38, .y =  86, .w = 24, .h =  8, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 11},
	{.x = 38, .y =  74, .w = 24, .h =  8, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 9},
	{.x = 38, .y =  62, .w = 24, .h =  8, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 7},
	{.x = 38, .y =  44, .w = 24, .h = 14, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 5},
	{.x = 38, .y =  26, .w = 24, .h = 14, .flags = MK3_BTN, .colour = 0xffffffff, .fb_id = 3},
	{.x = 38, .y =  10, .w = 24, .h =  8, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 1},
	/* Channel, Arranger, Browser, <, File, Auto */
	{.x = 9, .y =  10, .w = 24, .h =  8, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 0},
	{.x = 9, .y =  26, .w = 24, .h = 14, .flags = MK3_BTN, .colour = 0xffffffff, .fb_id = 2},
	{.x = 9, .y =  44, .w = 24, .h = 14, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 4},
	{.x = 9, .y =  62, .w = 24, .h =  8, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 6},
	{.x = 9, .y =  74, .w = 24, .h =  8, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 8},
	{.x = 9, .y =  86, .w = 24, .h =  8, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 10},
	/* Top buttons (7, 8 already above) */
	{.x =  82, .y = 10, .w = 24,  .h = 8, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 12},
	{.x = 110, .y = 10, .w = 24,  .h = 8, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 13},
	{.x = 138, .y = 10, .w = 24,  .h = 8, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 14},
	{.x = 166, .y = 10, .w = 24,  .h = 8, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 15},
	{.x = 194, .y = 10, .w = 24,  .h = 8, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 16},
	{.x = 222, .y = 10, .w = 24,  .h = 8, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 17},
	{.x = 250, .y = 10, .w = 24,  .h = 8, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 18},
	{.x = 278, .y = 10, .w = 24,  .h = 8, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 19},
	/* TODO: show encoder dial touch control */
	/* big dial encoder touch */
	{.x = 1, .y = 1, .w = 1, .h = 1, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 0},
	/* 1-8 dial encoder touch */
	{.x = 1, .y = 1, .w = 1, .h = 1, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 0},
	{.x = 1, .y = 1, .w = 1, .h = 1, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 0},
	{.x = 1, .y = 1, .w = 1, .h = 1, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 0},
	{.x = 1, .y = 1, .w = 1, .h = 1, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 0},
	{.x = 1, .y = 1, .w = 1, .h = 1, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 0},
	{.x = 1, .y = 1, .w = 1, .h = 1, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 0},
	{.x = 1, .y = 1, .w = 1, .h = 1, .flags = MK3_BTN, .colour = 0xff000000, .fb_id = 0},
};

static struct ctlra_item_info_t sliders_info[] = {
	/* touchstrip */
	/* TODO: verify measurements - these are guesses */
	{.x = 9, .y =185, .w = 109,  .h = 14, .flags = CTLRA_ITEM_FADER},
};
#define SLIDERS_SIZE (1)

static struct ctlra_item_info_t feedback_info[] = {
	/* Screen */
	{.x =  88, .y =  33, .w = 94,  .h = 54, .flags = CTLRA_ITEM_FB_SCREEN,
		.params[0] = 480, .params[1] = 272},
	{.x = 200, .y =  33, .w = 94,  .h = 54, .flags = CTLRA_ITEM_FB_SCREEN,
		.params[0] = 480, .params[1] = 272},
	/* TODO: expose LED ids:
	 * up    : 58
	 * left  : 59
	 * right : 60
	 * down  : 61
	 * Strip : 62 -> 86
	 */
};
#define FEEDBACK_SIZE (sizeof(feedback_info) / sizeof(feedback_info[0]))

static const char *slider_names[] = {
	"Volume (D)",
	"Volume (B)",
	"Volume (A)",
	"Volume (C)",
	"Loop Record Volume",
	"Crossfader",

	"Pitch (R)",
	"Pitch (L)",
	"Mic Volume",
	"Cue Mix",

	/* Jog distances converted to buttons */
	"Filter (D)",
	"EQ Low (D)",
	"EQ Mid (D)",
	"EQ High (D)",

	"Fx 3 (R)",
	"Fx 2 (R)",
	"Fx 1 (R)",
	"Fx Dry/Wet (R)",

	"Filter (B)",
	"EQ Low (B)",
	"EQ Mid (B)",
	"EQ High (B)",

	"Filter (A)",
	"EQ Low (A)",
	"EQ Mid (A)",
	"EQ High (A)",

	"Filter (C)",
	"EQ Low (C)",
	"EQ Mid (C)",
	"EQ High (C)",

	"Fx 3 (L)",
	"Fx 2 (L)",
	"Fx 1 (L)",
	"Fx Dry/Wet (L)",
};
#define SLIDER_NAMES_SIZE (sizeof(slider_names) / sizeof(slider_names[0]))

static const char *encoder_names[] = {
	"Enc. Turn",
	"Enc. 1 Turn",
	"Enc. 2 Turn",
	"Enc. 3 Turn",
	"Enc. 4 Turn",
	"Enc. 5 Turn",
	"Enc. 6 Turn",
	"Enc. 7 Turn",
	"Enc. 8 Turn",
};

static struct ctlra_item_info_t encoder_info[] = {
	/* big one on the left */
	{.x = 20, .y = 128, .w = 28,  .h = 28, .flags = CTLRA_ITEM_ENCODER},
	/* 8 across */
	{.x = 85 + 29 * 0, .y = 98, .w = 18,  .h = 18, .flags = CTLRA_ITEM_ENCODER},
	{.x = 85 + 29 * 1, .y = 98, .w = 18,  .h = 18, .flags = CTLRA_ITEM_ENCODER},
	{.x = 85 + 29 * 2, .y = 98, .w = 18,  .h = 18, .flags = CTLRA_ITEM_ENCODER},
	{.x = 85 + 29 * 3, .y = 98, .w = 18,  .h = 18, .flags = CTLRA_ITEM_ENCODER},
	{.x = 85 + 29 * 4, .y = 98, .w = 18,  .h = 18, .flags = CTLRA_ITEM_ENCODER},
	{.x = 85 + 29 * 5, .y = 98, .w = 18,  .h = 18, .flags = CTLRA_ITEM_ENCODER},
	{.x = 85 + 29 * 6, .y = 98, .w = 18,  .h = 18, .flags = CTLRA_ITEM_ENCODER},
	{.x = 85 + 29 * 7, .y = 98, .w = 18,  .h = 18, .flags = CTLRA_ITEM_ENCODER},
};
#define ENCODERS_SIZE (sizeof(encoder_info) / sizeof(encoder_info[0]))

#define CONTROLS_SIZE (BUTTONS_SIZE + ENCODERS_SIZE)

#define LIGHTS_SIZE (62)
/* 25 + 16 bytes enough, but padding required for USB message */
#define LIGHTS_PADS_SIZE (80)

#define NPADS                  (16)
/* KERNEL_LENGTH must be a power of 2 for masking */
#define KERNEL_LENGTH          (8)
#define KERNEL_MASK            (KERNEL_LENGTH-1)



/* Represents the the hardware device */
struct ni_kontrol_s4mk1_t {
	/* base handles usb i/o etc */
	struct ctlra_dev_t base;
	/* current value of each controller is stored here */
	float hw_values[CONTROLS_SIZE];

	float slider_values[SLIDERS_SIZE];

	/* current state of the lights, only flush on dirty */
	uint8_t lights_dirty;

	/* Lights endpoint used to transfer with hidapi */
	uint8_t lights_endpoint;
	uint8_t lights[LIGHTS_SIZE];

	uint8_t lights_pads_endpoint;
	uint8_t lights_pads[LIGHTS_PADS_SIZE];
	uint8_t pad_colour;

	/* state of the pedal, according to the hardware */
	uint8_t pedal;

	uint8_t encoder_value;
	uint16_t touchstrip_value;
	/* Pressure filtering for note-onset detection */
	uint64_t pad_last_msg_time;
	uint16_t pad_hit;
	uint16_t pad_idx[NPADS];
	uint16_t pad_pressures[NPADS*KERNEL_LENGTH];
};

static const char *
ni_kontrol_s4mk1_control_get_name(enum ctlra_event_type_t type,
                                       uint32_t control_id)
{
	if(type == CTLRA_EVENT_BUTTON && control_id < CONTROL_NAMES_SIZE)
		return ni_kontrol_s4mk1_control_names[control_id];
	if(type == CTLRA_EVENT_ENCODER && control_id < 9)
		return encoder_names[control_id];
	if(type == CTLRA_EVENT_SLIDER && control_id < SLIDER_NAMES_SIZE)
		return slider_names[control_id];
	return 0;
}

static uint32_t ni_kontrol_s4mk1_poll(struct ctlra_dev_t *base)
{
	struct ni_kontrol_s4mk1_t *dev = (struct ni_kontrol_s4mk1_t *)base;
	uint8_t buf[512];
	int ret = ctlra_dev_impl_usb_bulk_read(base, USB_HANDLE_IDX,
				     USB_ENDPOINT_READ, buf, 512, 100);
	if(ret < 0) {
		printf("poll ret = %d\n", ret);
	}

	return 0;
}

static void
s4mk1_decode_buttons(struct ni_kontrol_s4mk1_t *dev, const uint8_t *data)
{
	for(int i = 0; i < BUTTONS_SIZE; i++) {
		/* if(i < 12) printf("btn %d, v %x\n", i, data[i+4]); */
		int offset = buttons[i].buf_byte_offset;
		int mask   = buttons[i].mask;

		uint16_t v = *((uint16_t *)&data[4 + offset]) & mask;
		int value_idx = i;

		if(dev->hw_values[value_idx] != v) {
			dev->hw_values[value_idx] = v;

			struct ctlra_event_t event = {
				.type = CTLRA_EVENT_BUTTON,
				.button  = {
					.id = i,
					.pressed = v > 0
				},
			};
			struct ctlra_event_t *e = {&event};
			dev->base.event_func(&dev->base, 1, &e,
					     dev->base.event_func_userdata);
		}
	}
}

/* iterates "count" sliders at indexes "idx_array[i]" */
static void
s4mk1_decode_sliders(struct ni_kontrol_s4mk1_t *dev, const uint8_t *data,
		     const uint8_t *idx_array, uint32_t count,
		     uint32_t ev_offset)
{
	for (int i = 0; i < count; i++) {
		uint32_t idx  = idx_array[i];
		uint16_t *d = (uint16_t *)&data[idx*2];
		uint16_t v_int = __bswap_16(*d);
		float v = (float)v_int / 4096.f;

		uint32_t slider_off = ev_offset + i;
		if(0.02 < fabs(v - dev->slider_values[slider_off])) {
			struct ctlra_event_t event = {
				.type = CTLRA_EVENT_SLIDER,
				.slider = {
					.id = i + ev_offset,
					.value = v,
				},
			};
			struct ctlra_event_t *e = {&event};
			dev->base.event_func(&dev->base, 1, &e,
					     dev->base.event_func_userdata);
			dev->slider_values[slider_off] = v;
		}

	}
}

void
ni_kontrol_s4mk1_usb_read_cb(struct ctlra_dev_t *base,
				  uint32_t endpoint, uint8_t *data,
				  uint32_t size)
{
	struct ni_kontrol_s4mk1_t *dev =
		(struct ni_kontrol_s4mk1_t *)base;
	static double worst_poll;
	int32_t nbytes = size;

	while (size > 0) {
		uint8_t blk = (data[0] << 8) | data[1];
		//printf("blk = %d\n", blk);

		switch(blk) {
		case 0: s4mk1_decode_buttons(dev, data); break;
		case 1: /* TODO: jog wheels + encoders */
			break;
		case 2: {/* vol d b a c, loop rec, crossfader */
			const uint8_t idxs[] = {1,2,3,4,5, 7 };
			s4mk1_decode_sliders(dev, data, idxs, 6, 0);
			} break;
		case 3: {
			/* pitch r, pitch l, mic vol, cue mix */
			const uint8_t idxs[] = {3,4, 6,7};
			s4mk1_decode_sliders(dev, data, idxs, 4, 6);
			} break;
		case 4: {
			// Jog Wheel controls on 1,2 - conver to buttons
			// D: filter, eq low, mid high FX-R 3
			const uint8_t idxs[] = {3,4,5,6,7};
			s4mk1_decode_sliders(dev, data, idxs, 5, 10);
			} break;
		case 5: {
			const uint8_t idxs[] = {1,2,3,4,5,6,7};
			s4mk1_decode_sliders(dev, data, idxs, 7, 15);
			} break;
		case 6: {
			const uint8_t idxs[] = {1,2,3,4,5,6,7};
			s4mk1_decode_sliders(dev, data, idxs, 7, 22);
			} break;
		case 7: {
			const uint8_t idxs[] = {1,2,3,4,5};
			s4mk1_decode_sliders(dev, data, idxs, 5, 29);
			} break;
		default:
			printf("invalid block %d, banishing!\n", blk);
			dev->base.banished = 1;
		};

		/* decrement size, move to next 16 bytes */
		size -= 16;
		data += 16;
	}
}

static void ni_kontrol_s4mk1_light_set(struct ctlra_dev_t *base,
                uint32_t light_id,
                uint32_t light_status)
{
	struct ni_kontrol_s4mk1_t *dev = (struct ni_kontrol_s4mk1_t *)base;
	int ret;

	if(!dev)
		return;

	// TODO: debug the -1, why is it required to get the right size?
	if(light_id > (LIGHTS_SIZE + 25 + 16) - 1)
		return;

	int idx = light_id;
	const uint8_t r = ((light_status >> 16) & 0xFF);
	const uint8_t g = ((light_status >>  8) & 0xFF);
	const uint8_t b = ((light_status >>  0) & 0xFF);

	uint8_t max = r > g ? r : g;
	max = b > max ? b : max;
	uint8_t min = r < g ? r : g;
	min = b < min ? b : min;

	/* rgb to hsv: nasty, but the device requires a H value input,
	 * so we have to calculate it here. Icky branchy divide-y... */
	uint8_t v = max;
	uint8_t h, s;
	if (v == 0 || (max - min) == 0) {
		h = 0;
		s = 0;
	} else {
		s = 255 * (max - min) / v;
		if (s == 0)
			h = 0;
		if (max == r)
			h = 0 + 43 * (g - b) / (max - min);
		else if (max == g)
			h = 85 + 43 * (b - r) / (max - min);
		else
			h = 171 + 43 * (r - g) / (max - min);
	}

	uint32_t bright = light_status >> 27;
	uint8_t hue = h / 16 + 1;

	/* if equal components, then set white */
	if(r == g && r == b)
		hue = 0xff;

	/* if the input was totally zero, set the LED off */
	if(light_status == 0)
		hue = 0;

	/* normal LEDs */
	if(idx < LIGHTS_SIZE) {
		switch(idx) {
		/* Sampling */
		case 5:
		/* ABCDEFGH */
		case 29: case 30: case 31: case 32:
		case 33: case 34: case 35: case 36:
		/* Encoder up, left, right, down */
		case 58: case 59: case 60: case 61:
		{
			uint8_t v = (hue << 2) | ((bright >> 2) & 0x3);
			dev->lights[idx] = v;
		} break;
		default:
			/* brighness 2 bits at the start of the
			 * uint8_t for the light */
			dev->lights[idx] = bright;
			break;
		};
		dev->lights_dirty = 1;
	} else {
		/* 25 strip + 16 pads */
		uint8_t v = (hue << 2) | (bright & 0x3);
		dev->lights_pads[idx - LIGHTS_SIZE] = v;
	}
}

void
ni_kontrol_s4mk1_light_flush(struct ctlra_dev_t *base, uint32_t force)
{
	struct ni_kontrol_s4mk1_t *dev = (struct ni_kontrol_s4mk1_t *)base;
	if(!dev->lights_dirty && !force)
		return;

	uint8_t *data = &dev->lights_endpoint;
	dev->lights_endpoint = 0x80;

#if 0
	/* error handling in USB subsystem */
	ctlra_dev_impl_usb_interrupt_write(base,
					   USB_HANDLE_IDX,
					   USB_ENDPOINT_WRITE,
					   data,
					   LIGHTS_SIZE + 1);

	data = &dev->lights_pads_endpoint;
	dev->lights_pads_endpoint = 0x81;
	ctlra_dev_impl_usb_interrupt_write(base,
					   USB_HANDLE_IDX,
					   USB_ENDPOINT_WRITE,
					   data,
					   LIGHTS_SIZE + 1);
#endif
	return;
}

static int32_t
ni_kontrol_s4mk1_disconnect(struct ctlra_dev_t *base)
{
	struct ni_kontrol_s4mk1_t *dev = (struct ni_kontrol_s4mk1_t *)base;

	memset(dev->lights, 0x0, LIGHTS_SIZE);
	dev->lights_dirty = 1;
	memset(dev->lights_pads, 0x0, LIGHTS_PADS_SIZE);

	if(!base->banished)
		ni_kontrol_s4mk1_light_flush(base, 1);

	ctlra_dev_impl_usb_close(base);
	free(dev);
	return 0;
}

struct ctlra_dev_info_t ctlra_ni_kontrol_s4mk1_info;

struct ctlra_dev_t *
ctlra_ni_kontrol_s4mk1_connect(ctlra_event_func event_func,
				    void *userdata, void *future)
{
	(void)future;
	struct ni_kontrol_s4mk1_t *dev =
		calloc(1,sizeof(struct ni_kontrol_s4mk1_t));
	if(!dev)
		goto fail;

	int err = ctlra_dev_impl_usb_open(&dev->base,
					  CTLRA_DRIVER_VENDOR,
					  CTLRA_DRIVER_DEVICE);
	if(err) {
		free(dev);
		return 0;
	}

	err = ctlra_dev_impl_usb_open_interface(&dev->base,
					 USB_INTERFACE_ID, USB_HANDLE_IDX);
	if(err) {
		printf("error opening interface\n");
		free(dev);
		return 0;
	}
		//kontrol_s4mk1_blit_to_screen(dev, 1);

	/* The S4 Mk1 has two "alternative settings" available, both on
	 * interface zero, see below for a graphical depcition. Note that
	 * the device needs to be in the alternate setting to emit its
	 * control values over the cable - so we must change it at start.
	 * Device (0xbaff)
	 *   - Interface 0:
	 *       -- Endpoint 01;
	 *       -- Endpoint 81;
	 *   - Interface 0: // NOT a typo, there are 2 "interface 0"'s
	 *       -- Endpoint 01;
	 *       -- Endpoint 81;
	 *       -- Endpoint 82;
	 *       -- Endpoint 84;
	 *       -- Endpoint 06;
	 *       -- Endpoint 08;
	 */
	int handle_idx = 0;
	int interface = 0;
	int alt_setting = 1;
	err = ctlra_dev_impl_usb_set_alt_setting(&dev->base, handle_idx,
						 interface, alt_setting);
	if(err)
		printf("s4mk1: error: cannot update alt setting: err = %d\n", err);

	dev->lights_dirty = 1;

	dev->base.info = ctlra_ni_kontrol_s4mk1_info;

	dev->base.poll = ni_kontrol_s4mk1_poll;
	dev->base.usb_read_cb = ni_kontrol_s4mk1_usb_read_cb;
	dev->base.disconnect = ni_kontrol_s4mk1_disconnect;
	dev->base.light_set = ni_kontrol_s4mk1_light_set;
	dev->base.light_flush = ni_kontrol_s4mk1_light_flush;

	dev->base.event_func = event_func;
	dev->base.event_func_userdata = userdata;

	return (struct ctlra_dev_t *)dev;
fail:
	free(dev);
	return 0;
}

struct ctlra_dev_info_t ctlra_ni_kontrol_s4mk1_info = {
	.vendor    = "Native Instruments",
	.device    = "Kontrol S4 Mk1",
	.vendor_id = CTLRA_DRIVER_VENDOR,
	.device_id = CTLRA_DRIVER_DEVICE,
	.size_x    = 320,
	.size_y    = 290,

#if 0
	.control_count[CTLRA_EVENT_BUTTON] = BUTTONS_SIZE,
	.control_info [CTLRA_EVENT_BUTTON] = buttons_info,

	.control_count[CTLRA_FEEDBACK_ITEM] = FEEDBACK_SIZE,
	.control_info [CTLRA_FEEDBACK_ITEM] = feedback_info,

	.control_count[CTLRA_EVENT_SLIDER] = SLIDERS_SIZE,
	.control_info [CTLRA_EVENT_SLIDER] = sliders_info,

	.control_count[CTLRA_EVENT_ENCODER] = ENCODERS_SIZE,
	.control_info [CTLRA_EVENT_ENCODER] = encoder_info,

	.control_count[CTLRA_EVENT_GRID] = 0,
#endif

	.get_name = ni_kontrol_s4mk1_control_get_name,
};

CTLRA_DEVICE_REGISTER(ni_kontrol_s4mk1)
