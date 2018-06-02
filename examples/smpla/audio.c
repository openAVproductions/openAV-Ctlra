
#include "audio.h"

#include <stdlib.h>

struct smpla_t *smpla_init(int rate)
{
	struct smpla_t *s = calloc(1, sizeof(struct smpla_t));
	if(!s)
		return 0;

	instanceInitforga(&s->forga, rate);
	s->sampler = sampler_init(rate);

	return s;
}

void smpla_free(struct smpla_t *s)
{
	free(s);
}

int smpla_process(struct smpla_t *s,
		  uint32_t nframes,
		  const float *inputs[],
		  float *outputs[])
{
	const float *in_l = inputs[0];
	const float *in_r = inputs[1];
	float *out_l = outputs[0];
	float *out_r = outputs[1];


	for(int i = 0; i < nframes; i++) {
		out_l[i] = in_l[i];
		out_r[i] = in_r[i];
	}

	float *audio[2] = {out_l, out_r};
	uint32_t ret = sampler_process(s->sampler,
				       audio,
				       nframes);
	(void)ret;

	return 0;
}

int
smpla_to_ctlra_write(struct smpla_t *s,
		     smpla_rt_msg_func func,
		     void *data, uint32_t size)
{
	/* message to enqueue into ring */
	struct smpla_rt_msg m = {
		.func = func,
		.data_size = size,
	};

	/* TODO: check write space available */

	uint32_t dw = zix_ring_write(s->to_rt_data_ring, data, size);
	if(dw != size) {
		printf("error didn't write data to ring: %d\n", dw);
	}

	uint32_t w = zix_ring_write(s->to_rt_ring, &m, sizeof(m));
	if(w != sizeof(m)) {
		printf("error didn't write full msg to ring: %d\n", w);
	}

	return 0;
}

