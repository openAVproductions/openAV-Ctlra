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

#ifndef CTLRA_MIDI_H
#define CTLRA_MIDI_H

#include <stdint.h>

struct ctlra_midi_t;

/** the callback that will be called for each input event */
typedef int (*ctlra_midi_input_cb)(uint8_t nbytes,
				   uint8_t * buffer,
				   void *ud);

/** Open an ALSA MIDI port for Controller I/O */
struct ctlra_midi_t *ctlra_midi_open(const char *name,
				     ctlra_midi_input_cb cb,
				     void *userdata);

/** Cleanup the MIDI I/O ports */
void ctlra_midi_destroy(struct ctlra_midi_t *s);

/** Call this function to write MIDI output */
int ctlra_midi_output_write(struct ctlra_midi_t *s, uint8_t nbytes,
                            uint8_t * buffer);

/** Call this to poll for input. This results in the callback getting
 * called once for each input event */
int ctlra_midi_input_poll(struct ctlra_midi_t *s);

#endif /* CTLRA_MIDI_H */
