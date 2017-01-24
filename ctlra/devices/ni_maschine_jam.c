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

//#include "ni_maschine_jam.h"
#include "impl.h"

#define NI_VENDOR          (0x17cc)
#define NI_MASCHINE_JAM    (0x1500)
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

//#define USE_LIBUSB 1

#warning REMOVE SMTH PRESSED
static int something_presssed;

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
};
#define CONTROL_NAMES_SIZE (sizeof(ni_maschine_jam_control_names) /\
			    sizeof(ni_maschine_jam_control_names[0]))

static const struct ni_maschine_jam_ctlra_t sliders[] = {
	/* Left */
	{0    ,  1, UINT32_MAX},
};
#define SLIDERS_SIZE (sizeof(sliders) / sizeof(sliders[0]))

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
	NI_MASCHINE_JAM_BTN_
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

#define CONTROLS_SIZE (SLIDERS_SIZE + BUTTONS_SIZE)


#define NI_MASCHINE_JAM_LED_COUNT \
	(15 /* left pane */ +\
	 24 /* center non-grid */ +\
	 11 /* right */ +\
	 16 /* vu */ +\
	 64 /* grid */)

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

	uint8_t lights_interface;
	uint8_t lights[NI_MASCHINE_JAM_LED_COUNT*2];
};

static const char *
ni_maschine_jam_control_get_name(const struct ctlra_dev_t *base,
			       enum ctlra_event_type_t type,
			       uint32_t control_id)
{
	struct ni_maschine_jam_t *dev = (struct ni_maschine_jam_t *)base;
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
		printf("got %d\n", nbytes);
		ni_machine_jam_usb_read_cb(base, USB_ENDPOINT_READ,
					   buf, nbytes);
	} while (nbytes > 0);
#endif
	return 0;
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
		while(i --> 0) {
			printf("%02x ", data[i]);
			old[i] = data[i];
		}
		printf("\n");

		break;
	}
	case 17: {
		static uint8_t old[17];
		int i = 17;
#if 1
		while(i --> 0) {
			printf("%02x ", data[i]);
			old[i] = data[i];
		}
		printf("\n");
#endif

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
				if(p) {
					e->grid.pos = (r * 8) + c;
					printf("%d %d = %d\n", r, c, p > 0);
					dev->base.event_func(&dev->base, 1, &e,
							     dev->base.event_func_userdata);
				}
			}
			uint8_t p = data[4+1+r] & 0x1;
			if(p) {
				e->grid.pos = (r * 8) + 6;
				printf("%d %d = %d\n", r, 6, p);
				dev->base.event_func(&dev->base, 1, &e,
						     dev->base.event_func_userdata);
			}
			p = data[4+1+r] & 0x2;
			if(p) {
				printf("%d %d = %d\n", r, 7, p);
				e->grid.pos = (r * 8) + 7;
				dev->base.event_func(&dev->base, 1, &e,
						     dev->base.event_func_userdata);
			}
		}

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

				something_presssed = !something_presssed;
				int col = 0x30 * something_presssed;
				if(value_idx == 0)
					col = 0xe;
				for(int i = 0; i < 86; i++)
					dev->lights[i] = col;
				ni_maschine_jam_light_flush(base, 1);
			}
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

	data[0] = 0x82;
	int ret = write(dev->fd, data, 81);
	write(dev->fd, data, 81);
	printf("write huge: ret %d\n", ret);
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

struct ctlra_dev_t *
ni_maschine_jam_connect(ctlra_event_func event_func,
				  void *userdata, void *future)
{
	(void)future;
	struct ni_maschine_jam_t *dev = calloc(1, sizeof(struct ni_maschine_jam_t));
	if(!dev)
		goto fail;
	printf("dev alloc %p\n", dev);

	snprintf(dev->base.info.vendor, sizeof(dev->base.info.vendor),
		 "%s", "Native Instruments");
	snprintf(dev->base.info.device, sizeof(dev->base.info.device),
		 "%s", "Maschine Jam");


#ifdef USE_LIBUSB
	int err = ctlra_dev_impl_usb_open(&dev->base,
					 NI_VENDOR, NI_MASCHINE_JAM);
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
			if(info.vendor  == NI_VENDOR &&
			   info.product == NI_MASCHINE_JAM) {
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
	dev->base.info.vendor_id = NI_VENDOR;
	dev->base.info.device_id = NI_MASCHINE_JAM;

	dev->base.poll = ni_maschine_jam_poll;
	dev->base.disconnect = ni_maschine_jam_disconnect;
	dev->base.light_set = ni_maschine_jam_light_set;
	dev->base.control_get_name = ni_maschine_jam_control_get_name;
	dev->base.light_flush = ni_maschine_jam_light_flush;
	dev->base.usb_read_cb = ni_machine_jam_usb_read_cb;

	dev->base.event_func = event_func;
	dev->base.event_func_userdata = userdata;

	return (struct ctlra_dev_t *)dev;
fail:
	free(dev);
	return 0;
}

