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

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/** @file
 * Ctlra is a lightweight C library for accessing controllers, to
 * make it easier to implement support for them in software. This library
 * abstracts the hardware and transport layer, providing events for all
 * inputs available, and functions that can be called to display feedback
 * on the device.
 */

#include "event.h"
#include "devices.h"

#define CTLRA_DEV_NAME_MAX   32
#define CTLRA_DEV_SERIAL_MAX 64

/** Connect to a controller device. */
struct ctlra_dev_t *ctlra_dev_connect(enum ctlra_dev_id_t dev_id,
				    ctlra_event_func event_func,
				    void *userdata,
				    void *future);

/** Poll device for events, causing an event to be emitted for each
 * event that has occured since the last poll.
 * @param dev The Device to poll.
 * @returns Number of events read from device.
 */
uint32_t ctlra_dev_poll(struct ctlra_dev_t *dev);

/** Disconnect from controller device, resetting to a neutral state.
 * @param dev The device to be disconnected
 * @retval 0 Successfully disconnected
 * @retval -ENOTSUP Disconnect not supported by driver
 * @retval <0 Error in disconnecting device
 */
int32_t ctlra_dev_disconnect(struct ctlra_dev_t *dev);

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

/** Struct that provides info about the controller */
struct ctlra_dev_info_t {
	/** Name of the vendor/company */
	char vendor[CTLRA_DEV_NAME_MAX];
	/** Name of the device */
	char device[CTLRA_DEV_NAME_MAX];
	/** Serial as a string (if applicable) */
	char serial[CTLRA_DEV_SERIAL_MAX];
	/** Serial as a number (if applicable) */
	uint64_t serial_number;

	/** Number of controls the device has of each type. Read eg number
	 * buttons by accessing th array by *ctlra_event_type_t*
	 * CTRLA_EVENT_BUTTON */
	uint32_t control_count[CTLRA_EVENT_T_COUNT];
};

/** Get the human readable name for the device. The returned pointer is
 * still owned by the ctlra library, the application must not free it */
void ctlra_dev_get_info(const struct ctlra_dev_t *dev,
		       struct ctlra_dev_info_t * info);

/** Get the human readable name for *control_id* from *dev*. The
 * control id is passed in eg: event.button.id, or can be any of the
 * DEVICE_NAME_CONTROLS enumeration. Ownership of the string *remains* in
 * the driver, so the application *must not* attempt to free the returned
 * pointer.
 * @retval 0 Invalid control_id requested
 * @retval Ptr A pointer to a string representing the control name
 * */
const char *ctlra_dev_control_get_name(const struct ctlra_dev_t *dev,
				       enum ctlra_event_type_t type,
				       uint32_t control_id);

/** Cleanup any resources allocated internally in Ctlra.
 * The application is expected to have called *ctlra_dev_disconnect*
 * on all open devices before calling this function. */
void ctlra_exit();

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OPENAV_CTLRA_H */
