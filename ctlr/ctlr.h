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

#ifndef OPENAV_CTLR_H
#define OPENAV_CTLR_H

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
#endif

#include <stdint.h>

/** @file
 * Ctlr is a lightweight C library for accessing controllers, to
 * make it easier to implement support for them in software. This library
 * abstracts the hardware and transport layer, providing events for all
 * inputs available, and functions that can be called to display feedback
 * on the device.
 */

#include "event.h"
#include "devices.h"

/** Connect to a controller device. */
struct ctlr_dev_t *ctlr_dev_connect(enum ctlr_dev_id_t dev_id,
				    ctlr_event_func event_func,
				    void *userdata,
				    void *future);

/** Poll device for events, causing event callback to be called for each
 * event that has occured since the last poll.
 * @param dev The Device to poll.
 * @retval  0 No events available from device.
 */
uint32_t ctlr_dev_poll(struct ctlr_dev_t *dev);

/** Disconnect from controller device, resetting to a neutral state.
 * @param dev The device to be disconnected
 * @retval 0 Successfully disconnected
 * @retval -ENOTSUP Disconnect not supported by driver
 * @retval <0 Error in disconnecting device
 */
int32_t ctlr_dev_disconnect(struct ctlr_dev_t *dev);

/** Send Lights/LEDs feedback to device.
 * The *light_id* is a value specific to the device that enumerates each
 * available light. The *light_status* variable represents the state of
 * the light, as a bitmask of 3 properties: blinking, brightness, colour.
 * The top bit (1 << 31) indicates if the light should be blinking.
 * The brightness (0x7F << 24) is a 0 to 127 brightness value.
 * The remaining 16 bits are encoded as 0xRRGGBB in hex.
 * Controllers should support these inputs as best they can for the given
 * light_id.
 */
void ctlr_dev_light_set(struct ctlr_dev_t *dev,
			uint32_t light_id,
			uint32_t light_status);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OPENAV_CTLR_H */
