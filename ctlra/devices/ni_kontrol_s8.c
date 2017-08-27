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
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "ni_kontrol_s8.h"

#include "impl.h"

//#define MOCK_S8 1

#ifdef MOCK_S8
#define NI_VENDOR                 (0x17cc)
#define NI_KONTROL_S8             (0x1400)

#define USB_INTERFACE_BTNS        (0x0)
#define USB_ENDPOINT_BTNS_READ    (0x81)
#define USB_ENDPOINT_BTNS_WRITE   (0x01)

#define USB_INTERFACE_SCREEN      (0x1)
#define USB_ENDPOINT_SCREEN_WRITE (0x2)
#else
#define NI_VENDOR                 (0x17cc)
#define NI_KONTROL_S8             (0x1370)

/* trial-and-error: although iface is 0x5 it fails, while 0x0 works */
#define USB_INTERFACE_BTNS        (0x0)

#define USB_ENDPOINT_BTNS_READ    (0x84)
#define USB_ENDPOINT_BTNS_WRITE   (0x03)

//#define USB_INTERFACE_SCREEN      (0x1)
//#define USB_ENDPOINT_SCREEN_WRITE (0x2)
#endif

#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <linux/hidraw.h>

/* This struct is a generic struct to identify hw controls */
struct ni_kontrol_s8_ctlra_t {
	int event_id;
	int buf_byte_offset;
	uint32_t mask;
};

static const char *ni_kontrol_s8_button_names[] = {
	"Deck A",
	"Deck B",
	"Deck C",
	"Deck D",
	"FX Button 1",
	"FX Button 2",
	"FX Button 3",
	"FX Button 4",
	"FX Dial Touch 1",
	"FX Dial Touch 2",
	"FX Dial Touch 3",
	"FX Dial Touch 4",
	"FX Select",
	"Screen Left 1",
	"Screen Left 2",
	"Screen Left 3",
	"Screen Left 4",
	"Screen Right 1",
	"Screen Right 2",
	"Screen Right 3",
	"Screen Right 4",
	"Encoder Touch 1",
	"Encoder Touch 2",
	"Encoder Touch 3",
	"Encoder Touch 4",
	"Browse Encoder Press",
	"Browse Encoder Touch",
	"Back",
	"Capture",
	"Edit",
	"Loop Encoder Press",
	"Loop Encoder Touch",
	"On 1",
	"On 2",
	"On 3",
	"On 4",
	"Fader Touch 1",
	"Fader Touch 2",
	"Fader Touch 3",
	"Fader Touch 4",
	"Pad 1",
	"Pad 2",
	"Pad 3",
	"Pad 4",
	"Pad 5",
	"Pad 6",
	"Pad 7",
	"Pad 8",
	"Hotcue",
	"Loop",
	"Freeze",
	"Remix",
	"Flux",
	"Deck",
	"Shift",
	"Sync",
	"Cue",
	"Play",
	"Touchstrip Touch",
};
#define CONTROL_NAMES_BUTTON_SIZE (sizeof(ni_kontrol_s8_button_names) /\
				   sizeof(ni_kontrol_s8_button_names[0]))

static const char *ni_kontrol_s8_slider_names[] = {
	/* bounded sliders */
	"Fader 1",
	"Fader 2",
	"Fader 3",
	"Fader 4",
	"FX Dial 1",
	"FX Dial 2",
	"FX Dial 3",
	"FX Dial 4",
	"Touchstrip Movement",
};
#define CONTROL_NAMES_SLIDER_SIZE (sizeof(ni_kontrol_s8_slider_names) /\
				   sizeof(ni_kontrol_s8_slider_names[0]))

static const char *ni_kontrol_s8_encoder_names[] = {
	/* unbounded (endless) encoder sliders */
	"Screen Encoder 1",
	"Screen Encoder 2",
	"Screen Encoder 3",
	"Screen Encoder 4",
	/* encoders with notches */
	"Browse Encoder Turn",
	"Loop Encoder Turn",
};
#define CONTROL_NAMES_ENCODER_SIZE (sizeof(ni_kontrol_s8_encoder_names) /\
				    sizeof(ni_kontrol_s8_encoder_names[0]))

#define CONTROL_NAMES_SIZE (CONTROL_NAMES_BUTTON_SIZE + \
			    CONTROL_NAMES_SLIDER_SIZE + \
			    CONTROL_NAMES_ENCODER_SIZE)

/* Sliders are calulated on the fly, not using pre-set bitmasks */
#define SLIDERS_SIZE (12)

static const struct ni_kontrol_s8_ctlra_t buttons[] = {
	{NI_KONTROL_S8_BTN_DECK_A  , 5, 0x01},
	{NI_KONTROL_S8_BTN_DECK_B  , 5, 0x02},
	{NI_KONTROL_S8_BTN_DECK_C  , 5, 0x04},
	{NI_KONTROL_S8_BTN_DECK_D  , 5, 0x08},
	{NI_KONTROL_S8_BTN_FX_1    , 2, 0x80},
	{NI_KONTROL_S8_BTN_FX_2    , 3, 0x04},
	{NI_KONTROL_S8_BTN_FX_3    , 3, 0x02},
	{NI_KONTROL_S8_BTN_FX_4    , 3, 0x01},
	{NI_KONTROL_S8_BTN_FX_DIAL_TOUCH_1,  9, 0x40},
	{NI_KONTROL_S8_BTN_FX_DIAL_TOUCH_2,  9, 0x80},
	{NI_KONTROL_S8_BTN_FX_DIAL_TOUCH_3, 10, 0x10},
	{NI_KONTROL_S8_BTN_FX_DIAL_TOUCH_4, 10, 0x20},
	{NI_KONTROL_S8_BTN_FX_SELECT, 2, 0x40},
	{NI_KONTROL_S8_BTN_SCREEN_LEFT_1, 2, 0x20},
	{NI_KONTROL_S8_BTN_SCREEN_LEFT_2, 2, 0x10},
	{NI_KONTROL_S8_BTN_SCREEN_LEFT_3, 2, 0x01},
	{NI_KONTROL_S8_BTN_SCREEN_LEFT_4, 4, 0x40},
	{NI_KONTROL_S8_BTN_SCREEN_RIGHT_1, 3, 0x08},
	{NI_KONTROL_S8_BTN_SCREEN_RIGHT_2, 3, 0x10},
	{NI_KONTROL_S8_BTN_SCREEN_RIGHT_3, 3, 0x20},
	{NI_KONTROL_S8_BTN_SCREEN_RIGHT_4, 3, 0x40},
	{NI_KONTROL_S8_BTN_SCREEN_ENCODER_TOUCH_1, 9, 0x02},
	{NI_KONTROL_S8_BTN_SCREEN_ENCODER_TOUCH_2, 9, 0x04},
	{NI_KONTROL_S8_BTN_SCREEN_ENCODER_TOUCH_3, 9, 0x08},
	{NI_KONTROL_S8_BTN_SCREEN_ENCODER_TOUCH_4, 9, 0x10},
	{NI_KONTROL_S8_BTN_ENCODER_BROWSE_PRESS, 2, 0x08},
	{NI_KONTROL_S8_BTN_ENCODER_BROWSE_TOUCH, 9, 0x20},
	{NI_KONTROL_S8_BTN_BACK,    4, 0x80},
	{NI_KONTROL_S8_BTN_CAPTURE, 4, 0x20},
	{NI_KONTROL_S8_BTN_EDIT,    4, 0x01},
	{NI_KONTROL_S8_BTN_ENCODER_LOOP_PRESS, 6, 0x10},
	{NI_KONTROL_S8_BTN_ENCODER_LOOP_TOUCH, 9, 0x01},
	{NI_KONTROL_S8_BTN_ON_1, 4, 0x10},
	{NI_KONTROL_S8_BTN_ON_2, 4, 0x08},
	{NI_KONTROL_S8_BTN_ON_3, 4, 0x04},
	{NI_KONTROL_S8_BTN_ON_4, 4, 0x02},
	{NI_KONTROL_S8_BTN_FADER_TOUCH_1, 10, 0x01},
	{NI_KONTROL_S8_BTN_FADER_TOUCH_2, 10, 0x02},
	{NI_KONTROL_S8_BTN_FADER_TOUCH_3, 10, 0x04},
	{NI_KONTROL_S8_BTN_FADER_TOUCH_4, 10, 0x08},
	{NI_KONTROL_S8_BTN_PAD_1, 7, 0x08},
	{NI_KONTROL_S8_BTN_PAD_2, 7, 0x01},
	{NI_KONTROL_S8_BTN_PAD_3, 6, 0x01},
	{NI_KONTROL_S8_BTN_PAD_4, 6, 0x02},
	{NI_KONTROL_S8_BTN_PAD_5, 7, 0x20},
	{NI_KONTROL_S8_BTN_PAD_6, 7, 0x40},
	{NI_KONTROL_S8_BTN_PAD_7, 7, 0x80},
	{NI_KONTROL_S8_BTN_PAD_8, 7, 0x02},
	{NI_KONTROL_S8_BTN_HOTCUE, 8, 0x08},
	{NI_KONTROL_S8_BTN_LOOP  , 8, 0x04},
	{NI_KONTROL_S8_BTN_FREEZE, 8, 0x02},
	{NI_KONTROL_S8_BTN_REMIX , 8, 0x01},
	{NI_KONTROL_S8_BTN_FLUX  , 7, 0x10},
	{NI_KONTROL_S8_BTN_DECK  , 7, 0x04},
	{NI_KONTROL_S8_BTN_SHIFT, 8, 0x80},
	{NI_KONTROL_S8_BTN_SYNC , 8, 0x40},
	{NI_KONTROL_S8_BTN_CUE  , 8, 0x20},
	{NI_KONTROL_S8_BTN_PLAY , 8, 0x10},
};
#define BUTTONS_SIZE (sizeof(buttons) / sizeof(buttons[0]))

#define CONTROLS_SIZE (SLIDERS_SIZE + BUTTONS_SIZE)

/* Complicated:
 * Some lights have 3x colours (so 3 bytes) others 2, and most 1.
 * Do some math, and end up with this size. This is used as the USB
 * transfer size, and the size in the dev struct - so getting it wrong
 * means memcpy()-ing of invalid ranges, and overwriting stuff - don't :)
 */
/*
1-24   pads
25     fx select
26-29  fx btn 1 to 4
30-33  screen left 1 to 4
34-37  screen right 1 to 4
38 back
39 cap
40 edit
41-44 track on 1 to 4
45 hotque(white)
46     (blue)
47,48 loop w/b
49 50 freeze w/b
51 52 Remix
53 Flux
54 55 Deck
56 Shift
57 sync (green)
58 sync (red)
59 cue
60 play
61-64 loop surround white
65-68 loop surround blue
69-94: 25 touchstrip leds blue left to right
94-+25: 25 touchstrip leds orange left to right
119 - 122 decks a b c d
*/
#define LEDS_SIZE (122)


/* Screen blit commands - no need to have publicly in header */
static const uint8_t header[] = {
	0x84,  0x0, 0x0, 0x60,
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
#define NUM_PX (480 * 272)
struct s8_screen_blit {
	uint8_t header [sizeof(header)];
	uint8_t command[sizeof(command)];
	uint8_t pixels [NUM_PX*2]; // 565 uses 2 bytes per pixel
	uint8_t footer [sizeof(footer)];
};

/* Represents the the hardware device */
struct ni_kontrol_s8_t {
	/* base handles usb i/o etc */
	struct ctlra_dev_t base;

	/* fd for /dev/hidraw mode */
	int fd;

	/* current value of each controller is stored here */
	float hw_values[CONTROLS_SIZE];

	/* track the touch of the touchstrip seperatly, so we can send
	 * a button-event when a the touch-strip is pressed/released. The
	 * touchstrip movement itself is available as a slider */
	uint8_t touchstrip_touch;
	/* Encoders */
	uint8_t encoder_browse;
	uint8_t encoder_loop;

	/* current state of the lights, only flush on dirty */
	uint8_t lights_dirty;

	float screen_encoders[4];

	/* bit hacky, but allows sending of endpoint address as the
	 * usb transfer, as hidapi encodes endpoint by first byte of
	 * transfer. */
	uint8_t lights_endpoint;
	uint8_t lights[LEDS_SIZE];
	uint8_t waste;

	/* this is a huge datastructure that includes full frame pixels,
	 * leave it at the end of the struct to get out of the way */
	struct s8_screen_blit screen_blit;
};

static const char *
ni_kontrol_s8_control_get_name(enum ctlra_event_type_t type,
                               uint32_t control)
{
	const char *ret = 0;
	switch(type) {
	case CTLRA_EVENT_SLIDER:
		if(control >= CONTROL_NAMES_SLIDER_SIZE)
			break;
		ret = ni_kontrol_s8_slider_names[control];
		break;
	case CTLRA_EVENT_BUTTON:
		if(control >= CONTROL_NAMES_BUTTON_SIZE)
			break;
		ret = ni_kontrol_s8_button_names[control];
		break;
	case CTLRA_EVENT_ENCODER:
		if(control >= CONTROL_NAMES_ENCODER_SIZE)
			break;
		ret = ni_kontrol_s8_encoder_names[control];
		break;
	default:
		break;
	};

	return ret;
}

void ni_kontrol_s8_usb_read_cb(struct ctlra_dev_t *base, uint32_t endpoint,
				uint8_t *data, uint32_t size);

static uint32_t ni_kontrol_s8_poll(struct ctlra_dev_t *base)
{
	struct ni_kontrol_s8_t *dev = (struct ni_kontrol_s8_t *)base;
#define BUF_SIZE 1024
	uint8_t buf[BUF_SIZE];
	int handle_idx = 0;

#if 0
	ctlra_dev_impl_usb_interrupt_read(base, handle_idx,
					  USB_ENDPOINT_BTNS_READ,
					  buf, BUF_SIZE);
#else
	int nbytes;
	do {
		if ((nbytes = read(dev->fd, &buf, sizeof(buf))) < 0) {
			break;
		}
		/* call read cb directly */
		printf("hid read () got %d\n", nbytes);
		ni_kontrol_s8_usb_read_cb(base, USB_ENDPOINT_BTNS_READ,
					   buf, nbytes);
	} while (nbytes > 0);
#endif

	return 0;
}

void ni_kontrol_s8_usb_read_cb(struct ctlra_dev_t *base, uint32_t endpoint,
				uint8_t *data, uint32_t size)
{
	printf("%s : size = %d\n", __func__, size);
	if(size > 1024) {
		printf("size > 1024, returning\n");
		return;
	}

#define GREEN "\x1b[32m"
#define RESET "\x1b[0m"

	static uint8_t old[1024];
	for(int i = 0; i < size; i++) {
		int same = (old[i] == data[i]);
		printf("%s%02x%s ", same ? RESET : GREEN, data[i], RESET);
		old[i] = data[i];
	}
	printf("%s\n", RESET);
	return;

#if 0
	struct ni_kontrol_s8_t *dev = (struct ni_kontrol_s8_t *)base;
	uint8_t *buf = data;
	switch(size) {
	case 25: {
		for(uint32_t i = 0; i < 4; i++) {
			/* screen encoders here */
			uint16_t val = (buf[(i*2)+2] << 8) | (buf[i*2+1]);
			//printf("%d : %04x\n", i, val);
			if(dev->screen_encoders[i] != val) {
				float delta = val - dev->screen_encoders[i];
				dev->screen_encoders[i] = val;
				struct ctlra_event_t event = {
					.type =
						CTLRA_EVENT_ENCODER,
					.encoder  = {
						.id = NI_KONTROL_S8_ENCODER_SCREEN_1 + i,
						.flags = CTLRA_EVENT_ENCODER_FLAG_FLOAT,
						.delta_float = delta / 999.f,
					}
				};
				struct ctlra_event_t *e = {&event};
				dev->base.event_func(&dev->base, 1, &e,
						     dev->base.event_func_userdata);
				//printf("encoder %d: value = %f\n", i, event.encoder.delta_float);
				dev->screen_encoders[i] = val;
			}
		}
		for(uint32_t i = 4; i < SLIDERS_SIZE; i++) {
			uint16_t val = (buf[(i*2)+2] << 8) | (buf[i*2+1]);
			float v = ((float)val) / 0xfee;
			if(dev->hw_values[i] != val) {
				dev->hw_values[i] = val;
				int id = NI_KONTROL_S8_SLIDER_FADER_1 + (i - 4);
				struct ctlra_event_t event = {
					.type = CTLRA_EVENT_SLIDER,
					.slider  = {
						.id = id,
						.value = v
					},
				};
				struct ctlra_event_t *e = {&event};
				dev->base.event_func(&dev->base, 1, &e,
						     dev->base.event_func_userdata);
			}
		}
		break;
	}

	case 17: {
		for(uint32_t i = 0; i < BUTTONS_SIZE; i++) {
			int id     = buttons[i].event_id;
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
						.pressed = v > 0
					},
				};
				struct ctlra_event_t *e = {&event};
				dev->base.event_func(&dev->base, 1, &e,
						     dev->base.event_func_userdata);
			}
		}
		/* Browse / Loop Encoders */
		struct ctlra_event_t event = {
			.type = CTLRA_EVENT_ENCODER,
			.encoder = {
				.id = NI_KONTROL_S8_ENCODER_BROWSE,
				.flags = CTLRA_EVENT_ENCODER_FLAG_INT,
				.delta = 0,
			},
		};
		struct ctlra_event_t *e = {&event};
		int8_t browse = ((buf[1] & 0xf0) >> 4) & 0xf;
		int8_t loop   = ((buf[1] & 0x0f)     ) & 0xf;
		/* Browse encoder turn event */
		if(browse != dev->encoder_browse) {
			int dir = ctlra_dev_encoder_wrap_16(browse,
							    dev->encoder_browse);
			event.encoder.delta = dir;
			dev->encoder_browse = browse;
			dev->base.event_func(&dev->base, 1, &e,
					     dev->base.event_func_userdata);
		}
		/* Loop encoder turn event */
		if(loop != dev->encoder_loop) {
			int dir = ctlra_dev_encoder_wrap_16(loop,
							    dev->encoder_loop);
			event.encoder.id = NI_KONTROL_S8_ENCODER_LOOP;
			event.encoder.delta = dir;
			dev->encoder_loop = loop;
			dev->base.event_func(&dev->base, 1, &e,
					     dev->base.event_func_userdata);
		}

		/* Touchstrip */
		uint16_t v = (buf[14] << 8) | buf[13];
		if(dev->touchstrip_touch != (v > 0) ) {
			dev->touchstrip_touch = v > 0;
			struct ctlra_event_t event = {
				.type = CTLRA_EVENT_BUTTON,
				.button = {
					.id = NI_KONTROL_S8_BTN_TOUCHSTRIP_TOUCH,
					.pressed = v > 0,
				},
			};
			struct ctlra_event_t *e = {&event};
			dev->base.event_func(&dev->base, 1, &e,
					     dev->base.event_func_userdata);
		}
		/* Send touchstrip updates after button detection */
		if(dev->touchstrip_touch) {
			struct ctlra_event_t event = {
				.type = CTLRA_EVENT_SLIDER,
				.slider = {
					.id = NI_KONTROL_S8_SLIDER_TOUCHSTRIP,
					.value = v / 1024.f
				},
			};
			struct ctlra_event_t *e = {&event};
			dev->base.event_func(&dev->base, 1, &e,
					     dev->base.event_func_userdata);
		}
		break;
	} /* case 17 */
	} /* switch */
#endif
}

uint8_t *
ni_kontrol_s8_screen_get_pixels(struct ctlra_dev_t *base)
{
	struct ni_kontrol_s8_t *dev = (struct ni_kontrol_s8_t *)base;
	return (uint8_t *)&dev->screen_blit.pixels;
}

static void
ni_kontrol_s8_screen_splash(struct ctlra_dev_t *base)
{
	struct ni_kontrol_s8_t *dev = (struct ni_kontrol_s8_t *)base;
	memset(dev->screen_blit.pixels, 0x0,
	       sizeof(dev->screen_blit.pixels));
	ni_kontrol_s8_screen_blit(&dev->base);
}

void
ni_kontrol_s8_screen_blit(struct ctlra_dev_t *base)
{
	/* NOT SUPPORTED - working on input first */
#if 0
	struct ni_kontrol_s8_t *dev = (struct ni_kontrol_s8_t *)base;

	int ret = ctlra_dev_impl_usb_bulk_write(base, USB_INTERFACE_SCREEN,
						USB_ENDPOINT_SCREEN_WRITE,
						(uint8_t *)&dev->screen_blit,
						sizeof(dev->screen_blit));
	if(ret < 0)
		printf("%s write failed!\n", __func__);
#endif
}

int32_t
ni_kontrol_s8_screen_get_data(struct ctlra_dev_t *base, uint8_t **pixels,
			      uint32_t *bytes, uint8_t flush)
{
	/* NOT SUPPORTED - working on input first */
#if 0
	struct ni_kontrol_s8_t *dev = (struct ni_kontrol_s8_t *)base;
	/* fill in out params */
	*pixels = ni_kontrol_s8_screen_get_pixels(base);
	*bytes = sizeof(dev->screen_blit.pixels);

	if(flush)
		ni_kontrol_s8_screen_blit(base);
#endif
	return 0;
}

void
ni_kontrol_s8_light_touchstrip(struct ctlra_dev_t *base,
                               uint8_t *orange,
                               uint8_t *blue)
{
	struct ni_kontrol_s8_t *dev = (struct ni_kontrol_s8_t *)base;
	dev->lights_dirty = 1;
	for(int i = 0; i < 25; i++) {
		dev->lights[68+i] = blue[i];
		dev->lights[93+i] = orange[i];
	}
}

static void
ni_kontrol_s8_light_set(struct ctlra_dev_t *base, uint32_t light_id,
                        uint32_t light_status)
{
	struct ni_kontrol_s8_t *dev = (struct ni_kontrol_s8_t *)base;
	int ret;

	/* write brighness to all LEDs */
	uint32_t bright = (light_status >> 24) & 0x7F;
	uint8_t r = (light_status >> 16) & 0xff;
	uint8_t g = (light_status >>  8) & 0xff;
	uint8_t b = (light_status >>  0) & 0xff;

	int idx;

	switch(light_id) {
	case NI_KONTROL_S8_LED_PAD_1:
	case NI_KONTROL_S8_LED_PAD_2:
	case NI_KONTROL_S8_LED_PAD_3:
	case NI_KONTROL_S8_LED_PAD_4:
	case NI_KONTROL_S8_LED_PAD_5:
	case NI_KONTROL_S8_LED_PAD_6:
	case NI_KONTROL_S8_LED_PAD_7:
	case NI_KONTROL_S8_LED_PAD_8:
		/* handle RGB pads */
		idx = (light_id-NI_KONTROL_S8_LED_PAD_1) * 3;
		dev->lights[idx+0] = b;
		dev->lights[idx+1] = g;
		dev->lights[idx+2] = r;
		break;
	case NI_KONTROL_S8_LED_SCREEN_LEFT_1:
	case NI_KONTROL_S8_LED_SCREEN_LEFT_2:
	case NI_KONTROL_S8_LED_SCREEN_LEFT_3:
	case NI_KONTROL_S8_LED_SCREEN_LEFT_4:
	case NI_KONTROL_S8_LED_SCREEN_RIGHT_1:
	case NI_KONTROL_S8_LED_SCREEN_RIGHT_2:
	case NI_KONTROL_S8_LED_SCREEN_RIGHT_3:
	case NI_KONTROL_S8_LED_SCREEN_RIGHT_4:
		idx = light_id - NI_KONTROL_S8_LED_SCREEN_LEFT_1;
		dev->lights[29+idx] = bright;
		break;
	case NI_KONTROL_S8_LED_FX_SELECT:
	case NI_KONTROL_S8_LED_FX_1:
	case NI_KONTROL_S8_LED_FX_2:
	case NI_KONTROL_S8_LED_FX_3:
	case NI_KONTROL_S8_LED_FX_4:
		idx = light_id - NI_KONTROL_S8_LED_FX_SELECT;
		dev->lights[24+idx] = bright;
		break;
	case NI_KONTROL_S8_LED_ON_1:
	case NI_KONTROL_S8_LED_ON_2:
	case NI_KONTROL_S8_LED_ON_3:
	case NI_KONTROL_S8_LED_ON_4:
		idx = light_id - NI_KONTROL_S8_LED_ON_1;
		dev->lights[40+idx] = bright;
		break;
	case NI_KONTROL_S8_LED_DECK_A:
	case NI_KONTROL_S8_LED_DECK_B:
	case NI_KONTROL_S8_LED_DECK_C:
	case NI_KONTROL_S8_LED_DECK_D:
		idx = light_id - NI_KONTROL_S8_LED_DECK_A;
		dev->lights[118+idx] = bright;
		break;
	case NI_KONTROL_S8_LED_BACK:
	case NI_KONTROL_S8_LED_CAPTURE:
	case NI_KONTROL_S8_LED_EDIT:
		idx = light_id - NI_KONTROL_S8_LED_BACK;
		dev->lights[37+idx] = bright;
		break;
	case NI_KONTROL_S8_LED_HOTCUE:
		dev->lights[44] = bright;
		dev->lights[45] = b;
		break;
	case NI_KONTROL_S8_LED_LOOP:
		dev->lights[46] = bright;
		dev->lights[47] = b;
		break;
	case NI_KONTROL_S8_LED_FREEZE:
		dev->lights[48] = bright;
		dev->lights[49] = b;
		break;
	case NI_KONTROL_S8_LED_REMIX:
		dev->lights[50] = bright;
		dev->lights[51] = b;
		break;
	case NI_KONTROL_S8_LED_FLUX:
		dev->lights[52] = bright;
		break;
	case NI_KONTROL_S8_LED_DECK:
		dev->lights[53] = bright;
		dev->lights[54] = b;
		break;
	case NI_KONTROL_S8_LED_SHIFT:
		dev->lights[55] = bright;
		break;
	case NI_KONTROL_S8_LED_SYNC:
		dev->lights[56] = g;
		dev->lights[57] = r;
		break;
	case NI_KONTROL_S8_LED_CUE:
		dev->lights[58] = bright;
		break;
	case NI_KONTROL_S8_LED_PLAY:
		dev->lights[59] = bright;
		break;
	case NI_KONTROL_S8_LED_LOOP_CIRCLE_1:
	case NI_KONTROL_S8_LED_LOOP_CIRCLE_2:
	case NI_KONTROL_S8_LED_LOOP_CIRCLE_3:
	case NI_KONTROL_S8_LED_LOOP_CIRCLE_4:
		idx = light_id - NI_KONTROL_S8_LED_LOOP_CIRCLE_1;
		dev->lights[60+idx] = bright;
		dev->lights[64+idx] = b;
		break;
	default: break;
	}

	dev->lights_dirty = 1;
}

void
ni_kontrol_s8_light_flush(struct ctlra_dev_t *base, uint32_t force)
{
	struct ni_kontrol_s8_t *dev = (struct ni_kontrol_s8_t *)base;
	if(!dev->lights_dirty && !force)
		return;

	uint8_t *data = &dev->lights_endpoint;
	dev->lights_endpoint = 0x80;

#if 0
	int ret = ctlra_dev_impl_usb_interrupt_write(&dev->base,
	                USB_INTERFACE_BTNS,
	                USB_ENDPOINT_BTNS_WRITE,
	                data, LEDS_SIZE+1);
	if(ret < 0)
		printf("%s write failed!\n", __func__);
#endif
}

static int32_t
ni_kontrol_s8_disconnect(struct ctlra_dev_t *base)
{
	struct ni_kontrol_s8_t *dev = (struct ni_kontrol_s8_t *)base;

	/* Turn off all lights */
	memset(dev->lights, 0x0, sizeof(dev->lights));
	if(!base->banished) {
		ni_kontrol_s8_light_flush(&dev->base, 1);
		ni_kontrol_s8_screen_splash(base);
	}

	close(dev->fd);
	//ctlra_dev_impl_usb_close(base);
	free(dev);
	return 0;
}

struct ctlra_dev_t *
ctlra_ni_kontrol_s8_connect(ctlra_event_func event_func,
                      void *userdata, void *future)
{
	(void)future;
	struct ni_kontrol_s8_t *dev = calloc(1, sizeof(struct ni_kontrol_s8_t));
	if(!dev)
		goto fail;

	snprintf(dev->base.info.vendor, sizeof(dev->base.info.vendor),
	         "%s", "Native Instruments");
	snprintf(dev->base.info.device, sizeof(dev->base.info.device),
	         "%s", "Kontrol S8");


#if 1
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
			if(info.vendor  == NI_VENDOR &&
			   info.product == NI_KONTROL_S8) {
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
	printf("s8 on fd %d\n", fd);
#else
	/* Open buttons / leds handle */
	int err = ctlra_dev_impl_usb_open(&dev->base, NI_VENDOR, NI_KONTROL_S8);
	if(err) {
		printf("%s: failed to open usb interface %d\n",
		       __func__, __LINE__);
		goto fail;
	}

	printf("%s: Connected! usb_open() success\n", __func__);

	err = ctlra_dev_impl_usb_open_interface(&dev->base,
	                                        USB_INTERFACE_BTNS,
	                                        USB_INTERFACE_BTNS);
	if(err) {
		printf("%s: failed to open button usb interface\n", __func__);
		goto fail;
	}

	printf("%s: Connected! open_interface success\n", __func__);

#if 0
	/* NOT SUPPORTED - working on input first */
	err = ctlra_dev_impl_usb_open_interface(&dev->base,
	                                        USB_INTERFACE_SCREEN,
	                                        USB_INTERFACE_SCREEN);
	if(err) {
		printf("%s: failed to open screen usb interface\n", __func__);
		goto fail;
	}
#endif

#endif /* libusb / /dev/hidrawX */

	dev->base.info.control_count[CTLRA_EVENT_SLIDER] =
		CONTROL_NAMES_SLIDER_SIZE;
	dev->base.info.control_count[CTLRA_EVENT_BUTTON] =
		CONTROL_NAMES_BUTTON_SIZE;
	dev->base.info.control_count[CTLRA_EVENT_ENCODER] =
		CONTROL_NAMES_ENCODER_SIZE;
	dev->base.info.get_name = ni_kontrol_s8_control_get_name;

	/* Copy the screen update details into the embedded struct */
	memcpy(dev->screen_blit.header , header , sizeof(dev->screen_blit.header));
	memcpy(dev->screen_blit.command, command, sizeof(dev->screen_blit.command));
	memcpy(dev->screen_blit.footer , footer , sizeof(dev->screen_blit.footer));

	dev->base.poll = ni_kontrol_s8_poll;
	dev->base.disconnect = ni_kontrol_s8_disconnect;
	dev->base.light_set = ni_kontrol_s8_light_set;
	dev->base.light_flush = ni_kontrol_s8_light_flush;
	dev->base.usb_read_cb = ni_kontrol_s8_usb_read_cb;
	dev->base.screen_get_data = ni_kontrol_s8_screen_get_data;

	dev->base.event_func = event_func;
	dev->base.event_func_userdata = userdata;

	printf("%s: Connected! connect() returning %p\n", __func__, dev);

	return (struct ctlra_dev_t *)dev;
fail:
	free(dev);
	return 0;
}

