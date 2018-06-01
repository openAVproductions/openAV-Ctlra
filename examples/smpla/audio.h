#pragma once

#include <stdint.h>

struct smpla_t;

/* define struct + functions here that can be bound to func ptr comms */

struct smpla_t *smpla_init(int rate);
void smpla_free(struct smpla_t *s);

int smpla_process(struct smpla_t *s,
		  uint32_t nframes,
		  const float *inputs[],
		  float *outputs[]);
