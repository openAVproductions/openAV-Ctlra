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

//#define DEBUG_USB_READ 1

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

#define SLIDERS_SIZE (1)
#define ENCODERS_SIZE (1)

#define CONTROLS_SIZE (BUTTONS_SIZE + ENCODERS_SIZE)
/* Its the first pad in hw index */
#define BUTTONS_LIGHTS_SIZE ((int)NI_MASCHINE_MIKRO_MK3_LED_PAD13)
#define LIGHTS_SIZE (80)

#define NPADS                  (16)
/* KERNEL_LENGTH must be a power of 2 for masking */
#define KERNEL_LENGTH          (8)
#define KERNEL_MASK            (KERNEL_LENGTH-1)

static const uint8_t pad_idx_light_mapping[] = {
    NI_MASCHINE_MIKRO_MK3_LED_PAD1,
    NI_MASCHINE_MIKRO_MK3_LED_PAD2,
    NI_MASCHINE_MIKRO_MK3_LED_PAD3,
    NI_MASCHINE_MIKRO_MK3_LED_PAD4,
    NI_MASCHINE_MIKRO_MK3_LED_PAD5,
    NI_MASCHINE_MIKRO_MK3_LED_PAD6,
    NI_MASCHINE_MIKRO_MK3_LED_PAD7,
    NI_MASCHINE_MIKRO_MK3_LED_PAD8,
    NI_MASCHINE_MIKRO_MK3_LED_PAD9,
    NI_MASCHINE_MIKRO_MK3_LED_PAD10,
    NI_MASCHINE_MIKRO_MK3_LED_PAD11,
    NI_MASCHINE_MIKRO_MK3_LED_PAD12,
    NI_MASCHINE_MIKRO_MK3_LED_PAD13,
    NI_MASCHINE_MIKRO_MK3_LED_PAD14,
    NI_MASCHINE_MIKRO_MK3_LED_PAD15,
    NI_MASCHINE_MIKRO_MK3_LED_PAD16,
};

static const uint8_t display_header_top[] = {
    0xE0, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x02, 0x00
};

static const uint8_t display_header_bottom[] = {
    0xE0, 0x00, 0x00, 0x02, 0x00, 0x80, 0x00, 0x02, 0x00
};

struct ni_screen_t {
    uint8_t header [sizeof(display_header_bottom)];
    uint8_t pixels[256];
};

struct ni_lights_t {
    uint8_t header;
    uint8_t data[LIGHTS_SIZE];
};

/* Represents the the hardware device */
struct ni_maschine_mikro_mk3_t {
	/* base handles usb i/o etc */
	struct ctlra_dev_t base;
	/* current value of each controller is stored here */
	float hw_values[CONTROLS_SIZE];
	/* current state of the lights, only flush on dirty */
	uint8_t lights_dirty;

    struct ni_lights_t lights;

	uint8_t encoder_value, encoder_init;
	uint16_t touchstrip_value;
	/* Pressure filtering for note-onset detection */
	uint16_t pad_hit;
	uint16_t pad_idx[NPADS];
	uint16_t pad_pressures[NPADS*KERNEL_LENGTH];

    struct ni_screen_t screen_top;
    struct ni_screen_t screen_bottom;
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

#ifdef CTLRA_MIKRO_MK3_PRESSURE_DEBUG
        int state_change = (current != new);
		int pressure_change = pad_pressures[i] != dev->pad_pressures[i];

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

	uint8_t *buf = data;

#ifdef DEBUG_USB_READ
    for(int i=0; i< size; i++){
        printf("%2x ", buf[i]);
    }
    printf("\n");
#endif

    switch(nbytes) {
        /* Return of LED state, after update written to device */
        case 81: {
            if(memcmp(data, &dev->lights, sizeof(dev->lights)) != 0){
                dev->lights_dirty = 1;
            }
        } break;
        case 128:
            ni_maschine_mikro_mk3_pads(dev, data);
            break;
        case 14: {
            uint16_t v = buf[10] | (buf[11] << 8);
            if(v && v != dev->touchstrip_value) {
                struct ctlra_event_t event = {
                        .type = CTLRA_EVENT_SLIDER,
                        .slider = {
                                .id = 0,
                                .value = v / 1024.f,
                        },
                };
                struct ctlra_event_t *e = {&event};
                dev->base.event_func(&dev->base, 1, &e,
                                     dev->base.event_func_userdata);
                dev->touchstrip_value = v;
            }

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
            int8_t enc = buf[7] & 0x0f;

            /* Skip first event, it will be always send */
            if(!dev->encoder_init) {
                dev->encoder_value = enc;
                dev->encoder_init = 1;
            } else if(enc != dev->encoder_value) {
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
                                            uint32_t light_status) {
    struct ni_maschine_mikro_mk3_t *dev = (struct ni_maschine_mikro_mk3_t *) base;

    if (!dev)
        return;

    if(light_id > sizeof(dev->lights.data))
        return;

    uint32_t bright = light_status >> 27;

    /* normal LEDs */
	if(light_id < BUTTONS_LIGHTS_SIZE) {
        if(dev->lights.data[light_id] != bright) {
            dev->lights.data[light_id] = bright;
            dev->lights_dirty = 1;
        }
    /* 25 strip + 16 pads */
	} else {
        uint8_t hue;

        const uint8_t r = ((light_status >> 16) & 0xFF);
        const uint8_t g = ((light_status >> 8) & 0xFF);
        const uint8_t b = ((light_status >> 0) & 0xFF);

        /* if the input was totally zero, set the LED off */
        if (light_status == 0) {
            hue = 0;
            bright = 0;
        } else {
            /* if equal components, then set white */
            if(r == g && r == b) {
                hue = 0xff;
            } else {
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

                hue = h / 16 + 1;
            }
        };
        uint8_t pad_idx = pad_idx_light_mapping[light_id - BUTTONS_LIGHTS_SIZE];

        uint8_t v = (hue << 2) | (bright & 0x3);
        if(dev->lights.data[pad_idx] != v) {
            dev->lights.data[pad_idx] = v;
            dev->lights_dirty = 1;
        }
	}
}

void
ni_maschine_mikro_mk3_light_flush(struct ctlra_dev_t *base, uint32_t force)
{
    struct ni_maschine_mikro_mk3_t *dev = (struct ni_maschine_mikro_mk3_t *)base;
    if(!dev->lights_dirty && !force)
        return;

    /* error handling in USB subsystem */
    int ret = ctlra_dev_impl_usb_interrupt_write(base,
                                                 USB_HANDLE_IDX,
                                                 USB_ENDPOINT_WRITE,
                                                 (uint8_t *)&dev->lights,
                                                 LIGHTS_SIZE + 1);
    if (ret >= 0){
        dev->lights_dirty = 0;
    }
}


static void
maschine_mikro_mk3_blit_to_screen(struct ni_maschine_mikro_mk3_t *dev)
{
    void *data_top = &dev->screen_top;
    void *data_bottom = &dev->screen_bottom;

    int ret_top = ctlra_dev_impl_usb_bulk_write(&dev->base,
                                                USB_HANDLE_IDX,
                                                USB_ENDPOINT_WRITE,
                                                data_top,
                                                sizeof(dev->screen_top));

    int ret_bottom = ctlra_dev_impl_usb_bulk_write(&dev->base,
                                                   USB_HANDLE_IDX,
                                                   USB_ENDPOINT_WRITE,
                                                   data_bottom,
                                                   sizeof(dev->screen_bottom));

    if(ret_top < 0 || ret_bottom < 0)
        printf("%s screen write failed!\n", __func__);
}


static int32_t
ni_maschine_mikro_mk3_disconnect(struct ctlra_dev_t *base)
{
	struct ni_maschine_mikro_mk3_t *dev = (struct ni_maschine_mikro_mk3_t *)base;

	memset(dev->lights.data, 0x0, LIGHTS_SIZE);
	dev->lights_dirty = 1;

	if(!base->banished) {
		ni_maschine_mikro_mk3_light_flush(base, 1);

        /* Make screen black */
        memset(dev->screen_top.pixels, 0xff,
               sizeof(dev->screen_top.pixels));
        memset(dev->screen_bottom.pixels, 0xff,
               sizeof(dev->screen_bottom.pixels));
        maschine_mikro_mk3_blit_to_screen(dev);
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

    dev->lights.header = 0x80;

    memcpy(dev->screen_top.header , display_header_top, sizeof(dev->screen_top.header));
    memcpy(dev->screen_bottom.header , display_header_bottom, sizeof(dev->screen_bottom.header));

    uint8_t *st = dev->screen_top.pixels;
    uint8_t *sb = dev->screen_bottom.pixels;

    for(int i = 0; i < sizeof(dev->screen_top.pixels); i++) {
        *st++ = 0xff;
        *sb++ = 0x00;
    }

    maschine_mikro_mk3_blit_to_screen(dev);

	dev->lights_dirty = 1;

	dev->base.info = ctlra_ni_maschine_mikro_mk3_info;

	dev->base.poll = ni_maschine_mikro_mk3_poll;
	dev->base.usb_read_cb = ni_maschine_mikro_mk3_usb_read_cb;
	dev->base.disconnect = ni_maschine_mikro_mk3_disconnect;
	dev->base.light_set = ni_maschine_mikro_mk3_light_set;
	dev->base.light_flush = ni_maschine_mikro_mk3_light_flush;

	dev->base.event_func = event_func;
	dev->base.event_func_userdata = userdata;

    for(int i = 0; i < 16; i++) {
        int id = pad_idx_light_mapping[i];
        uint32_t col = 0x0;
        dev->base.light_set(&dev->base, id, col);
    }

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

//	.control_count[CTLRA_FEEDBACK_ITEM] = FEEDBACK_SIZE,
//	.control_info [CTLRA_FEEDBACK_ITEM] = feedback_info,

	.control_count[CTLRA_EVENT_SLIDER] = SLIDERS_SIZE,

	.control_count[CTLRA_EVENT_ENCODER] = ENCODERS_SIZE,

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
			.params[0] = BUTTONS_LIGHTS_SIZE,
			/* end light id */
			.params[1] = BUTTONS_LIGHTS_SIZE + NPADS,
		}
	},

	.get_name = ni_maschine_mikro_mk3_control_get_name,
};

CTLRA_DEVICE_REGISTER(ni_maschine_mikro_mk3)
