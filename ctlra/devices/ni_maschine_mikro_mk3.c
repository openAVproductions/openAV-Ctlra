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
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>

#include "ni_maschine_mikro_mk3.h"
#include "impl.h"

// Uncomment to debug pad on/off
//#define CTLRA_MIKRO_MK3_PADS 1
//#define CTLRA_MIKRO_MK3_PRESSURE_DEBUG 1

#define CTLRA_DRIVER_VENDOR (0x17cc)
#define CTLRA_DRIVER_DEVICE (0x1700)
#define USB_INTERFACE_ID      (0x00)
#define USB_HANDLE_IDX        (0x00)
#define USB_ENDPOINT_READ     (0x81)
#define USB_ENDPOINT_WRITE    (0x01)

#define LIGHTS_ENDPOINT       (0x80)
#define LIGHT_PADS_ENDPOINT   (0x81)

/* This struct is a generic struct to identify hw controls */
struct ni_maschine_mikro_mk3_ctlra_t {
	int event_id;
	int buf_byte_offset;
	uint32_t mask;
};

static const char *ni_maschine_mikro_mk3_control_names[] = {
	"Native Instruments",
	"Star",
	"Browse",
	"Volume",
	"Swing",
	"Tempo",
	"Plug-in",
	"Sampling",
    "Left Arrow",
    "Right Arrow",
    "Pitch",
    "Mod",
    "Perform",
    "Notes",
    "Group",
    "Auto",
    "Lock",
    "Note Repeat",
    "Restart",
    "Erase",
    "Tap",
    "Follow",
    "Play",
    "Rec",
    "Stop",
    "Shift",
    "Fixed Vel.",
    "Pad Mode",
    "Keyboard",
    "Chords",
    "Step",
    "Scene",
    "Pattern",
    "Events",
    "Variations",
    "Duplicate",
    "Select",
    "Solo",
    "Mute",
};
#define CONTROL_NAMES_SIZE (sizeof(ni_maschine_mikro_mk3_control_names) /\
			    sizeof(ni_maschine_mikro_mk3_control_names[0]))

static const struct ni_maschine_mikro_mk3_ctlra_t buttons[] = {
	/* encoder */
	{NI_MASCHINE_MIKRO_MK3_BTN_NATIVE_INSTRUMENTS,  1, 0x01},
	{NI_MASCHINE_MIKRO_MK3_BTN_STAR,                1, 0x02},
	{NI_MASCHINE_MIKRO_MK3_BTN_SEARCH,              1, 0x04},
	{NI_MASCHINE_MIKRO_MK3_BTN_VOLUME,              1, 0x08},
	{NI_MASCHINE_MIKRO_MK3_BTN_SWING,               1, 0x10},
	{NI_MASCHINE_MIKRO_MK3_BTN_TEMPO,               1, 0x20},
	{NI_MASCHINE_MIKRO_MK3_BTN_PLUG_IN,             1, 0x40},
	{NI_MASCHINE_MIKRO_MK3_BTN_SAMPLING,            1, 0x80},
    {NI_MASCHINE_MIKRO_MK3_BTN_LEFT_ARROW,          2, 0x01},
    {NI_MASCHINE_MIKRO_MK3_BTN_RIGHT_ARROW,         2, 0x02},
    {NI_MASCHINE_MIKRO_MK3_BTN_PITCH,               2, 0x04},
    {NI_MASCHINE_MIKRO_MK3_BTN_MOD,                 2, 0x08},
    {NI_MASCHINE_MIKRO_MK3_BTN_PERFORM,             2, 0x10},
    {NI_MASCHINE_MIKRO_MK3_BTN_NOTES,               2, 0x20},
    {NI_MASCHINE_MIKRO_MK3_BTN_GROUP,               2, 0x40},
    {NI_MASCHINE_MIKRO_MK3_BTN_AUTO,                2, 0x80},
    {NI_MASCHINE_MIKRO_MK3_BTN_LOCK,                3, 0x01},
    {NI_MASCHINE_MIKRO_MK3_BTN_NOTE_REPEAT,         3, 0x02},
    {NI_MASCHINE_MIKRO_MK3_BTN_RESTART,             3, 0x04},
    {NI_MASCHINE_MIKRO_MK3_BTN_ERASE,               3, 0x08},
    {NI_MASCHINE_MIKRO_MK3_BTN_TAP,                 3, 0x10},
    {NI_MASCHINE_MIKRO_MK3_BTN_FOLLOW,              3, 0x20},
    {NI_MASCHINE_MIKRO_MK3_BTN_PLAY,                3, 0x40},
    {NI_MASCHINE_MIKRO_MK3_BTN_RECORD,              3, 0x80},
    {NI_MASCHINE_MIKRO_MK3_BTN_STOP,                4, 0x01},
    {NI_MASCHINE_MIKRO_MK3_BTN_SHIFT,               4, 0x02},
    {NI_MASCHINE_MIKRO_MK3_BTN_FIXED_VEL,           4, 0x04},
    {NI_MASCHINE_MIKRO_MK3_BTN_PAD_MODE,            4, 0x08},
    {NI_MASCHINE_MIKRO_MK3_BTN_KEYBOARD,            4, 0x10},
    {NI_MASCHINE_MIKRO_MK3_BTN_CHORDS,              4, 0x20},
    {NI_MASCHINE_MIKRO_MK3_BTN_STEP,                4, 0x40},
    {NI_MASCHINE_MIKRO_MK3_BTN_SCENE,               4, 0x80},
    {NI_MASCHINE_MIKRO_MK3_BTN_PATTERN,             5, 0x01},
    {NI_MASCHINE_MIKRO_MK3_BTN_EVENTS,              5, 0x02},
    {NI_MASCHINE_MIKRO_MK3_BTN_VARIATION,           5, 0x04},
    {NI_MASCHINE_MIKRO_MK3_BTN_DUPLICATE,           5, 0x08},
    {NI_MASCHINE_MIKRO_MK3_BTN_SELECT,              5, 0x10},
    {NI_MASCHINE_MIKRO_MK3_BTN_SOLO,                5, 0x20},
    {NI_MASCHINE_MIKRO_MK3_BTN_MUTE,                5, 0x40},
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

static struct ctlra_item_info_t encoder_info[] = {
	/* big one on the left */
	{.x = 20, .y = 128, .w = 28,  .h = 28, .flags = CTLRA_ITEM_ENCODER},
};

#define SLIDERS_SIZE (1)
#define ENCODERS_SIZE (1)

#define CONTROLS_SIZE (BUTTONS_SIZE + ENCODERS_SIZE)

#define LIGHTS_SIZE (80)
/* 25 + 16 bytes enough, but padding required for USB message */
#define LIGHTS_PADS_SIZE (80)

#define NPADS                  (16)
/* KERNEL_LENGTH must be a power of 2 for masking */
#define KERNEL_LENGTH          (8)
#define KERNEL_MASK            (KERNEL_LENGTH-1)


/* TODO: Refactor out screen impl, and push to ctlra_ni_screen.h ? */
/* Screen blit commands - no need to have publicly in header */
static const uint8_t header_right[] = {
	0x84,  0x0, 0x01, 0x60,
	0x0,  0x0, 0x0,  0x0,
	0x0,  0x0, 0x0,  0x0,
	0x1, 0xe0, 0x1, 0x10,
};
static const uint8_t header_left[] = {
	0x84,  0x0, 0x00, 0x60,
	0x0,  0x0, 0x0,  0x0,
	0x0,  0x0, 0x0,  0x0,
	0x1, 0xe0, 0x1, 0x10,
};
static const uint8_t command[] = {
	/* num_px/2: 0xff00 is the (total_px/2) */
	0x00, 0x0, 0xff, 0x00,
};
static const uint8_t footer[] = {
	0x03, 0x00, 0x00, 0x00,
	0x40, 0x00, 0x00, 0x00
};
/* 565 encoding, hence 2 bytes per px */
#define NUM_PX (480 * 272)
struct ni_screen_t {
	uint8_t header [sizeof(header_left)];
	uint8_t command[sizeof(command)];
	uint16_t pixels [NUM_PX]; // 565 uses 2 bytes per pixel
	uint8_t footer [sizeof(footer)];
};

/* Represents the the hardware device */
struct ni_maschine_mikro_mk3_t {
	/* base handles usb i/o etc */
	struct ctlra_dev_t base;
	/* current value of each controller is stored here */
	float hw_values[CONTROLS_SIZE];
	/* current state of the lights, only flush on dirty */
	uint8_t lights_dirty;
	uint8_t lights_pads_dirty;

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

	struct ni_screen_t screen_left;
	struct ni_screen_t screen_right;
};

static const char *
ni_maschine_mikro_mk3_control_get_name(enum ctlra_event_type_t type,
                                       uint32_t control_id)
{
	if(type == CTLRA_EVENT_BUTTON && control_id < CONTROL_NAMES_SIZE)
		return ni_maschine_mikro_mk3_control_names[control_id];
	if(type == CTLRA_EVENT_ENCODER)
		return "Encoder";
	if(type == CTLRA_EVENT_SLIDER && control_id == 0)
		return "Touchstrip";
	return 0;
}

static uint32_t ni_maschine_mikro_mk3_poll(struct ctlra_dev_t *base)
{
	struct ni_maschine_mikro_mk3_t *dev = (struct ni_maschine_mikro_mk3_t *)base;
	uint8_t buf[128];
	ctlra_dev_impl_usb_interrupt_read(base, USB_HANDLE_IDX,
					  USB_ENDPOINT_READ,
					  buf, sizeof(buf));
	return 0;
}

void
ni_maschine_mikro_mk3_light_flush(struct ctlra_dev_t *base, uint32_t force);

/* ABCDEFGH Pad colour */
static const uint8_t pad_cols[] = {
	0x2a, 0b11101, 0b11000011, 0x5e,
	0b11011,
	0b1111,
	0b1011,
	0b101,
};

static void
ni_maschine_mikro_mk3_pads_decode_set(struct ni_maschine_mikro_mk3_t *dev,
				uint8_t *buf,
				uint8_t msg_idx)
{
	/* This function decodes a single 64 byte pads message. See
	 * comments in calling code to understand how sets work */
	/* Template event */
	struct ctlra_event_t event = {
		.type = CTLRA_EVENT_GRID,
		.grid  = {
			.id = 0,
			.flags = CTLRA_EVENT_GRID_FLAG_BUTTON,
			.pos = 0,
			.pressed = 1
		},
	};
	struct ctlra_event_t *e = {&event};

	/* pre-process pressed pads into bitmask. Keep state from before,
	 * the messages will update only those that have changed */
	uint16_t pad_pressures[16] = {0};
	uint16_t rpt_pressed = dev->pad_hit;
	int flush_lights = 0;
	uint8_t d1, d2;
	int i;
	for(i = 0; i < 16; i++) {
		/* skip over pressure values */
		uint8_t p = buf[1+i*3];
		d1 = buf[2+i*3];
		d2 = buf[3+i*3];

		/* pad number is zero when list of pads has ended */
		if(p == 0 && d1 == 0)
			break;

		/* software threshold for gentle release */
		uint16_t pressure = ((d1 & 0xf) << 8) | d2;
		if(pressure > 128)
			rpt_pressed |= 1 << p;
		else
			rpt_pressed &= ~(1 << p);

		/* store pressure value for setting in event later */
		pad_pressures[p] = pressure;
	}

	for(int i = 0; i < 16; i++) {
		/* detect state change */
		int current = (dev->pad_hit & (1 << i));
		int new     = (rpt_pressed  & (1 << i));

		int state_change = (current != new);
		int pressure_change = pad_pressures[i] != dev->pad_pressures[i];

#ifdef CTLRA_MIKRO_MK3_PRESSURE_DEBUG
		printf("[msg_idx:%d]: pad %2d state (CH %d, V %d) pressure(CH %d, V %d)\n",
		       msg_idx, i, state_change, new > 0, pressure_change,
		       pad_pressures[i]);
#endif

		/* Pressure value */
		if(current == new) {
			continue;
		}

		/* rotate grid to match order on device (but zero
		 * based counting instead of 1 based). */
		event.grid.pos = (3-(i/4))*4 + (i%4);
		int press = new > 0;
		event.grid.pressed = press;
		event.grid.pressure = pad_pressures[i] * (1 / 4096.f) * press;

		dev->base.event_func(&dev->base, 1, &e,
				     dev->base.event_func_userdata);
#ifdef CTLRA_MIKRO_MK3_PADS
		dev->lights_pads[25+i] = dev->pad_colour * event.grid.pressed;
		ni_maschine_mikro_mk3_light_flush(&dev->base, 1);
#endif
	}

	dev->pad_hit = rpt_pressed;
}

static void
ni_maschine_mikro_mk3_pads(struct ni_maschine_mikro_mk3_t *dev,
		     uint8_t *buf)
{
	/* decode the mk3 pads:
	 * - The data format is interesting - requires some knowledge to
	 *   decode the pad states.
	 * - The data is "double-pumped" - each 128 byte pads message
	 *   contains two sets of the pad data. This suggests that the
	 *   hardware internally samples the pad ADCs (at least) twice the
	 *   maximum USB xfer delta. The first set is referred to as set A,
	 *   and the second set is known as B for the rest of the code.
	 * - Providing both sets of pad data allows smoother pressure
	 *   curves to be generated, using some interpolation and guessing
	 *   of the timestamps.
	 * - Both messages (Set A and B) can contain note-on and note-off
	 *   messages - a correct implementation *must* decode both in
	 *   order to not drop events.
	 * - Somehow, the note-on/off events are *usually* in set A, but
	 *   faster / more-busy passages start dropping notes a lot. This
	 *   is from experience - there is no real logic as to why..?
	 * - The commented code below prints the two messages side-by-side
	 *   making it easy to spot differences. An example below:
	 *     Eg:       02 02 01 01 41 30 75 00
	 *     Set A:    02    01    41    75
	 *     Set B:       02    01    30    00
	 *   Not taking B into account would cause the note-off to be
	 *   dropped.
	 */
#ifdef CTLRA_MIKRO_MK3_PADS
	for(int i = 0; i < 64; i++) {
		printf("%02x ", buf[i]);
		printf("%02x ", buf[i+64]);
		if(i > 1 && buf[i] == 0)
			break;
	}
	printf("\n");
#endif
	/* call for Set A, then again for set B */
	ni_maschine_mikro_mk3_pads_decode_set(dev, &buf[0], 0);
	ni_maschine_mikro_mk3_pads_decode_set(dev, &buf[64], 1);
};

void
ni_maschine_mikro_mk3_usb_read_cb(struct ctlra_dev_t *base,
				  uint32_t endpoint, uint8_t *data,
				  uint32_t size)
{
	struct ni_maschine_mikro_mk3_t *dev = (struct ni_maschine_mikro_mk3_t *)base;
	int32_t nbytes = size;

	int count = 0;

//    DEBUG
//    printf("%s : size = %d\n", __func__, size);
//    for(int i = 0; i < size; i++) {
//        printf("%02x ", data[i]);
//    }
//    printf("\n");

	uint8_t *buf = data;

    switch(nbytes) {
//	case 81: {
//		/* Return of LED state, after update written to device */
//		} break;
	case 128:
		ni_maschine_mikro_mk3_pads(dev, data);
		break;
	case 14: {
		/* touchstrip: dont send event if 0, as this is release */
//		uint16_t v = *((uint16_t *)&buf[30]);
//		if(v && v != dev->touchstrip_value) {
//			struct ctlra_event_t event = {
//				.type = CTLRA_EVENT_SLIDER,
//				.slider = {
//					.id = 0,
//					.value = v / 1024.f,
//				},
//			};
//			struct ctlra_event_t *e = {&event};
//			dev->base.event_func(&dev->base, 1, &e,
//					     dev->base.event_func_userdata);
//			dev->touchstrip_value = v;
//		}

		/* Buttons */
		for(uint32_t i = 0; i < BUTTONS_SIZE; i++) {
			int id     = buttons[i].event_id;
			int offset = buttons[i].buf_byte_offset;
			int mask   = buttons[i].mask;

			uint16_t v = *((uint16_t *)&buf[offset]) & mask;
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

		/* Main Encoder */
		int8_t enc   = buf[11] & 0x0f;
		if(enc != dev->encoder_value) {
			int dir = ctlra_dev_encoder_wrap_16(enc, dev->encoder_value);
			dev->encoder_value = enc;

            struct ctlra_event_t event = {
                    .type = CTLRA_EVENT_ENCODER,
                    .encoder = {
                            .id = 0,
                            .flags = CTLRA_EVENT_ENCODER_FLAG_INT,
                            .delta = 0,
                    },
            };
            struct ctlra_event_t *e = {&event};
            event.encoder.delta = dir;
			dev->base.event_func(&dev->base, 1, &e,
					     dev->base.event_func_userdata);
		}
		break;
		} /* case 42: buttons */
	}
}

static void ni_maschine_mikro_mk3_light_set(struct ctlra_dev_t *base,
                uint32_t light_id,
                uint32_t light_status)
{
	struct ni_maschine_mikro_mk3_t *dev = (struct ni_maschine_mikro_mk3_t *)base;
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
		dev->lights_pads_dirty = 1;
	}
}

void
ni_maschine_mikro_mk3_light_flush(struct ctlra_dev_t *base, uint32_t force)
{
	struct ni_maschine_mikro_mk3_t *dev = (struct ni_maschine_mikro_mk3_t *)base;
	if(!dev->lights_dirty && !force)
		return;

	uint8_t *data = &dev->lights_endpoint;
	dev->lights_endpoint = LIGHTS_ENDPOINT;

	/* error handling in USB subsystem */
	ctlra_dev_impl_usb_interrupt_write(base,
					   USB_HANDLE_IDX,
					   USB_ENDPOINT_WRITE,
					   data,
					   LIGHTS_SIZE + 1);

	data = &dev->lights_pads_endpoint;
	dev->lights_pads_endpoint = LIGHT_PADS_ENDPOINT;
	ctlra_dev_impl_usb_interrupt_write(base,
					   USB_HANDLE_IDX,
					   USB_ENDPOINT_WRITE,
					   data,
					   LIGHTS_SIZE + 1);
}

static int32_t
ni_maschine_mikro_mk3_disconnect(struct ctlra_dev_t *base)
{
	struct ni_maschine_mikro_mk3_t *dev = (struct ni_maschine_mikro_mk3_t *)base;

	memset(dev->lights, 0x0, LIGHTS_SIZE);
	dev->lights_dirty = 1;
	memset(dev->lights_pads, 0x0, LIGHTS_PADS_SIZE);
	dev->lights_pads_dirty = 1;

	if(!base->banished) {
		ni_maschine_mikro_mk3_light_flush(base, 1);
		memset(dev->screen_left.pixels, 0x0,
		       sizeof(dev->screen_left.pixels));
		memset(dev->screen_right.pixels, 0x0,
		       sizeof(dev->screen_right.pixels));
	}

	ctlra_dev_impl_usb_close(base);
	free(dev);
	return 0;
}

struct ctlra_dev_info_t ctlra_ni_maschine_mikro_mk3_info;

struct ctlra_dev_t *
ctlra_ni_maschine_mikro_mk3_connect(ctlra_event_func event_func,
				    void *userdata, void *future)
{
	(void)future;
	struct ni_maschine_mikro_mk3_t *dev =
		calloc(1,sizeof(struct ni_maschine_mikro_mk3_t));
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

	dev->pad_colour = pad_cols[0];
	dev->lights_dirty = 1;

	dev->base.info = ctlra_ni_maschine_mikro_mk3_info;

	dev->base.poll = ni_maschine_mikro_mk3_poll;
	dev->base.usb_read_cb = ni_maschine_mikro_mk3_usb_read_cb;
	dev->base.disconnect = ni_maschine_mikro_mk3_disconnect;
	dev->base.light_set = ni_maschine_mikro_mk3_light_set;
	dev->base.light_flush = ni_maschine_mikro_mk3_light_flush;

	dev->base.event_func = event_func;
	dev->base.event_func_userdata = userdata;

	return (struct ctlra_dev_t *)dev;
fail:
	free(dev);
	return 0;
}

struct ctlra_dev_info_t ctlra_ni_maschine_mikro_mk3_info = {
	.vendor    = "Native Instruments",
	.device    = "Maschine Mikro Mk3",
	.vendor_id = CTLRA_DRIVER_VENDOR,
	.device_id = CTLRA_DRIVER_DEVICE,
	.size_x    = 320,
	.size_y    = 290,

	.control_count[CTLRA_EVENT_BUTTON] = BUTTONS_SIZE,
	.control_info [CTLRA_EVENT_BUTTON] = buttons_info,

//	.control_count[CTLRA_FEEDBACK_ITEM] = FEEDBACK_SIZE,
//	.control_info [CTLRA_FEEDBACK_ITEM] = feedback_info,

	.control_count[CTLRA_EVENT_SLIDER] = SLIDERS_SIZE,
	.control_info [CTLRA_EVENT_SLIDER] = sliders_info,

	.control_count[CTLRA_EVENT_ENCODER] = ENCODERS_SIZE,
	.control_info [CTLRA_EVENT_ENCODER] = encoder_info,

#if 1
	.control_count[CTLRA_EVENT_GRID] = 1,
	.grid_info[0] = {
		.rgb = 1,
		.velocity = 1,
		.pressure = 1,
		.x = 4,
		.y = 4,
		.info = {
			.x = 168,
			.y = 140,
			.w = 138,
			.h = 138,
			/* start light id */
			.params[0] = 87,
			/* end light id */
			.params[1] = 87 + 16,
		}
	},
#endif

	.get_name = ni_maschine_mikro_mk3_control_get_name,
};

CTLRA_DEVICE_REGISTER(ni_maschine_mikro_mk3)
