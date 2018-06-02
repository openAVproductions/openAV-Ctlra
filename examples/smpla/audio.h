#pragma once

#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include "dsp_forga.h"

struct smpla_t {
	forga_t forga;
	struct sampler_t *sampler;
};

/* define struct + functions here that can be bound to func ptr comms */
struct smpla_sample_state_t {
	/* the 0 - 15 pad # to operate on */
	uint8_t pad_id;
	/* actions:
	 * 0) stop (also stops recording)
	 * 1) start playback from frame_start, to frame_end
	 * 2) record:
	 */
	uint8_t action;
	/* the ID of the sample buffer itself. If 0 create a new one */
	uint16_t sample_id;
	/* sample playback */
	uint32_t frame_start;
	uint32_t frame_end;
};
void smpla_sample_state(struct smpla_t *s,
			struct smpla_sample_state_t *d);

/* SAMPLER functions */
struct sampler_t *sampler_init(int rate);

/* SMPLA container functions */
struct smpla_t *smpla_init(int rate);
void smpla_free(struct smpla_t *s);
int smpla_process(struct smpla_t *s,
                  uint32_t nframes,
                  const float *inputs[],
                  float *outputs[]);
