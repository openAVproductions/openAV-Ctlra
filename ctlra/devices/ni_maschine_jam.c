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

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "impl.h"
#include "ni_maschine_jam.h"

#define CTLRA_DRIVER_VENDOR (0x17cc)
#define CTLRA_DRIVER_DEVICE (0x1500)
#define USB_HANDLE_IDX     (0x0)
#define USB_INTERFACE_ID   (0x0)
#define USB_ENDPOINT_READ  (0x81)
#define USB_ENDPOINT_WRITE (0x01)

/* This struct is a generic struct to identify hw controls */
struct ni_maschine_jam_ctlra_t {
	int event_id;
	int buf_byte_offset;
	uint32_t mask;
};

// WIP: we can use HIDRAW or LibUSB backends
//#define USE_LIBUSB 1

static const char *ni_maschine_jam_control_names[] = {
	/* Faders / Dials */
	"Song",
	"Step",
	"Pad Mode",
	"Clear",
	"Duplicate",
	"DPad Up",
	"DPad Left",
	"DPad Right",
	"DPad Down",
	"Note Repeat",
	"Macro",
	"Level",
	"Aux",
	"Control",
	"Auto",
	"Shift",
	"Play",
	"Rec",
	"Left",
	"Right",
	"Tempo",
	"Grid",
	"Solo",
	"Mute",
	"Select",
	"Swing",
	"Tune",
	"Lock",
	"Notes",
	"Perform",
	"Browse",
	"Encoder Touch",
	"Encoder Press",
	"In",
	"Headphones",
	"Master (MST)",
	"Group (GRP)",
	"1",
	"2",
	"3",
	"4",
	"5",
	"6",
	"7",
	"8",
	"A",
	"B",
	"C",
	"D",
	"E",
	"F",
	"G",
	"H",
	"Footswitch",
	"Strip 1",
	"Strip 2",
	"Strip 3",
	"Strip 4",
	"Strip 5",
	"Strip 6",
	"Strip 7",
	"Strip 8",
};
#define CONTROL_NAMES_SIZE (sizeof(ni_maschine_jam_control_names) /\
			    sizeof(ni_maschine_jam_control_names[0]))


#define JAM_BTN (CTLRA_ITEM_BUTTON | CTLRA_ITEM_LED_INTENSITY | \
		 CTLRA_ITEM_LED_COLOR | CTLRA_ITEM_HAS_FB_ID)
static struct ctlra_item_info_t buttons_info[] = {
	/* song, step, pad mode, clear, duplicate */
	{.x = 10, .y = 20, .w = 20,  .h = 6, .colour = 0xff000000, .flags = JAM_BTN, .fb_id = 1},
	{.x = 10, .y = 39, .w = 20,  .h =10, .colour = 0xff000000, .flags = JAM_BTN, .fb_id = 2},
	{.x = 10, .y = 54, .w = 20,  .h =10, .colour = 0xff000000, .flags = JAM_BTN, .fb_id = 3},
	{.x = 10, .y = 68, .w = 20,  .h =10, .colour = 0xff000000, .flags = JAM_BTN, .fb_id = 4},
	{.x = 10, .y = 82, .w = 20,  .h =10, .colour = 0xff000000, .flags = JAM_BTN, .fb_id = 5},
	/* Up Left Right Down */
	{.x = 16, .y = 120, .w = 8,  .h = 8, .colour = 0xff000000, .flags = JAM_BTN, .fb_id = 6},
	{.x =  6, .y = 130, .w = 8,  .h = 8, .colour = 0xff000000, .flags = JAM_BTN, .fb_id = 7},
	{.x = 26, .y = 130, .w = 8,  .h = 8, .colour = 0xff000000, .flags = JAM_BTN, .fb_id = 8},
	{.x = 16, .y = 140, .w = 8,  .h = 8, .colour = 0xff000000, .flags = JAM_BTN, .fb_id = 9},
	/* Note Repeat, macro, level, aux */
	{.x = 10, .y = 160, .w = 20,  .h = 10, .colour = 0xff000000, .flags = JAM_BTN, .fb_id = 10},
	{.x = 10, .y = 184, .w = 20,  .h = 6, .colour = 0xff000000, .flags = JAM_BTN, .fb_id = 18},
	{.x = 10, .y = 196, .w = 20,  .h = 6, .colour = 0xff000000, .flags = JAM_BTN, .fb_id = 19},
	{.x = 10, .y = 209, .w = 20,  .h = 6, .colour = 0xff000000, .flags = JAM_BTN, .fb_id = 20},
	/* control, auto, shift */
	{.x = 10, .y = 228, .w = 20,  .h = 6, .colour = 0xff000000, .flags = JAM_BTN, .fb_id = 21},
	{.x = 10, .y = 240, .w = 20,  .h = 6, .colour = 0xff000000, .flags = JAM_BTN, .fb_id = 22},
	{.x = 10, .y = 260, .w = 20,  .h = 10, .colour = 0xff000000, .flags = JAM_BTN, .fb_id = 28},
	/* play rec < > */
	{.x =  50, .y = 260, .w = 20,  .h = 10, .colour = 0xff00ff00, .flags = JAM_BTN, .fb_id = 29},
	{.x =  78, .y = 260, .w = 20,  .h = 10, .colour = 0xffff0000, .flags = JAM_BTN, .fb_id = 30},
	{.x = 107, .y = 260, .w = 20,  .h = 10, .colour = 0xff000000, .flags = JAM_BTN, .fb_id = 31},
	{.x = 136, .y = 260, .w = 20,  .h = 10, .colour = 0xff000000, .flags = JAM_BTN, .fb_id = 32},
	/* Tempo, Grid, Solo, Mute */
	{.x = 164, .y = 260, .w = 20,  .h = 10, .colour = 0xff000000, .flags = JAM_BTN, .fb_id = 33},
	{.x = 192, .y = 260, .w = 20,  .h = 10, .colour = 0xff000000, .flags = JAM_BTN, .fb_id = 34},
	{.x = 221, .y = 260, .w = 20,  .h = 10, .colour = 0xff000000, .flags = JAM_BTN, .fb_id = 35},
	{.x = 250, .y = 260, .w = 20,  .h = 10, .colour = 0xff000000, .flags = JAM_BTN, .fb_id = 36},
	/* Select, Swing, Tune, Lock */
	{.x = 290, .y = 260, .w = 20,  .h = 10, .colour = 0xff000000, .flags = JAM_BTN, .fb_id = 37},
	{.x = 290, .y = 240, .w = 20,  .h = 6, .colour = 0xff000000, .flags = JAM_BTN, .fb_id = 27},
	{.x = 290, .y = 227, .w = 20,  .h = 6, .colour = 0xff000000, .flags = JAM_BTN, .fb_id = 26},
	/* Lock, notes, perform, browse */
	{.x = 290, .y = 208, .w = 20,  .h = 6, .colour = 0xff000000, .flags = JAM_BTN, .fb_id = 25},
	{.x = 290, .y = 196, .w = 20,  .h = 6, .colour = 0xff000000, .flags = JAM_BTN, .fb_id = 24},
	{.x = 290, .y = 184, .w = 20,  .h = 6, .colour = 0xff000000, .flags = JAM_BTN, .fb_id = 23},
	{.x = 290, .y = 160, .w = 20,  .h = 10, .colour = 0xff000000, .flags = JAM_BTN, .fb_id = 17},
	/* Encoder Touch, press, etc */
	{.x = 290, .y = 120, .w = 20,  .h = 6, .colour = 0xff000000, .flags = CTLRA_ITEM_BUTTON},
	{.x = 290, .y = 148, .w = 20,  .h = 6, .colour = 0xff000000, .flags = CTLRA_ITEM_BUTTON},
	/* In 1, Headphone, Mst, Grp */
	{.x = 290, .y = 100, .w = 6,  .h = 6, .colour = 0xff0000ff, .flags = JAM_BTN, .fb_id = 13},
	{.x = 304, .y = 100, .w = 6,  .h = 6, .colour = 0xff000000, .flags = JAM_BTN, .fb_id = 15},
	{.x = 290, .y =  82, .w = 6,  .h = 6, .colour = 0xff000000, .flags = JAM_BTN, .fb_id = 11},
	{.x = 304, .y =  82, .w = 6,  .h = 6, .colour = 0xff000000, .flags = JAM_BTN, .fb_id = 12},
	/* 1,2,3,4 */
	{.x =  50, .y = 20, .w = 20,  .h = 6, .colour = 0xff000000, .flags = JAM_BTN, .fb_id = 0},
	{.x =  78, .y = 20, .w = 20,  .h = 6, .colour = 0xff000000, .flags = JAM_BTN, .fb_id = 0},
	{.x = 107, .y = 20, .w = 20,  .h = 6, .colour = 0xff000000, .flags = JAM_BTN, .fb_id = 0},
	{.x = 136, .y = 20, .w = 20,  .h = 6, .colour = 0xff000000, .flags = JAM_BTN, .fb_id = 0},
	/* 5,6,7,8 */
	{.x = 164, .y = 20, .w = 20,  .h = 6, .colour = 0xff000000, .flags = JAM_BTN, .fb_id = 0},
	{.x = 192, .y = 20, .w = 20,  .h = 6, .colour = 0xff000000, .flags = JAM_BTN, .fb_id = 0},
	{.x = 221, .y = 20, .w = 20,  .h = 6, .colour = 0xff000000, .flags = JAM_BTN, .fb_id = 0},
	{.x = 250, .y = 20, .w = 20,  .h = 6, .colour = 0xff000000, .flags = JAM_BTN, .fb_id = 0},
	/* a, b, c, d */
	{.x =  50, .y = 160, .w = 20,  .h = 10, .colour = 0xff000000, .flags = JAM_BTN, .fb_id = 0},
	{.x =  78, .y = 160, .w = 20,  .h = 10, .colour = 0xff000000, .flags = JAM_BTN, .fb_id = 0},
	{.x = 107, .y = 160, .w = 20,  .h = 10, .colour = 0xff000000, .flags = JAM_BTN, .fb_id = 0},
	{.x = 136, .y = 160, .w = 20,  .h = 10, .colour = 0xff000000, .flags = JAM_BTN, .fb_id = 0},
	/* e,f,g,h */
	{.x = 164, .y = 160, .w = 20,  .h = 10, .colour = 0xff000000, .flags = JAM_BTN, .fb_id = 0},
	{.x = 192, .y = 160, .w = 20,  .h = 10, .colour = 0xff000000, .flags = JAM_BTN, .fb_id = 0},
	{.x = 221, .y = 160, .w = 20,  .h = 10, .colour = 0xff000000, .flags = JAM_BTN, .fb_id = 0},
	{.x = 250, .y = 160, .w = 20,  .h = 10, .colour = 0xff000000, .flags = JAM_BTN, .fb_id = 0},
};

static struct ctlra_item_info_t sliders_info[] = {
	/* 8 touch faders left-to-right */
	{.x =  52, .y = 185, .w = 16, .h = 65, .flags = CTLRA_ITEM_FADER},
	{.x =  81, .y = 185, .w = 16, .h = 65, .flags = CTLRA_ITEM_FADER},
	{.x = 110, .y = 185, .w = 16, .h = 65, .flags = CTLRA_ITEM_FADER},
	{.x = 137, .y = 185, .w = 16, .h = 65, .flags = CTLRA_ITEM_FADER},
	/* */
	{.x = 165, .y = 185, .w = 16, .h = 65, .flags = CTLRA_ITEM_FADER},
	{.x = 196, .y = 185, .w = 16, .h = 65, .flags = CTLRA_ITEM_FADER},
	{.x = 224, .y = 185, .w = 16, .h = 65, .flags = CTLRA_ITEM_FADER},
	{.x = 256, .y = 185, .w = 16, .h = 65, .flags = CTLRA_ITEM_FADER},
};

static struct ctlra_item_info_t encoder_info[] = {
	{.x = 287, .y = 125, .w = 24, .h = 24, .flags = CTLRA_ITEM_ENCODER},
};

enum BTN_T {
	NI_MASCHINE_JAM_BTN_SONG,
	NI_MASCHINE_JAM_BTN_STEP,
	NI_MASCHINE_JAM_BTN_PAD_MODE,
	NI_MASCHINE_JAM_BTN_CLEAR,
	NI_MASCHINE_JAM_BTN_DUPLICATE,
	NI_MASCHINE_JAM_BTN_DPAD_UP,
	NI_MASCHINE_JAM_BTN_DPAD_LEFT,
	NI_MASCHINE_JAM_BTN_DPAD_RIGHT,
	NI_MASCHINE_JAM_BTN_DPAD_DOWN,
	NI_MASCHINE_JAM_BTN_NOTE_REPEAT,
	NI_MASCHINE_JAM_BTN_MACRO,
	NI_MASCHINE_JAM_BTN_LEVEL,
	NI_MASCHINE_JAM_BTN_AUX,
	NI_MASCHINE_JAM_BTN_CONTROL,
	NI_MASCHINE_JAM_BTN_AUTO,
	NI_MASCHINE_JAM_BTN_SHIFT,
	/* Below touchstrip */
	NI_MASCHINE_JAM_BTN_PLAY,
	NI_MASCHINE_JAM_BTN_REC,
	NI_MASCHINE_JAM_BTN_LEFT,
	NI_MASCHINE_JAM_BTN_RIGHT,
	NI_MASCHINE_JAM_BTN_TEMPO,
	NI_MASCHINE_JAM_BTN_GRID,
	NI_MASCHINE_JAM_BTN_SOLO,
	NI_MASCHINE_JAM_BTN_MUTE,
	/* Right */
	NI_MASCHINE_JAM_BTN_SELECT,
	NI_MASCHINE_JAM_BTN_SWING,
	NI_MASCHINE_JAM_BTN_TUNE,
	NI_MASCHINE_JAM_BTN_LOCK,
	NI_MASCHINE_JAM_BTN_NOTES,
	NI_MASCHINE_JAM_BTN_PERFORM,
	NI_MASCHINE_JAM_BTN_BROWSE,
	NI_MASCHINE_JAM_BTN_ENCODER_TOUCH,
	NI_MASCHINE_JAM_BTN_ENCODER_PRESS,
	NI_MASCHINE_JAM_BTN_IN,
	NI_MASCHINE_JAM_BTN_HEADPHONES,
	NI_MASCHINE_JAM_BTN_MASTER,
	NI_MASCHINE_JAM_BTN_GROUP,
	/* Numbered buttons above grid */
	NI_MASCHINE_JAM_BTN_1,
	NI_MASCHINE_JAM_BTN_2,
	NI_MASCHINE_JAM_BTN_3,
	NI_MASCHINE_JAM_BTN_4,
	NI_MASCHINE_JAM_BTN_5,
	NI_MASCHINE_JAM_BTN_6,
	NI_MASCHINE_JAM_BTN_7,
	NI_MASCHINE_JAM_BTN_8,
	/* Group buttons between grid and touchstrip */
	NI_MASCHINE_JAM_BTN_A,
	NI_MASCHINE_JAM_BTN_B,
	NI_MASCHINE_JAM_BTN_C,
	NI_MASCHINE_JAM_BTN_D,
	NI_MASCHINE_JAM_BTN_E,
	NI_MASCHINE_JAM_BTN_F,
	NI_MASCHINE_JAM_BTN_G,
	NI_MASCHINE_JAM_BTN_H,
	/* Misc */
	NI_MASCHINE_JAM_BTN_FOOTSWITCH,
	NI_MASCHINE_JAM_BTN_COUNT
};

static const struct ni_maschine_jam_ctlra_t buttons[] = {
	{NI_MASCHINE_JAM_BTN_SONG       , 2, 0x01},
	{NI_MASCHINE_JAM_BTN_STEP       , 3, 0x02},
	{NI_MASCHINE_JAM_BTN_PAD_MODE   , 3, 0x04},
	{NI_MASCHINE_JAM_BTN_CLEAR      , 3, 0x08},
	{NI_MASCHINE_JAM_BTN_DUPLICATE  , 3, 0x10},
	{NI_MASCHINE_JAM_BTN_DPAD_UP    , 3, 0x20},
	{NI_MASCHINE_JAM_BTN_DPAD_LEFT  , 3, 0x40},
	{NI_MASCHINE_JAM_BTN_DPAD_RIGHT , 3, 0x80},
	{NI_MASCHINE_JAM_BTN_DPAD_DOWN  , 4, 0x01},
	{NI_MASCHINE_JAM_BTN_NOTE_REPEAT, 4, 0x02},
	{NI_MASCHINE_JAM_BTN_MACRO      , 13, 0x80},
	{NI_MASCHINE_JAM_BTN_LEVEL      , 14, 0x01},
	{NI_MASCHINE_JAM_BTN_AUX        , 14, 0x02},
	{NI_MASCHINE_JAM_BTN_CONTROL    , 14, 0x04},
	{NI_MASCHINE_JAM_BTN_AUTO       , 14, 0x08},
	{NI_MASCHINE_JAM_BTN_SHIFT      , 15, 0x02},
	/* Below touchstrips */
	{NI_MASCHINE_JAM_BTN_PLAY       , 15, 0x04},
	{NI_MASCHINE_JAM_BTN_REC        , 15, 0x08},
	{NI_MASCHINE_JAM_BTN_LEFT       , 15, 0x10},
	{NI_MASCHINE_JAM_BTN_RIGHT      , 15, 0x20},
	{NI_MASCHINE_JAM_BTN_TEMPO      , 15, 0x40},
	{NI_MASCHINE_JAM_BTN_GRID       , 15, 0x80},
	{NI_MASCHINE_JAM_BTN_SOLO       , 16, 0x01},
	{NI_MASCHINE_JAM_BTN_MUTE       , 16, 0x02},
	{NI_MASCHINE_JAM_BTN_SELECT     , 16, 0x04},
	//{NI_MASCHINE_JAM_BTN_, , 0x0},
	{NI_MASCHINE_JAM_BTN_SWING      , 15, 0x01},
	{NI_MASCHINE_JAM_BTN_TUNE       , 14, 0x80},
	{NI_MASCHINE_JAM_BTN_LOCK       , 14, 0x40},
	{NI_MASCHINE_JAM_BTN_NOTES      , 14, 0x20},
	{NI_MASCHINE_JAM_BTN_PERFORM    , 14, 0x10},
	{NI_MASCHINE_JAM_BTN_BROWSE     , 13, 0x40},
	{NI_MASCHINE_JAM_BTN_ENCODER_TOUCH, 16, 0x08},
	{NI_MASCHINE_JAM_BTN_ENCODER_PRESS, 16, 0x10},
	{NI_MASCHINE_JAM_BTN_IN         , 13, 0x10},
	{NI_MASCHINE_JAM_BTN_HEADPHONES , 13, 0x20},
	{NI_MASCHINE_JAM_BTN_MASTER     , 13, 0x04},
	{NI_MASCHINE_JAM_BTN_GROUP      , 13, 0x08},
	/* top buttons */
	{NI_MASCHINE_JAM_BTN_1          ,  2, 0x02},
	{NI_MASCHINE_JAM_BTN_2          ,  2, 0x04},
	{NI_MASCHINE_JAM_BTN_3          ,  2, 0x08},
	{NI_MASCHINE_JAM_BTN_4          ,  2, 0x10},
	{NI_MASCHINE_JAM_BTN_5          ,  2, 0x20},
	{NI_MASCHINE_JAM_BTN_6          ,  2, 0x40},
	{NI_MASCHINE_JAM_BTN_7          ,  2, 0x80},
	{NI_MASCHINE_JAM_BTN_8          ,  3, 0x01},
	/* group buttons */
	{NI_MASCHINE_JAM_BTN_A          , 12, 0x04},
	{NI_MASCHINE_JAM_BTN_B          , 12, 0x08},
	{NI_MASCHINE_JAM_BTN_C          , 12, 0x10},
	{NI_MASCHINE_JAM_BTN_D          , 12, 0x20},
	{NI_MASCHINE_JAM_BTN_E          , 12, 0x40},
	{NI_MASCHINE_JAM_BTN_F          , 12, 0x80},
	{NI_MASCHINE_JAM_BTN_G          , 13, 0x01},
	{NI_MASCHINE_JAM_BTN_H          , 13, 0x02},
	/* Misc */
	{NI_MASCHINE_JAM_BTN_FOOTSWITCH , 16, 0x20},
};
#define BUTTONS_SIZE (sizeof(buttons) / sizeof(buttons[0]))

/* Touchstrips * 3 : timestamp, single-touch, double touch */
#define SLIDERS_SIZE (8 * 3)

#define CONTROLS_SIZE (SLIDERS_SIZE + BUTTONS_SIZE)

/* usb endpoint, 8*8 grid, 1..8 and A..H */
#define GRID_SIZE (1+64+8*2)
#define TOUCHSTRIP_LEDS_SIZE (1 + 11 * 8)

#define NI_MASCHINE_JAM_LED_COUNT \
	(15 /* left pane */ +\
	 11 /* right */ +\
	 16 /* vu */ +\
	 GRID_SIZE)

/* Represents the the hardware device */
struct ni_maschine_jam_t {
	/* base handles usb i/o etc */
	struct ctlra_dev_t base;

	/* the hidraw file descriptor */
	int fd;

	/* current value of each controller is stored here */
	float hw_values[CONTROLS_SIZE];
	/* current state of the lights, only flush on dirty */
	uint8_t lights_dirty;

	uint8_t encoder;

	uint8_t lights_interface;
	uint8_t lights[NI_MASCHINE_JAM_LED_COUNT*2];

	uint8_t grid[GRID_SIZE];
	uint8_t touchstrips[TOUCHSTRIP_LEDS_SIZE];
};

static const char *
ni_maschine_jam_control_get_name(enum ctlra_event_type_t type,
				 uint32_t control_id)
{
	switch(type) {
	case CTLRA_EVENT_SLIDER:
		control_id += NI_MASCHINE_JAM_BTN_COUNT;
		break;
	case CTLRA_EVENT_ENCODER:
		if(control_id == 0) {
			return "Encoder";
		}
		return 0;
	default:
		break;
	}

	if(control_id < CONTROL_NAMES_SIZE)
		return ni_maschine_jam_control_names[control_id];

	return 0;
}

static void
ni_maschine_jam_light_flush(struct ctlra_dev_t *base, uint32_t force);

void ni_machine_jam_usb_read_cb(struct ctlra_dev_t *base, uint32_t endpoint,
				uint8_t *data, uint32_t size);

static uint32_t ni_maschine_jam_poll(struct ctlra_dev_t *base)
{
	struct ni_maschine_jam_t *dev = (struct ni_maschine_jam_t *)base;
	uint8_t buf[1024];
	int32_t nbytes;

#ifdef USE_LIBUSB
	nbytes = ctlra_dev_impl_usb_interrupt_read(base, USB_HANDLE_IDX,
						   USB_ENDPOINT_READ,
						   buf, 1024);
#else
	do {
		if ((nbytes = read(dev->fd, &buf, sizeof(buf))) < 0) {
			break;
		}
		/* call read cb directly */
		//printf("got %d\n", nbytes);
		ni_machine_jam_usb_read_cb(base, USB_ENDPOINT_READ,
					   buf, nbytes);
	} while (nbytes > 0);
#endif
	return 0;
}

/* 11 values per touchstrip */
static void
ni_maschine_jam_touchstrip_led(struct ctlra_dev_t *base,
			       uint8_t touchstrip_id, /* 0 to 7 */
			       uint8_t *values)
{
	struct ni_maschine_jam_t *dev = (struct ni_maschine_jam_t *)base;
	for(int i = 0; i < 11; i++)
		dev->touchstrips[1+touchstrip_id*11+i] = values[i];
}

void ni_machine_jam_usb_read_cb(struct ctlra_dev_t *base, uint32_t endpoint,
				uint8_t *data, uint32_t size)
{
	struct ni_maschine_jam_t *dev = (struct ni_maschine_jam_t *)base;
	switch(size) {
	case 49: {
		static uint8_t old[49];
		int i = 49;
		int do_lights = 0;
		struct ctlra_event_t event = {
			.type = CTLRA_EVENT_SLIDER,
			.slider  = {
				.id = 0,
				.value = 0.f,
			},
		};
		struct ctlra_event_t *e = {&event};

		/* touchstrips: 16 timestamp, 16 single-touch, 16 double */
		for(uint32_t i = 0; i < SLIDERS_SIZE / 3; i++) {
			/* first byte is 0x2 indicating touchstrips,
			 * 2 bytes last-used-timestamp,
			 * 2 bytes single-touch,
			 * 2 bytes double-touch, 2nd number always higher!
			 */
			int offset = 1 + i * 6;
			uint16_t ts = *((uint16_t *)&data[offset]);
			uint16_t t1 = *((uint16_t *)&data[offset+2]);
			uint16_t t2 = *((uint16_t *)&data[offset+4]);

			if(dev->hw_values[offset  ] != ts ||
			   dev->hw_values[offset+1] != t1 ||
			   dev->hw_values[offset+2] != t2) {
				//printf("%d\t%d\t%d\n", ts, t1, t2);
				e->slider.id    = i;
				e->slider.value = t1 / 1023.f;
				float f2 = t2 / 1023.f;
				dev->hw_values[offset  ] = ts;
				dev->hw_values[offset+1] = t1;
				dev->hw_values[offset+2] = t2;
				dev->base.event_func(&dev->base, 1, &e,
						     dev->base.event_func_userdata);

				uint8_t lights[11] = {0};
				for(int i = 0; i < 11; i++)
					lights[i] = (11 * e->slider.value > i) * 30;
				for(int i = 11 * e->slider.value; i < 11; i++)
					lights[i] = (11 * f2 > i) * 20;
				ni_maschine_jam_touchstrip_led(base, i, lights);
			}
		}
		break;
	}

	case 17: {
		static uint8_t old[17];
		int i = 17;
		uint64_t pressed = 0;

		static uint16_t col_mask[] = {
			/* todo: maskes for col 7 and 8 not working */
			0x4, 0x8, 0x10, 0x20, 0x40, 0x80, 0x1000, 0xff00,
		};

		/* rows */
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
		for(int r = 0; r < 8; r++) {
			uint16_t d = *(uint16_t *)&data[4+r];// & 0x3fc;
			/* columns */
			for(int c = 0; c < 6; c++) {
				uint8_t p = d & col_mask[c];
				if(p != dev->grid[r*8+c]) {
					dev->grid[r*8+c] = p;
					e->grid.pos = (r * 8) + c;
					e->grid.pressed = p;
					printf("%d %d = %d\n", r, c, p > 0);
					dev->base.event_func(&dev->base, 1, &e,
							     dev->base.event_func_userdata);
				}
			}
			uint8_t p = data[4+1+r] & 0x1;
			if(p != dev->grid[r*8+6]) {
				e->grid.pos = (r * 8) + 6;
				e->grid.pressed = p;
				dev->grid[r*8+6] = p;
				printf("%d %d = %d\n", r, 6, p);
				dev->base.event_func(&dev->base, 1, &e,
						     dev->base.event_func_userdata);
			}
			p = data[4+1+r] & 0x2;
			if(p != dev->grid[r*8+7]) {
				printf("%d %d = %d\n", r, 7, p);
				dev->grid[r*8+7] = p;
				e->grid.pressed = p;
				e->grid.pos = (r * 8) + 7;
				dev->base.event_func(&dev->base, 1, &e,
						     dev->base.event_func_userdata);
			}
		}

		/* buttons */
		for(uint32_t i = 0; i < BUTTONS_SIZE; i++) {
			int id     = buttons[i].event_id;
			int offset = buttons[i].buf_byte_offset;
			int mask   = buttons[i].mask;

			uint16_t v = *((uint16_t *)&data[offset]) & mask;
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

#if 0
				/* debug surrounding lights */
				something_presssed = !something_presssed;
				int col = 0x30 * something_presssed;
				if(value_idx == 0)
					col = 0xe;
				for(int i = 0; i < 86; i++)
					dev->lights[i] = col;
				ni_maschine_jam_light_flush(base, 1);
#endif
			}
		}

		/* encoder */
		uint8_t encoder_now = (data[1] & 0xf);
		if(dev->encoder != encoder_now) {
			int dir = ctlra_dev_encoder_wrap_16(encoder_now,
							    dev->encoder);
			dev->encoder = encoder_now;

			struct ctlra_event_t event = {
				.type = CTLRA_EVENT_ENCODER,
				.encoder  = {
					.id = 0,
				},
			};
			event.encoder.delta = dir;
			struct ctlra_event_t *e = {&event};
			dev->base.event_func(&dev->base, 1, &e,
					     dev->base.event_func_userdata);
		}
	} /* case 17 */
	} /* switch */
}

static void ni_maschine_jam_light_set(struct ctlra_dev_t *base,
				    uint32_t light_id,
				    uint32_t light_status)
{
	struct ni_maschine_jam_t *dev = (struct ni_maschine_jam_t *)base;
	int ret;

	if(!dev || light_id > NI_MASCHINE_JAM_LED_COUNT)
		return;

	/* write brighness to all LEDs */
	uint32_t bright = (light_status >> 24) & 0x7F;
	dev->lights[light_id] = bright;

	/* FX ON buttons have orange and blue */
#if 0
	if(light_id == NI_MASCHINE_JAM_LED_FX_ON_LEFT ||
	   light_id == NI_MASCHINE_JAM_LED_FX_ON_RIGHT) {
		uint32_t r      = (light_status >> 16) & 0xFF;
		uint32_t b      = (light_status >>  0) & 0xFF;
		dev->lights[light_id  ] = r;
		dev->lights[light_id+1] = b;
	}
#endif

	dev->lights_dirty = 1;
}

uint8_t *
ni_maschine_jam_grid_get_data(struct ctlra_dev_t *base)
{
	struct ni_maschine_jam_t *dev = (struct ni_maschine_jam_t *)base;
	/* 1st is usb endpoint, next 8 are top lights */
	return &dev->grid[9];
}

static void
ni_maschine_jam_light_flush(struct ctlra_dev_t *base, uint32_t force)
{
	struct ni_maschine_jam_t *dev = (struct ni_maschine_jam_t *)base;
	if(!dev->lights_dirty && !force)
		return;
	/* technically interface 3, testing showed 0 works but 3 doesnt */
	uint8_t *data = &dev->lights_interface;
	dev->lights_interface = 0x80;
#ifdef NOPE
	0x04 == dark red
	0x06 == bright red
	0x07 == pink
	0x08 == dark orange
	0x0a == bright orange
	0x0b == salmon
	0x0c == murky yellow
	0x0e == golden
	0x12 == dark yellow
	0x13 == pale yellow
	
	0xb4 == bright purple

	80: "Surrounding" buttons / leds
	81: Grid and Top/Bottom buttons
	82: touch leds
#endif

	static uint8_t col;

	for(int i = 0; i <NI_MASCHINE_JAM_LED_COUNT; i++) {
		//data[i] = 0x06;// 0b11110 * something_presssed;
	}
	//memset(data, 0, sizeof(dev->lights));

#ifdef USE_LIBUSB
	data[0] = 0x80;
	int ret = ctlra_dev_impl_usb_interrupt_write(base, USB_HANDLE_IDX,
						     USB_ENDPOINT_WRITE,
						     data,
						     640+1);
	if(ret < 0)
		printf("%s write failed, ret %d\n", __func__, ret);
	data[0] = 0x81;
	ret = ctlra_dev_impl_usb_interrupt_write(base, USB_HANDLE_IDX,
						     USB_ENDPOINT_WRITE,
						     data,
						     640+1);
	if(ret < 0)
		printf("%s write failed, ret %d\n", __func__, ret);
	data[0] = 0x82;
	ret = ctlra_dev_impl_usb_interrupt_write(base, USB_HANDLE_IDX,
						     USB_ENDPOINT_WRITE,
						     data,
						     640+1);
	if(ret < 0)
		printf("%s write failed, ret %d\n", __func__, ret);
#else
#if 0
	data[0] = 0x80;
	int ret = write(dev->fd, data, 65);
	ret = write(dev->fd, data, 65+16);
	printf("write 1: ret %d\n", ret);

	data[0] = 0x81;
	ret = write(dev->fd, data, 8*10+1);
	printf("write 2: ret %d\n", ret);

	data[0] = 0x82;
	ret = write(dev->fd, data, 64+1);
	printf("write 3: ret %d\n", ret);
	//write(dev->fd, data, 2);
	
#else
	/* try sending one huge message */

	dev->lights[0] = 0x80;
	int ret = write(dev->fd, dev->lights, 81);
	write(dev->fd, dev->lights, 81);
	//write(dev->fd, data, 81);

#if 0
	dev->lights[0] = 0x81;
	ret = write(dev->fd, dev->lights, 81);
	//write(dev->fd, data, 81);
	write(dev->fd, dev->lights, 81);
#endif /* write Grids */

	/*
	uint8_t lights[11];
	for(int i = 0; i < 11; i++)
		lights[i] = 30 * i > (11 * dev->hw_values[1]);
	lights[10] = 20;
	ni_maschine_jam_touchstrip_led(base, 3, lights);
	*/

#if 0
	dev->lights[0] = 0x82;
	/*
	ret = write(dev->fd, dev->touchstrips, TOUCHSTRIP_LEDS_SIZE);
	write(dev->fd, dev->touchstrips, TOUCHSTRIP_LEDS_SIZE);
	*/
	ret = write(dev->fd, dev->lights, TOUCHSTRIP_LEDS_SIZE);
	write(dev->fd, dev->lights, TOUCHSTRIP_LEDS_SIZE);
#endif /* write touchstrips */

#endif

#endif
}

static int32_t
ni_maschine_jam_disconnect(struct ctlra_dev_t *base)
{
	struct ni_maschine_jam_t *dev = (struct ni_maschine_jam_t *)base;

	/* Turn off all lights */
	//memset(dev->lights, 0, NI_MASCHINE_JAM_LED_COUNT);
	if(!base->banished)
		ni_maschine_jam_light_flush(base, 1);

	printf("dev disco %p\n", base);
	free(dev);
	return 0;
}

struct ctlra_dev_info_t ctlra_ni_maschine_jam_info;

struct ctlra_dev_t *
ctlra_ni_maschine_jam_connect(ctlra_event_func event_func,
			      void *userdata, void *future)
{
	(void)future;
	struct ni_maschine_jam_t *dev = calloc(1, sizeof(struct ni_maschine_jam_t));
	if(!dev)
		goto fail;

	snprintf(dev->base.info.vendor, sizeof(dev->base.info.vendor),
		 "%s", "Native Instruments");
	snprintf(dev->base.info.device, sizeof(dev->base.info.device),
		 "%s", "Maschine Jam");


#ifdef USE_LIBUSB
	int err = ctlra_dev_impl_usb_open(&dev->base, CTLRA_DRIVER_VENDOR,
					  CTLRA_DRIVER_DEVICE);
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
#else

#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <linux/hidraw.h>

	int fd, i, res, found = 0;
	char buf[256];
	struct hidraw_devinfo info;

	for(i = 0; i < 64; i++) {
		const char *device = "/dev/hidraw";
		snprintf(buf, sizeof(buf), "%s%d", device, i);
		fd = open(buf, O_RDWR|O_NONBLOCK); // |O_NONBLOCK
		if(fd < 0)
			continue;

		memset(&info, 0x0, sizeof(info));
		res = ioctl(fd, HIDIOCGRAWINFO, &info);

		if (res < 0) {
			perror("HIDIOCGRAWINFO");
		} else {
			if(info.vendor  == CTLRA_DRIVER_VENDOR  &&
			   info.product == CTLRA_DRIVER_DEVICE) {
				found = 1;
				break;
			}
		}
		close(fd);
		/* continue searching next HID dev */
	}

	if(!found) {
		free(dev);
		return 0;
	}

	dev->fd = fd;
	printf("jam on fd %d\n", fd);
#endif

	dev->base.info = ctlra_ni_maschine_jam_info;

	dev->base.poll = ni_maschine_jam_poll;
	dev->base.disconnect = ni_maschine_jam_disconnect;
	dev->base.light_set = ni_maschine_jam_light_set;
	dev->base.light_flush = ni_maschine_jam_light_flush;
	dev->base.usb_read_cb = ni_machine_jam_usb_read_cb;

	dev->base.event_func = event_func;
	dev->base.event_func_userdata = userdata;

	return (struct ctlra_dev_t *)dev;
fail:
	free(dev);
	return 0;
}

struct ctlra_dev_info_t ctlra_ni_maschine_jam_info = {
	.vendor    = "Native Instruments",
	.device    = "Maschine Jam",
	.vendor_id = CTLRA_DRIVER_VENDOR,
	.device_id = CTLRA_DRIVER_DEVICE,
	.size_x    = 320,
	.size_y    = 295,

	/* TODO: expose info */
	.control_count[CTLRA_EVENT_BUTTON] = BUTTONS_SIZE,
	.control_count[CTLRA_EVENT_SLIDER] = SLIDERS_SIZE,
	.control_count[CTLRA_EVENT_ENCODER] = 1,
	.control_count[CTLRA_EVENT_GRID] = 1,

	.control_info[CTLRA_EVENT_BUTTON] = buttons_info,
	.control_info[CTLRA_EVENT_SLIDER] = sliders_info,
	.control_info[CTLRA_EVENT_ENCODER] = encoder_info,

	/* TODO: feedback items:
	 * - 2 VU meter style LED strips
	 *   Left  id: 38 to 45,
	 *   Right id: 46 to 53
	 */
#if 0
	.control_count[CTLRA_FEEDBACK_ITEM] = FEEDBACK_SIZE,
	.control_info[CTLRA_FEEDBACK_ITEM] = feedback_info,
#endif

	.grid_info[0] = {
		.rgb = 1,
		.x = 8,
		.y = 8,
		.info = {
			.x = 51,
			.y = 39,
			.w = 215,
			.h = 100,
			/* TODO: Figure out the light mappings */
			/* start light id */
			.params[0] = 255,
			/* end light id */
			.params[1] = 255,
		}
	},

	.get_name = ni_maschine_jam_control_get_name,
};

CTLRA_DEVICE_REGISTER(ni_maschine_jam)
