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

#ifndef OPENAV_CTLRA_NI_KONTROL_F1_H
#define OPENAV_CTLRA_NI_KONTROL_F1_H

enum NI_KONTROL_F1_SLIDERS {
	NI_KONTROL_F1_SLIDER_FILTER_1,
	NI_KONTROL_F1_SLIDER_FILTER_2,
	NI_KONTROL_F1_SLIDER_FILTER_3,
	NI_KONTROL_F1_SLIDER_FILTER_4,
	NI_KONTROL_F1_SLIDER_FADER_1,
	NI_KONTROL_F1_SLIDER_FADER_2,
	NI_KONTROL_F1_SLIDER_FADER_3,
	NI_KONTROL_F1_SLIDER_FADER_4,
};

enum NI_KONTROL_F1_BUTTONS {
	NI_KONTROL_F1_BTN_SHIFT,
	NI_KONTROL_F1_BTN_REVERSE,
	NI_KONTROL_F1_BTN_TYPE,
	NI_KONTROL_F1_BTN_SIZE,
	NI_KONTROL_F1_BTN_BROWSE,
	NI_KONTROL_F1_BTN_ENCODER_PRESS,
	NI_KONTROL_F1_BTN_STOP_1,
	NI_KONTROL_F1_BTN_STOP_2,
	NI_KONTROL_F1_BTN_STOP_3,
	NI_KONTROL_F1_BTN_STOP_4,
	NI_KONTROL_F1_BTN_SYNC,
	NI_KONTROL_F1_BTN_QUANT,
	NI_KONTROL_F1_BTN_CAPTURE,
};

#endif /* OPENAV_CTLRA_NI_KONTROL_F1_H */

