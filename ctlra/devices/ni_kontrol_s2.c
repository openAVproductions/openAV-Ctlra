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

#include "ni_kontrol_s2.h"
#include "impl.h"

#define NI_VENDOR          (0x17cc)
#define NI_KONTROL_S2      (0x1320)
#define USB_HANDLE_IDX     (0x0)
#define USB_INTERFACE_ID   (0x03)
#define USB_ENDPOINT_READ  (0x83)
#define USB_ENDPOINT_WRITE (0x02)

/* This struct is a generic struct to identify hw controls */
struct ni_kontrol_s2_ctlra_t {
	int event_id;
	int buf_byte_offset;
	uint32_t mask;
};

static const char *ni_kontrol_s2_names_sliders[] = {
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
#define CONTROL_NAMES_SLIDERS_SIZE (sizeof(ni_kontrol_s2_names_sliders) /\
				    sizeof(ni_kontrol_s2_names_sliders[0]))

static const char *ni_kontrol_s2_names_buttons[] = {
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
#define CONTROL_NAMES_BUTTONS_SIZE (sizeof(ni_kontrol_s2_names_buttons) /\
				    sizeof(ni_kontrol_s2_names_buttons[0]))
#define CONTROL_NAMES_SIZE (CONTROL_NAMES_SLIDERS_SIZE + \
			    CONTROL_NAMES_BUTTONS_SIZE)

static const struct ni_kontrol_s2_ctlra_t sliders[] = {
	/* Left */
	{NI_KONTROL_S2_SLIDER_CROSSFADER,  1, UINT32_MAX},
	{NI_KONTROL_S2_SLIDER_PITCH_L   ,  3, UINT32_MAX},
	{NI_KONTROL_S2_SLIDER_PITCH_R   ,  5, UINT32_MAX},
	{NI_KONTROL_S2_SLIDER_CUE_MIX   ,  7, UINT32_MAX},
	{NI_KONTROL_S2_SLIDER_REMIX     ,  9, UINT32_MAX},
	{NI_KONTROL_S2_SLIDER_MAIN_LEVEL, 11, UINT32_MAX},
	{NI_KONTROL_S2_SLIDER_LEVEL     , 13, UINT32_MAX},
	{NI_KONTROL_S2_SLIDER_FADER_L   , 15, UINT32_MAX},
	{NI_KONTROL_S2_SLIDER_FADER_R   , 17, UINT32_MAX},
	{NI_KONTROL_S2_SLIDER_FX_L_DRY  , 19, UINT32_MAX},
	{NI_KONTROL_S2_SLIDER_FX_L_1    , 21, UINT32_MAX},
	{NI_KONTROL_S2_SLIDER_FX_L_2    , 23, UINT32_MAX},
	{NI_KONTROL_S2_SLIDER_FX_L_3    , 25, UINT32_MAX},
	{NI_KONTROL_S2_SLIDER_FX_L_DRY  , 27, UINT32_MAX},
	{NI_KONTROL_S2_SLIDER_FX_L_1    , 29, UINT32_MAX},
	{NI_KONTROL_S2_SLIDER_FX_L_2    , 31, UINT32_MAX},
	{NI_KONTROL_S2_SLIDER_FX_L_3    , 33, UINT32_MAX},
	{NI_KONTROL_S2_SLIDER_EQ_L_HI   , 35, UINT32_MAX},
	{NI_KONTROL_S2_SLIDER_EQ_L_MID  , 37, UINT32_MAX},
	{NI_KONTROL_S2_SLIDER_EQ_L_LOW  , 39, UINT32_MAX},
	{NI_KONTROL_S2_SLIDER_EQ_R_HI   , 41, UINT32_MAX},
	{NI_KONTROL_S2_SLIDER_EQ_R_MID  , 43, UINT32_MAX},
	{NI_KONTROL_S2_SLIDER_EQ_R_LOW  , 45, UINT32_MAX},
};
#define SLIDERS_SIZE (sizeof(sliders) / sizeof(sliders[0]))

static const struct ni_kontrol_s2_ctlra_t buttons[] = {
	{NI_KONTROL_S2_BTN_DECKB_PLAY , 9, 0x01},
	{NI_KONTROL_S2_BTN_DECKB_CUE  , 9, 0x02},
	{NI_KONTROL_S2_BTN_DECKB_SYNC , 9, 0x04},
	{NI_KONTROL_S2_BTN_DECKB_SHIFT, 9, 0x08},
	{NI_KONTROL_S2_BTN_DECKB_CUE_4, 9, 0x10},
	{NI_KONTROL_S2_BTN_DECKB_CUE_3, 9, 0x20},
	{NI_KONTROL_S2_BTN_DECKB_CUE_2, 9, 0x40},
	{NI_KONTROL_S2_BTN_DECKB_CUE_1, 9, 0x80},

	{NI_KONTROL_S2_BTN_DECKA_JOG_PRESS  , 10, 0x01},
	{NI_KONTROL_S2_BTN_DECKB_JOG_PRESS  , 10, 0x02},
	{NI_KONTROL_S2_BTN_MAIN_BOOTH_SWITCH, 10, 0x04},
	{NI_KONTROL_S2_BTN_MIC_ENGAGE       , 10, 0x08},
	{NI_KONTROL_S2_BTN_DECKB_MIXER_CUE  , 10, 0x10},
	{NI_KONTROL_S2_BTN_DECKB_FLUX       , 10, 0x20},
	{NI_KONTROL_S2_BTN_DECKB_LOOP_IN    , 10, 0x40},
	{NI_KONTROL_S2_BTN_DECKB_LOOP_OUT   , 10, 0x80},

	{NI_KONTROL_S2_BTN_DECKA_PLAY , 11, 0x01},
	{NI_KONTROL_S2_BTN_DECKA_CUE  , 11, 0x02},
	{NI_KONTROL_S2_BTN_DECKA_SYNC , 11, 0x04},
	{NI_KONTROL_S2_BTN_DECKA_SHIFT, 11, 0x08},
	{NI_KONTROL_S2_BTN_DECKA_CUE_4, 11, 0x10},
	{NI_KONTROL_S2_BTN_DECKA_CUE_3, 11, 0x20},
	{NI_KONTROL_S2_BTN_DECKA_CUE_2, 11, 0x40},
	{NI_KONTROL_S2_BTN_DECKA_CUE_1, 11, 0x80},

	{NI_KONTROL_S2_BTN_REMIX_ON_B     , 12, 0x01},
	{NI_KONTROL_S2_BTN_REMIX_ON_A     , 12, 0x02},
	{NI_KONTROL_S2_BTN_BROWSE_LOAD_B  , 12, 0x04},
	{NI_KONTROL_S2_BTN_BROWSE_LOAD_A  , 12, 0x08},
	{NI_KONTROL_S2_BTN_DECKB_MIXER_CUE, 12, 0x10},
	{NI_KONTROL_S2_BTN_DECKB_FLUX     , 12, 0x20},
	{NI_KONTROL_S2_BTN_DECKB_LOOP_IN  , 12, 0x40},
	{NI_KONTROL_S2_BTN_DECKB_LOOP_OUT , 12, 0x80},

	/* waste */
	/* waste */
	{NI_KONTROL_S2_BTN_FX2_DRY_WET     , 13, 0x04},
	{NI_KONTROL_S2_BTN_FX2_3           , 13, 0x08},
	{NI_KONTROL_S2_BTN_FX2_2           , 13, 0x10},
	{NI_KONTROL_S2_BTN_FX2_1           , 13, 0x20},
	{NI_KONTROL_S2_BTN_DECKA_GAIN_PRESS, 13, 0x40},
	{NI_KONTROL_S2_BTN_DECKA_GAIN_PRESS, 13, 0x80},

	{NI_KONTROL_S2_BTN_MIXER_B_FX2, 14, 0x01},
	{NI_KONTROL_S2_BTN_MIXER_B_FX1, 14, 0x02},
	{NI_KONTROL_S2_BTN_MIXER_A_FX2, 14, 0x04},
	{NI_KONTROL_S2_BTN_MIXER_A_FX1, 14, 0x08},
	{NI_KONTROL_S2_BTN_FX2_DRY_WET, 14, 0x10},
	{NI_KONTROL_S2_BTN_FX2_3      , 14, 0x20},
	{NI_KONTROL_S2_BTN_FX2_2      , 14, 0x40},
	{NI_KONTROL_S2_BTN_FX2_1      , 14, 0x80},

	{NI_KONTROL_S2_BTN_DECKA_LEFT_ENCODER_PRESS , 15, 0x01},
	{NI_KONTROL_S2_BTN_DECKA_RIGHT_ENCODER_PRESS, 15, 0x02},
	{NI_KONTROL_S2_BTN_BROWSE_ENCODER_PRESS     , 15, 0x04},
	{NI_KONTROL_S2_BTN_DECKB_LEFT_ENCODER_PRESS , 15, 0x08},
	{NI_KONTROL_S2_BTN_DECKB_RIGHT_ENCODER_PRESS, 15, 0x10},
};
#define BUTTONS_SIZE (sizeof(buttons) / sizeof(buttons[0]))

#define CONTROLS_SIZE (SLIDERS_SIZE + BUTTONS_SIZE)

#define NI_KONTROL_S2_LED_COUNT 64

/* Represents the the hardware device */
struct ni_kontrol_s2_t {
	/* base handles usb i/o etc */
	struct ctlra_dev_t base;
	/* current value of each controller is stored here */
	float hw_values[CONTROLS_SIZE];
	/* current state of the lights, only flush on dirty */
	uint8_t lights_dirty;

	uint8_t lights_interface;
	uint8_t lights[NI_KONTROL_S2_LED_COUNT];

	uint8_t deck_lights_interface;
	uint8_t deck_lights[16*3]; // TODO: *3 for rgb
};

static const char *
ni_kontrol_s2_control_get_name(enum ctlra_event_type_t type,
			       uint32_t control)
{
	switch(type) {
	case CTLRA_EVENT_SLIDER:
		if(control >= CONTROL_NAMES_SLIDERS_SIZE)
			return 0;
		return ni_kontrol_s2_names_sliders[control];
	case CTLRA_EVENT_BUTTON:
		if(control >= CONTROL_NAMES_BUTTONS_SIZE)
			return 0;
		return ni_kontrol_s2_names_buttons[control];
	default:
		break;
	}

	return 0;
}

static uint32_t ni_kontrol_s2_poll(struct ctlra_dev_t *base)
{
	struct ni_kontrol_s2_t *dev = (struct ni_kontrol_s2_t *)base;
#define BUF_SIZE 1024
	uint8_t buf[BUF_SIZE];

	int handle_idx = 0;
	ctlra_dev_impl_usb_interrupt_read(base, USB_HANDLE_IDX,
					  USB_ENDPOINT_READ,
					  buf, BUF_SIZE);
	return 0;
}

void ni_kontrol_s2_usb_read_cb(struct ctlra_dev_t *base, uint32_t endpoint,
				uint8_t *data, uint32_t size)
{
	struct ni_kontrol_s2_t *dev = (struct ni_kontrol_s2_t *)base;
	uint8_t *buf = data;

	switch(size) {
	case 17: { /* buttons and jog wheels */
#if 0
#define RESET  "\x1B[0m"
#define GREEN  "\x1B[32m"
		static uint8_t array[17];
		for(int i = 0; i < 17; i++) {
			if(array[i] != buf[16-i])
				printf(GREEN);
			printf("%02x %s", buf[16 - i], RESET);
			array[i] = buf[16-i];
		}
		printf("\n");
#endif
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
		for(int i = 0; i < 51; i++) {
			printf("%02x", buf[51 - i]);
			/* if(i % 4 == 0) printf(" ");*/
		}
		printf("\n");
		/*
		uint16_t *v = &buf[5];
		struct ctlra_event_t event = {
			.type = CTLRA_EVENT_SLIDER,
			.slider  = {
				.id = 0,
				.value = *v / 4096.f},
		};
		struct ctlra_event_t *e = {&event};
		dev->base.event_func(&dev->base, 1, &e,
				     dev->base.event_func_userdata);
		break;
		*/

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

static void ni_kontrol_s2_light_set(struct ctlra_dev_t *base,
				    uint32_t light_id,
				    uint32_t light_status)
{
	struct ni_kontrol_s2_t *dev = (struct ni_kontrol_s2_t *)base;
	int ret;

	if(!dev || light_id > NI_KONTROL_S2_LED_COUNT)
		return;

	memset(dev->lights, 0, NI_KONTROL_S2_LED_COUNT);
	/* 0 to 4 level A: -15, -6 0, +3 +6,
	 * 5 to 9 level B: "
	 *
	 * 10 FX dry wet
	 * 11 fx 1
	 * 12 fx 2
	 * 13 fx 3
	 *
	 * 14 Mixer A FX 1
	 * 15 Mixer A FX 2
	 * 16 Mixer B FX 1
	 * 17 Mixer B FX 2
	 *
	 * 18 FX dry wet
	 * 19 fx 1
	 * 20 fx 2
	 * 21 fx 3
	 *
	 * 22 Remix On A
	 *    Remix On B
	 * 24 Browse Load A
	 *    Browse Load B
	 * 26 CUE A
	 * 27 Mixer Warning
	 * 28 Mixer USB Connection
	 *  ANALOG MIC - Requires audio to be running?
	 * 30 CUE B
	 *
	 * 31 Deck A Flux 
	 * 32 Deck A Loop In
	 * 33 Deck A Loop Out
	 *
	 * 34 Deck B Loop In
	 * 35 Deck B Loop Out
	 * 36 Deck B Flux
	 */
	dev->lights[28] = 0xff;

	memset(dev->deck_lights, 0, NI_KONTROL_S2_LED_COUNT);
	/* dec A cue1 RGB, cue2 rgb ...
	 * dec B cue1 RGB, cue2 rgb ...
	 * deck A shift (brightness) 24
	 *	  sync 25
	 *	  cue 26
	 *	  play 27
	 * deck B shift (brightness) 28
	 *	  sync 29
	 *	  cue 30
	 *	  play 31
	 */
	//dev->deck_lights[32] = 0xff;

	dev->lights_dirty = 1;
	return ;

	/* write brighness to all LEDs */
	uint32_t bright = (light_status >> 24) & 0x7F;
	dev->lights[light_id] = bright;

	/* FX ON buttons have orange and blue */
	if(light_id == NI_KONTROL_S2_LED_FX_ON_LEFT ||
	   light_id == NI_KONTROL_S2_LED_FX_ON_RIGHT) {
		uint32_t r      = (light_status >> 16) & 0xFF;
		uint32_t b      = (light_status >>  0) & 0xFF;
		dev->lights[light_id  ] = r;
		dev->lights[light_id+1] = b;
	}

	dev->lights_dirty = 1;
}

void
ni_kontrol_s2_light_flush(struct ctlra_dev_t *base, uint32_t force)
{
	struct ni_kontrol_s2_t *dev = (struct ni_kontrol_s2_t *)base;
	if(!dev->lights_dirty && !force)
		return;
	/* technically interface 3, testing showed 0 works but 3 doesnt */
	uint8_t *data = &dev->lights_interface;

	/* all normal single-colour (brightness) leds */
	dev->lights_interface = 0x80;

	int ret = ctlra_dev_impl_usb_interrupt_write(base, USB_HANDLE_IDX,
						     USB_ENDPOINT_WRITE,
						     data,
						     NI_KONTROL_S2_LED_COUNT+1);
	if(ret < 0) {
		//printf("%s write failed!\n", __func__);
	}

	/* Cue / Remix slots, shift0sync-cue-play for both decks */
	data = &dev->deck_lights_interface;
	dev->deck_lights_interface = 0x81;
	ret = ctlra_dev_impl_usb_interrupt_write(base, USB_HANDLE_IDX,
						     USB_ENDPOINT_WRITE,
						     data,
						     NI_KONTROL_S2_LED_COUNT+1);
	if(ret < 0) {
		//printf("%s write failed!\n", __func__);
	}

}

static int32_t
ni_kontrol_s2_disconnect(struct ctlra_dev_t *base)
{
	struct ni_kontrol_s2_t *dev = (struct ni_kontrol_s2_t *)base;

	/* Turn off all lights */
	memset(dev->lights, 0, NI_KONTROL_S2_LED_COUNT);
	if(!base->banished)
		ni_kontrol_s2_light_flush(base, 1);

	ctlra_dev_impl_usb_close(base);
	free(dev);
	return 0;
}

struct ctlra_dev_t *
ctlra_ni_kontrol_s2_connect(ctlra_event_func event_func,
				  void *userdata, void *future)
{
	(void)future;
	struct ni_kontrol_s2_t *dev = calloc(1, sizeof(struct ni_kontrol_s2_t));
	if(!dev)
		goto fail;

	snprintf(dev->base.info.vendor, sizeof(dev->base.info.vendor),
		 "%s", "Native Instruments");
	snprintf(dev->base.info.device, sizeof(dev->base.info.device),
		 "%s", "Kontrol S2");


	dev->base.info.control_count[CTLRA_EVENT_BUTTON] = BUTTONS_SIZE;
	dev->base.info.control_count[CTLRA_EVENT_SLIDER] = SLIDERS_SIZE;
	dev->base.info.get_name = ni_kontrol_s2_control_get_name;

	int err = ctlra_dev_impl_usb_open(&dev->base,
					 NI_VENDOR, NI_KONTROL_S2);
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

	dev->base.poll = ni_kontrol_s2_poll;
	dev->base.disconnect = ni_kontrol_s2_disconnect;
	dev->base.light_set = ni_kontrol_s2_light_set;
	dev->base.light_flush = ni_kontrol_s2_light_flush;
	dev->base.usb_read_cb = ni_kontrol_s2_usb_read_cb;

	dev->base.event_func = event_func;
	dev->base.event_func_userdata = userdata;

	return (struct ctlra_dev_t *)dev;
fail:
	free(dev);
	return 0;
}

