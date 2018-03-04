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

#ifndef OPENAV_CTLRA_DEVICE_IMPL
#define OPENAV_CTLRA_DEVICE_IMPL

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>

#define CTLRA_INTERNAL 1

#include "ctlra.h"
#include "event.h"

#define debug_print_check(c, level)					\
	(!c || ((c->opts.debug_level & CTLRA_DEBUG_LEVEL_MASK) > level))

#define CTLRA_STRERROR(ctlra, err)					\
	do { ctlra->strerror = err; } while (0)

#define CTLRA_ERROR(ctlra, fmt, ...)					\
	do { if (debug_print_check(ctlra, CTLRA_DEBUG_ERROR))		\
	fprintf(stderr, "[\033[1;31m%s +%d\033[0m] " fmt,		\
		__func__, __LINE__, __VA_ARGS__);			\
	} while (0)
#define CTLRA_WARN(ctlra, fmt, ...)					\
	do { if (debug_print_check(ctlra, CTLRA_DEBUG_WARN))		\
	fprintf(stderr, "[\033[1;33m%s +%d\033[0m] " fmt,		\
		__func__, __LINE__, __VA_ARGS__);			\
	} while (0)
#define CTLRA_INFO(ctlra, fmt, ...)					\
	do { if (debug_print_check(ctlra, CTLRA_DEBUG_INFO))		\
	fprintf(stderr, "[\033[1;32m%s +%d\033[0m] " fmt,		\
		__func__, __LINE__, __VA_ARGS__);			\
	} while (0)
#define CTLRA_DRIVER(ctlra, fmt, ...)					\
	do { if (!ctlra || ctlra->opts.debug_level & CTLRA_DEBUG_DRIVER)\
	fprintf(stderr, "[\033[1;36m%s +%d\033[0m] " fmt,		\
		__func__, __LINE__, __VA_ARGS__);			\
	} while (0)


struct ctlra_dev_t;

/* Functions each device can implement */
typedef uint32_t (*ctlra_dev_impl_poll)(struct ctlra_dev_t *dev);
typedef int32_t (*ctlra_dev_impl_disconnect)(struct ctlra_dev_t *dev);
typedef void (*ctlra_dev_impl_light_set)(struct ctlra_dev_t *dev,
					   uint32_t light_id,
					   uint32_t light_status);
typedef void (*ctlra_dev_impl_feedback_set)(struct ctlra_dev_t *dev,
					    uint32_t fb_id,
					    float value);
typedef void (*ctlra_dev_impl_feedback_digits)(struct ctlra_dev_t *dev,
					    uint32_t fb_id,
					    float value);
typedef void (*ctlra_dev_impl_light_flush)(struct ctlra_dev_t *dev,
					  uint32_t force);
typedef void (*ctlra_dev_impl_usb_read_cb)(struct ctlra_dev_t *dev,
					   uint32_t endpoint,
					   uint8_t *data,
					   uint32_t size);
typedef int32_t (*ctlra_dev_impl_screen_get_data)(struct ctlra_dev_t *dev,
						  uint32_t screen_idx,
						  uint8_t **pixels,
						  uint32_t *bytes,
						  struct ctlra_screen_zone_t *redraw,
						  uint8_t flush);
typedef int32_t (*ctlra_dev_impl_grid_light_set)(struct ctlra_dev_t *dev,
						uint32_t grid_id,
						uint32_t light_id,
						uint32_t light_status);
typedef const char* (*ctlra_dev_impl_control_get_name)
						(const struct ctlra_dev_t *dev,
						enum ctlra_event_type_t type,
						uint32_t control_id);

#define CTLRA_USB_IFACE_PER_DEV 2

struct ctlra_dev_t {
	/* Instance and next in list */
	struct ctlra_t     *ctlra_context;
	struct ctlra_dev_t *dev_list_next;
	struct ctlra_dev_t *banished_list_next;

	/* Static Device Info  */
	int vendor_id;
	int product_id;
	int class_id;

	/* when set, the dev is not polled or fed feedback. The device is
	 * usually banished as the cable is unplugged or due to IO error */
	uint8_t banished;

	/* usb handle for this hardware device. */
	void *usb_device;

	/* Certain complex controllers require more than one
	 * usb interface to be fully controlled (typically screen/buttons
	 * are on bulk/interrupt interfaces). The controller is responsible
	 * for providing the correct interface_id to the usb_read/write()
	 * functions */
	void *usb_handle[CTLRA_USB_IFACE_PER_DEV];
	uint8_t usb_interface[CTLRA_USB_IFACE_PER_DEV];
	/* linked list of outstanding async transfers */
	void *usb_async_next;
	/* statistics of USB backend */
#define USB_XFER_INT_READ 0
#define USB_XFER_INT_WRITE 1
#define USB_XFER_BULK_WRITE 2
#define USB_XFER_CANCELLED 3
#define USB_XFER_TIMEOUT 4
#define USB_XFER_ERROR 5
#define USB_XFER_BULK_ERROR 6
#define USB_XFER_INFLIGHT_READ 7
#define USB_XFER_INFLIGHT_WRITE 8
#define USB_XFER_INFLIGHT_CANCEL 9
#define USB_XFER_COUNT 10
	uint32_t usb_xfer_counts[USB_XFER_COUNT];


	/* TODO; remove the belowusb xfer pointers */
	uint8_t usb_xfer_head;
	uint8_t usb_xfer_tail;
#define CTLRA_USB_XFER_COUNT 32 /*UINT8_MAX*/
	void *usb_xfer_ptr[CTLRA_USB_XFER_COUNT];



	/* MIDI I/O pointer */
	void *midi_in;
	void *midi_out;

	/* Event callback function */
	ctlra_event_func event_func;
	ctlra_feedback_func feedback_func;
	void* event_func_userdata;

	/* Function pointers to poll events from device */
	ctlra_dev_impl_poll poll;
	ctlra_dev_impl_disconnect disconnect;

	/* Function pointers to write feedback to device */
	ctlra_dev_impl_light_set light_set;
	ctlra_dev_impl_feedback_set feedback_set;
	ctlra_dev_impl_feedback_digits feedback_digits;
	ctlra_dev_impl_grid_light_set grid_light_set;
	ctlra_dev_impl_light_flush light_flush;
	ctlra_dev_impl_usb_read_cb usb_read_cb;

	/* Screen related functions */
	ctlra_dev_impl_screen_get_data screen_get_data;
	ctlra_screen_redraw_cb screen_redraw_cb;
	void *screen_redraw_ud;
	struct timespec screen_last_redraw;

	/* Function pointer to retrive info about a particular control */
	ctlra_dev_impl_control_get_name control_get_name;

	/* Function pointer to call just before the device is removed */
	ctlra_remove_dev_func remove_func;

	/* Internal representation of the controller info */
	struct ctlra_dev_info_t info;
};

/** Connect function to instantiate a dev from the driver */
typedef struct ctlra_dev_t *(*ctlra_dev_connect_func)(ctlra_event_func event_func,
						    void *userdata,
						    void *future);

/** Opens the libusb handle for the given vid:pid.
 * Implementation in usb.c.
 * @retval 0 on Success
 * @retval -1 on Error
 * @retval -ENODEV when device not found */
int ctlra_dev_impl_usb_open(struct ctlra_dev_t *dev, int vid, int pid);

/** Opens the interface on a usb device. This allows controllers to make
 * multiple connections to interfaces, allowing access to screens, lights,
 * etc regardless of what USB endpoints they are presented on.
 */
int ctlra_dev_impl_usb_open_interface(struct ctlra_dev_t *ctlra_dev,
				      int interface, int handle_idx);

/** Read bytes from the usb device, this is a non-blocking function but
 * _not_ realtime safe function. It polls the usb handle specified by *idx*
 * of the device *dev*, reading bytes up to *size* into the buffer pointed
 * to by *data*. */
int ctlra_dev_impl_usb_interrupt_read(struct ctlra_dev_t *dev, uint32_t idx,
				      uint32_t endpoint, uint8_t *data,
				      uint32_t size);

/** Writes bytes to the device using an interrupt USB transfer*/
int ctlra_dev_impl_usb_interrupt_write(struct ctlra_dev_t *dev, uint32_t idx,
				       uint32_t endpoint, uint8_t *data,
				       uint32_t size);

/** Writes bytes to the device using a bulk USB transfer*/
int ctlra_dev_impl_usb_bulk_write(struct ctlra_dev_t *dev, uint32_t idx,
				  uint32_t endpoint, uint8_t *data,
				  uint32_t size);

/** Close the USB device handles, returning them to the kernel */
void ctlra_dev_impl_usb_close(struct ctlra_dev_t *dev);

/* Marks a device as failed, and adds it to the disconnect list. After
 * having been banished, the device instance will not function again */
void ctlra_dev_impl_banish(struct ctlra_dev_t *dev);

/* IMPLEMENTATION DETAILS ONLY BELOW HERE */


struct ctlra_t
{
	/* Options this instance was created with */
	struct ctlra_create_opts_t opts;

	/* Accept callback for application */
	ctlra_accept_dev_func accept_dev_func;
	void *accept_dev_func_userdata;

	/* USB backend context */
	struct libusb_context *ctx;
	uint8_t usb_initialized;

	/* Linked list of devices currently in use */
	struct ctlra_dev_t *dev_list;
	/* List of devices that are banished */
	struct ctlra_dev_t *banished_list;

	/* context aware error message pointer */
	const char *strerror;
};

/* Macro extern declaration for the connect function */
#define CTLRA_DEVICE_DECL(name)					\
extern struct ctlra_dev_t * ctlra_ ## name ## _connect(		\
				ctlra_event_func event_func,	\
			    void *userdata, void *future)
/* Macro returns the function name registered using above macro */
#define CTLRA_DEVICE_FUNC(name)	ctlra_ ## name ## _connect

/* Macro to declare an extern info struct for a device */
#define CTLRA_DEVICE_INFO(name)					\
extern struct ctlra_dev_info_t ctlra_ ## name ## _info;
/* Macro that returns the name of the info struct */
#define CTLRA_DEVICE_INFO_NAME(name) ctlra_ ## name ## _info


struct ctlra_dev_connect_func_t {
	uint32_t vid;
	uint32_t pid;
	ctlra_dev_connect_func connect;
	struct ctlra_dev_info_t *info;
};

// TODO: check does this registration system even help
#define CTLRA_MAX_DEVICES 64
extern uint32_t __ctlra_device_count;
extern struct ctlra_dev_connect_func_t __ctlra_devices[CTLRA_MAX_DEVICES];


#define CTLRA_DEVICE_REGISTER(name)				\
static const struct ctlra_dev_connect_func_t __ctlra_dev = {	\
	.vid = CTLRA_DRIVER_VENDOR,				\
	.pid = CTLRA_DRIVER_DEVICE,				\
	.connect = ctlra_ ## name ## _connect,			\
	.info = &ctlra_ ## name ## _info,			\
};								\
__attribute__((constructor(102)))				\
static void ctlra_ ## name ## _register() {			\
	__ctlra_devices[__ctlra_device_count++] = __ctlra_dev;	\
}




/* Helper function for dealing with wrapped encoders */
static inline int8_t ctlra_dev_encoder_wrap_16(uint8_t newer, uint8_t older)
{
	/* direction */
	uint8_t ngt = newer > older;
	/* backwards mask */
	uint8_t bm = (newer == 0xf && older == 0x0) - 1;
	/* forward wrap mask */
	uint8_t fm = (newer == 0x0 && older == 0xf) - 1;
	/* Mask into direction */
	int8_t dir = ngt;
	dir = (dir & bm) | (0 &(~bm));
	dir = (dir & fm) | (1 &(~fm));
	/* Scale to -1 or 1 */
	return (dir * 2) - 1;
}

#ifdef __cplusplus
}
#endif

#endif /* OPENAV_CTLRA_DEVICE_IMPL */

