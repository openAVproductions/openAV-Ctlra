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

#ifndef OPENAV_CTLRA_H
#define OPENAV_CTLRA_H

/* Tell Doxygen to ignore this header */
/**  \cond */
#include <stdint.h>
/** \endcond */

#ifdef __cplusplus
extern "C" {
#endif

/*! \mainpage Ctlra - A plain C library to program with hardware controllers.
 *
 * Official Codebase, please file any issues with Ctlra against this repo:
 *  https://github.com/openAVproductions/openAV-ctlra
 *
 * The README.md file has instructions on building.
 *
 * General questions / comments:
 * Harry van Haaren <harryhaaren@gmail.com>
 * OpenAV Productions
 */

/** @file
 * Ctlra is a lightweight C library for accessing controllers, to
 * make it easier to implement support for them in software. This library
 * abstracts the hardware and transport layer, providing events for all
 * inputs available, and functions that can be called to display feedback
 * on the device.
 */

#include <stdio.h>

#include "event.h"

#define DEPRECATED __attribute__((deprecated))

#define CTLRA_STR_MAX        32
#define CTLRA_DEV_SERIAL_MAX 64
#define CTLRA_NUM_GRIDS_MAX   8
#define CTLRA_NUM_SCREENS_MAX 2

/** Ctlra context forward declaration. This struct is opaque to the user
 * of the ctlra library. It provides a handle for ctlra internals such as
 * hotplug callbacks and backend details (libusb context for example). It
 * also holds a handle to each ctlra_dev_t in use, allowing it to clean up
 * resources if the application calls ctlra_exit() on the *ctlra_t*.
 */
struct ctlra_t;

/* Errors only is default */
#define CTLRA_DEBUG_ERROR 0
#define CTLRA_DEBUG_NONE  1
#define CTLRA_DEBUG_WARN  2
#define CTLRA_DEBUG_INFO  3
#define CTLRA_DEBUG_LEVEL_MASK (0x0f)
#define CTLRA_DEBUG_DRIVER (1 << 6)
#define CTLRA_DEBUG_BACKEND (1 << 7)

/** Struct for forward compatibility, allowing various options to be passed
 * to ctlra, without breaking all the function calls */
struct ctlra_create_opts_t {
	/* creation time flags */
	uint8_t flags_usb_no_own_context : 1;
	uint8_t flags_usb_unsued : 7;

	/* debug verbosity */
	uint8_t debug_level;

	/* for any devices with screens, attempt to achieve this number of
	 * frames-per-second. This means the screen_redraw() callback will
	 * be called this fps value number of times per second.
	 */
	uint8_t screen_redraw_target_fps;

	/* reserve lots of space */
	uint8_t padding[62];
};

/** Get the human readable name for *control_id* from *dev*. The
 * control id is passed in eg: event.button.id, or can be any of the
 * DEVICE_NAME_CONTROLS enumeration. Ownership of the string *remains* in
 * the driver, so the application *must not* attempt to free the returned
 * pointer.
 * @retval 0 Invalid control_id requested
 * @retval Ptr A pointer to a string representing the control name
 */
typedef const char *(*ctlra_info_get_name_func)(enum ctlra_event_type_t type,
						uint32_t control_id);

/** Struct that provides physical layout and capabilities about each
 * item on the controller. Sizes are provided in millimeters. An item can
 * represent a control such as a slider or dial, but also feedback only
 * items such as an LED, or screen.
 */
#define CTLRA_ITEM_BUTTON        (1<< 0)
#define CTLRA_ITEM_FADER         (1<< 1) /* check W vs H for orientation */
#define CTLRA_ITEM_DIAL          (1<< 2)
#define CTLRA_ITEM_ENCODER       (1<< 3)
#define CTLRA_ITEM_CENTER_NOTCH  (1<< 4)

#define CTLRA_ITEM_LED_INTENSITY (1<< 5)
#define CTLRA_ITEM_LED_COLOR     (1<< 6)

#define CTLRA_ITEM_FB_LED_STRIP  (1<< 7)
#define CTLRA_ITEM_FB_SCREEN     (1<< 8)
#define CTLRA_ITEM_FB_7_SEGMENT  (1<< 9)

#define CTLRA_ITEM_HAS_FB_ID     (1<<31)
struct ctlra_item_info_t {
	uint32_t x; /* location of item on X axis */
	uint32_t y; /* location of item on Y axis */
	uint32_t w; /* size of item on X axis */
	uint32_t h; /* size of item on Y axis */

	/* TODO: figure out how to expose capabilities of item */
	uint64_t flags;

	/* The feedback id for this item. Calling ctlra_dev_light_set()
	 * with this light_id should result the the LED under the item
	 * changing state */
	uint32_t fb_id;
	/* mask of colours the hw can represent */
	uint32_t colour;
	/* feedback parameters depending on flags */
	uint16_t params[4];
};

/** A struct describing the properties of a grid */
struct ctlra_grid_info_t {
	/* capabilities of each pad */
	/** When non-zero each pad has the capability to show RGB colour.
	 * When zero, the pad either has no LED, or brightness only */
	uint8_t rgb;
	uint8_t velocity;
	uint8_t pressure;
	/* number of pads in x and y direction */
	uint32_t x;
	uint32_t y;
	/* location of grid itself on the device */
	struct ctlra_item_info_t info;
};

#define CTLRA_DEV_TYPE_INVALID          0
#define CTLRA_DEV_TYPE_USB_HID          1
#define CTLRA_DEV_TYPE_BLUETOOTH        2
#define CTLRA_DEV_TYPE_USB_MIDI         3

/** ID struct for a USB HID device */
struct ctlra_dev_usb_hid_t {
	/** USB Vendor ID */
	uint32_t vendor_id;
	/** USB device ID */
	uint32_t device_id;
	/** Serial number from USB library (if available) */
	uint64_t serial_number;
};

struct ctlra_dev_id_t {
	/* integer representing the type of this device.
	 * Eg: CTLRA_DEV_TYPE_USB_HID */
	uint32_t type;
	union {
		struct ctlra_dev_usb_hid_t usb_hid;
	};
};

/** Struct that provides info about the controller. Passed to the
 * application on probe().
 */
struct ctlra_dev_info_t {
	/** Name of the vendor/company */
	char vendor[CTLRA_STR_MAX];
	/** Name of the device */
	char device[CTLRA_STR_MAX];
	/** Serial as a string (if applicable) */
	char serial[CTLRA_DEV_SERIAL_MAX];

	/** USB Vendor ID */
	uint32_t vendor_id;
	/** USB device ID */
	uint32_t device_id;
	/** Serial as a number (if applicable) */
	uint64_t serial_number;
	/** The Unique ID assigned to this particular controller by the
	 * Ctlra library. A specific device (device + serial_number combo)
	 * is assigned an integer ID when accepted. This unique_id remains
	 * constant for the device - even if hot-plugged out and back in.
	 * 
	 * The use-case for this runtime-constant ID number is to allow an
	 * application to easily detect when a controller is "returning",
	 * or when a new controller is being plugged in.
	 */
	uint32_t unique_id;

	/** Number of controls the device has of each type. Read eg number
	 * buttons by accessing the array by *ctlra_event_type_t*
	 * CTRLA_EVENT_BUTTON */
	uint32_t control_count[CTLRA_EVENT_T_COUNT];

	/** An array pointers to ctlra_item_info_t structures.
	 * The pointers can be used to look up information about each
	 * control that the device has.
	 */
	struct ctlra_item_info_t *control_info[CTLRA_EVENT_T_COUNT];

	/* TODO: feedback/led only ctlra_item info */

	uint32_t size_x;
	uint32_t size_y;

	struct ctlra_grid_info_t grid_info[CTLRA_NUM_GRIDS_MAX];

	/** @internal function to get name from device. Application must
	 * use *ctlra_info_get_name* function. */
	ctlra_info_get_name_func get_name;
};

/** Callback function that gets invoked just before a device is removed,
 * or disconnected, by Ctlra. This allows the application to notify the
 * user of a disconnection, or free memory that was used by this instance.
 * The *unexpected_removal* parameter is set when eg: a cable is unplugged
 * unexpectedly, it is 0 on a clean shutdown.
 */
typedef void (*ctlra_remove_dev_func)(struct ctlra_dev_t *dev,
				      int unexpected_removal,
				      void *userdata);

/** A callback function that the application implements, called by the
 * Ctlra library when probing for supported devices. The application must
 * accept the device, and provide *ctlra_event_func* and userdata to Ctlra.
 * \retval 1 Accept the device - Ctlra will connect to the device
 * \retval 0 Reject the device - Ctlra does not connect, and the device
 *           is not used by this instance of Ctlra
 */
typedef int (*ctlra_accept_dev_func)(struct ctlra_t *ctlra,
				     const struct ctlra_dev_info_t *info,
				     struct ctlra_dev_t *dev,
				     void *userdata);

/* struct to represent a "zone" of a screen. Can be used for eg: redraw */
struct ctlra_screen_zone_t {
	uint32_t x;
	uint32_t y;
	uint32_t w;
	uint32_t h;
};

/** A callback function that the application must implement in order to
 * redraw a screen for a device. This callback is registered using the
 * *ctlra_dev_screen_register_callback* function.
 *
 * The application must write exactly the number of *bytes* specified, to
 * the location provided in *pixel_data. The data written must be formatted
 * in the devices native screen data type.
 *
 * In order to abstract the application from the device's native data
 * format, various functions are exposed to translate the data. For
 * example, the common Cairo library can be used to draw pixels, and the
 * cairo_surface_t * passed to the *ctlra_screen_cairo_to_device* helper
 * function, and then the appropriate pixel converion will take place.
 *
 * @retval 0 Screen will not be redrawn
 * @retval 1 Screen will fully redrawn
 * @retval 2 Screen will redraw only zone as indicated in *redraw_zone*
 */
typedef int32_t (*ctlra_screen_redraw_cb)(struct ctlra_dev_t *dev,
					  uint32_t screen_idx,
					  uint8_t *pixel_data,
					  uint32_t bytes,
					  struct ctlra_screen_zone_t *redraw_zone,
					  void *userdata);

/** Create a new ctlra context. This context holds state about the
 * connected devices, hotplug callbacks and backends (usb, bluetooth, etc)
 * for the controllers. The *opts* argument is a pointer to a struct
 * allowing specific control over the ctlra context being created. For
 * simple usage, pass NULL.
 */
struct ctlra_t *ctlra_create(const struct ctlra_create_opts_t *opts);

/** Probe for any devices that ctlra understands. This will depend on the
 * version of the Ctlra library, what compile options were enabled, and
 * the opts argument to ctlra_create(). This function causes the
 * ctlra_accept_dev callback function to be called in the application, once
 * for each device that is understood by Ctlra.
 * \retval The number of devices accepted and connected to successfully
 */
int ctlra_probe(struct ctlra_t *ctlra, ctlra_accept_dev_func accept_func,
		void *userdata);

/** Sets the feedback function for the device */
void ctlra_dev_set_feedback_func(struct ctlra_dev_t *dev,
				 ctlra_feedback_func func);

void ctlra_dev_set_callback_userdata(struct ctlra_dev_t *dev,
				     void *app_userdata);

/** Sets the screen redraw function for the device */
void ctlra_dev_set_screen_feedback_func(struct ctlra_dev_t *dev,
					ctlra_screen_redraw_cb func);

/** Sets the function that will be called on device removal */
void ctlra_dev_set_remove_func(struct ctlra_dev_t *dev,
			       ctlra_remove_dev_func func);

/** Iterate backends and see if anything has changed - this enables hotplug
 * detection and removal of devices.
 */
void ctlra_idle_iter(struct ctlra_t *ctlra);

/** Request Ctlra library to perform a screen refresh. This enables application
 * driven screen redrawing/refresh timeouts, to provide as timely screen updates
 * to controllers as is possible.
 */
void
ctlra_screen_request_redraw(struct ctlra_t *ctlra);

/** Cleanup any resources allocated internally in Ctlra. This function
 * releases all resources attached to this context, but does NOT interfere
 * with other ctlra instances */
void ctlra_exit(struct ctlra_t *ctlra);

/** Disconnect from controller device, resetting to a neutral state.
 * @param dev The device to be disconnected
 * @retval 0 Successfully disconnected
 * @retval -ENOTSUP Disconnect not supported by driver
 * @retval <0 Error in disconnecting device
 */
int32_t ctlra_dev_disconnect(struct ctlra_dev_t *dev);

/** Retrieve a list of vendors. The returned strings are human readable
 * names of the companies/manufacturers of controller hardware. This
 * function is to allow eg: a drop-down list of vendors be displayed to
 * the user, which are supported by ctlra (and can be virtualized).
 *
 * Note that the strings pointed to by vendors after this call *remain*
 * owned by the Ctlra library - and they must not be freed by the callee.
 *
 * @param vendors An array of *size* that will be filled in with null
 *                terminated strings, where each entry is a vendor's name.
 * @param size    Size of the callee supplied array
 * @returns       Number of vendors populated in the array
 */
int32_t ctlra_get_vendors(const char *vendors[], int32_t size);


/** Retrieve a list of devices by a specific vendor. The returned strings
 * are human-readable names of devices. These names could be presented to
 * the user in eg: a drop down box allowing the user to select a specific
 * device. One use case could be virtualizing a specific hardware device
 *
 * @param vendor The vendor from which to find all supported devices. See
 *               the *ctlra_get_vendors* function to retrieve vendors.
 * @param devices An array of at least *size* to be filled in with null
 *                terminated strings, with each string representing a device.
 * @param size   Size of the devices array passed in.
 * @return       The number of devices populated in the array
 */
int32_t ctlra_get_devices_by_vendor(const char *vendor,
				    const char *devices[],
				    int32_t size);

/** Add a virtualized device. This function adds a "virtual" device, which
 * provides the controls of the physical device by displaying a user
 * interface. In order for this function to operate, the device being
 * virtualized must provide its info statically in the driver.
 *
 * The info provided about the device is used to emulate the device
 * itself - the normal accept dev callback will be called in the
 * application, with the device descriptor filled out as if it was the
 * actual hardware plugged in. This allows total emulation of the hardware.
 *
 * @retval 0 Successfully virtualized the device using an available backend
 * @retval -1 Error in virtualizing
 */
int32_t ctlra_dev_virtualize(struct ctlra_t *ctlra, const char *vendor,
			     const char *device);

void ctlra_strerror(struct ctlra_t *ctlra, FILE* out);

/** Change the Event func. This may be useful when integrating into
 * an application that doesn't know yet which event func to attach to
 * the device when connecting to it. A dummy event_func can be used, and
 * the correct one set on the device when it is known.
 */
void ctlra_dev_set_event_func(struct ctlra_dev_t* dev,
			      ctlra_event_func event_func);

/** Write Lights/LEDs feedback to device. See *ctlra_dev_lights_flush* to
 * flush the actual bytes over the cable to the device.
 * The *light_id* is a value specific to the device that enumerates each
 * available light. The *light_status* variable represents the state of
 * the light, as a bitmask of 3 properties: blinking, brightness, colour.
 * The top bit (1 << 31) indicates if the light should be blinking.
 * The brightness (0x7F << 24) is a 0 to 127 brightness value.
 * The remaining 16 bits are encoded as 0xRRGGBB in hex.
 * Controllers should support these inputs as best they can for the given
 * light_id.
 */
void ctlra_dev_light_set(struct ctlra_dev_t *dev,
			uint32_t light_id,
			uint32_t light_status);

/** Feedback item set: sets the value for a feedback item */
void ctlra_dev_feedback_set(struct ctlra_dev_t *dev,
			    uint32_t fb_id,
			    float value);

/** Feedback item set: sets the value for a feedback item */
void ctlra_dev_feedback_digits(struct ctlra_dev_t *dev,
			       uint32_t feedback_id,
			       float value);

/** Flush the bytes with the Lights/LEDs info over the cable. The device
 * implementation must track which lights are actually dirty, and only
 * flush the bytes needed. If *force* is set, force flush everything.
 */
void ctlra_dev_light_flush(struct ctlra_dev_t *dev, uint32_t force);

/** Send Lights/LEDs feedback to a grid on the device. See
 * *ctlra_dev_light_set* documentation for details. The *grid_id* is
 * unique to the device, and allows easy access to lights in an array
 * or grid pattern.
 */
void ctlra_dev_grid_light_set(struct ctlra_dev_t *dev,
			     uint32_t grid_id,
			     uint32_t light_id,
			     uint32_t light_status);

/** @warning
 * @b DEPRECATED: this API has been superseeded, use the screen update
 * callback APIs instead.
 *
 * This function serves a dual purpose of getting the pixel data pointer
 * of the driver, and allows flushing the data to the device itself.
 * The function returns 0 on success, -ENOTSUP if the device doesn't
 * have a screen, or it is not yet supported.
 *
 * The *pixel_data* a pointer to where pixel data should be written,
 * where up to *bytes* may be written. Writing past *bytes* characters is
 * an error, and may cause memory corruption - don't do it. It is assumed
 * that the application writing the data knows the format to write.
 *
 * When *flush* is non-zero, the driver will flush the pixel data
 * from the driver to the device */

int32_t
#ifndef CTLRA_INTERNAL
DEPRECATED
#endif
ctlra_dev_screen_get_data(struct ctlra_dev_t *dev,
					 uint8_t **pixel_data,
					 uint32_t *bytes,
					 uint8_t flush);

/** Get the info struct from a device. The user-supplied info pointer is
 * filled in by the device driver */
void ctlra_dev_get_info(const struct ctlra_dev_t *dev,
		       struct ctlra_dev_info_t * info);

/** Get the human readable name for *control_id* from *dev*. The
 * control id is passed in eg: event.button.id, or can be any of the
 * DEVICE_NAME_CONTROLS enumeration. Ownership of the string *remains* in
 * the driver, so the application *must not* attempt to free the returned
 * pointer.
 * @retval 0 Invalid control_id requested
 * @retval Ptr A pointer to a string representing the control name
 */
const char *ctlra_info_get_name(const struct ctlra_dev_info_t *info,
				enum ctlra_event_type_t type,
				uint32_t control_id);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OPENAV_CTLRA_H */
