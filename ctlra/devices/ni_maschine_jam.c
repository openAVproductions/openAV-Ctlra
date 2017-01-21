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

static const char *ni_maschine_jam_control_names[] = {
	/* Faders / Dials */
	"todo",
};
#define CONTROL_NAMES_SIZE (sizeof(ni_maschine_jam_control_names) /\
			    sizeof(ni_maschine_jam_control_names[0]))

static const struct ni_maschine_jam_ctlra_t sliders[] = {
	/* Left */
	{0    ,  1, UINT32_MAX},
};
#define SLIDERS_SIZE (sizeof(sliders) / sizeof(sliders[0]))

static const struct ni_maschine_jam_ctlra_t buttons[] = {
	{0  , 29, 0x10},
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
	uint8_t lights[NI_MASCHINE_JAM_LED_COUNT];
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

static uint32_t ni_maschine_jam_poll(struct ctlra_dev_t *base)
{
	struct ni_maschine_jam_t *dev = (struct ni_maschine_jam_t *)base;
	uint8_t buf[1024];
	int32_t nbytes;

	do {
		if ((nbytes = read(dev->fd, &buf, sizeof(buf))) < 0) {
			break;
		}

		uint8_t *data = buf;

		switch(nbytes) {
		case 49: {
			static uint8_t old[49];
			int i = 49;
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
			while(i --> 0) {
				printf("%02x ", data[i]);
				old[i] = data[i];
			}
			printf("\n");

			uint64_t pressed = 0;

			static uint16_t col_mask[] = {
				/* todo: maskes for col 7 and 8 not working */
				0x4, 0x8, 0x10, 0x20, 0x40, 0x80, 0x1000, 0xff00,
			};

			/* rows */
			for(int r = 0; r < 8; r++) {
				uint16_t d = *(uint16_t *)&data[4+r];// & 0x3fc;
				/* columns */
				for(int c = 0; c < 6; c++) {
					uint8_t p = d & col_mask[c];
					if(p)
						printf("%d %d = %d\n", r, c, p > 0);
				}
				uint8_t p = data[4+1+r] & 0x1;
				if(p)
					printf("%d %d = %d\n", r, 6, p);
				p = data[4+1+r] & 0x2;
				if(p)
					printf("%d %d = %d\n", r, 7, p);
			}
#if 0
			for(uint32_t i = 0; i < SLIDERS_SIZE; i++) {
				int id     = sliders[i].event_id;
				int offset = sliders[i].buf_byte_offset;
				int mask   = sliders[i].mask;

				uint16_t v = *((uint16_t *)&buf[offset]) & mask;
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
							.pressed = v > 0},
					};
					struct ctlra_event_t *e = {&event};
					dev->base.event_func(&dev->base, 1, &e,
							     dev->base.event_func_userdata);
				}
			}
#endif
		} /* case 17 */
		} /* switch */
	} while (nbytes > 0);
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
		data[i] = 0xff;
	}

	data[0] = 0x80;
	write(dev->fd, data, 65);
	data[0] = 0x81;
	write(dev->fd, data, 8*10);
	data[0] = 0x82;
	write(dev->fd, data, 64);
	write(dev->fd, data, 2);

	usleep(100 * 1000);
#if 0
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

#if 0
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
#endif
	dev->base.info.vendor_id = NI_VENDOR;
	dev->base.info.device_id = NI_MASCHINE_JAM;

	dev->base.poll = ni_maschine_jam_poll;
	dev->base.disconnect = ni_maschine_jam_disconnect;
	dev->base.light_set = ni_maschine_jam_light_set;
	dev->base.control_get_name = ni_maschine_jam_control_get_name;
	dev->base.light_flush = ni_maschine_jam_light_flush;

	dev->base.event_func = event_func;
	dev->base.event_func_userdata = userdata;

	return (struct ctlra_dev_t *)dev;
fail:
	free(dev);
	return 0;
}

