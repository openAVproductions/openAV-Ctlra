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

#ifndef OPENAV_CTLR_DEVICE_IMPL
#define OPENAV_CTLR_DEVICE_IMPL

#include "../event.h"

#include "libusb.h"

struct ctlr_dev_t;

/* Functions each device must implement */
typedef uint32_t (*ctlr_dev_impl_poll)(struct ctlr_dev_t *dev);
typedef int32_t (*ctlr_dev_impl_disconnect)(struct ctlr_dev_t *dev);
typedef void (*ctlr_dev_impl_light_set)(struct ctlr_dev_t *dev,
					   uint32_t light_id,
					   uint32_t light_status);
typedef int32_t (*ctlr_dev_impl_grid_light_set)(struct ctlr_dev_t *dev,
						uint32_t grid_id,
						uint32_t light_id,
						uint32_t light_status);

struct ctlr_dev_t {
	/* Static Device Info  */
	int vendor_id;
	int product_id;
	int class_id;

	/* The common use-case interface that is used to write LEDs, and
	 * read button / dial messages from the device */
	int interface_id;

	/* Some devices with screens use a seperate interface for the
	 * screen, allowing faster or other types of transfers */
	int screen_interface_id;

	/* libusb generic handle for this hardware device */
	libusb_device *usb_device;
	/* The libusb handle for the opened instance of this device */
	libusb_device_handle *usb_handle;

	/* Event callback function */
	ctlr_event_func event_func;
	void* event_func_userdata;

	/* Function pointers to poll events from device */
	ctlr_dev_impl_poll poll;
	ctlr_dev_impl_disconnect disconnect;

	/* Function pointers to write feedback to device */
	ctlr_dev_impl_light_set light_set;
	ctlr_dev_impl_grid_light_set grid_light_set;
};

/** Connect function to instantiate a dev from the driver */
typedef struct ctlr_dev_t *(*ctlr_dev_connect_func)(ctlr_event_func event_func,
						    void *userdata,
						    void *future);

/* Macro extern declaration for the connect function */
#define DECLARE_DEV_CONNECT_FUNC(name)					\
extern struct ctlr_dev_t *name(ctlr_event_func event_func,		\
			    void *userdata, void *future)

/** Opens the libusb handle for the given vid:pid pair, skipping the
 * first *num_skip* entries, to allow opening the N(th) of a single
 * type of controller. Implementation in usb.c.
 * @retval -ENODEV when device not found */
int ctlr_dev_impl_usb_open(int vid,
			   int pid,
			   struct ctlr_dev_t *dev,
			   uint32_t num_skip);

#endif /* OPENAV_CTLR_DEVICE_IMPL */

