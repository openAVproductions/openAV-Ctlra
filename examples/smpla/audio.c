
#include "audio.h"

#include <stdlib.h>

struct smpla_t *smpla_init(int rate)
{
	struct smpla_t *s = calloc(1, sizeof(struct smpla_t));
	if(!s)
		return 0;

	instanceInitforga(&s->forga, rate);
	s->sampler = sampler_init(rate);

	/* initialize rings for x-thread messaging */
	s->to_rt_ring = zix_ring_new(4096);
	if(!s->to_rt_ring) {
		printf("ERROR creating zix ctlra->rt ring\n");
	}
	s->to_rt_data_ring = zix_ring_new(4 * 4096);
	if(!s->to_rt_data_ring) {
		printf("ERROR creating zix ctlra->rt DATA ring\n");
	}

	/* testing */
	struct smpla_sample_state_t d = {
		.pad_id = 0,
		.action = 1,
		.sample_id = 0,
		.frame_start = 0,
		.frame_end = -1,
	};
	smpla_sample_state(s, &d);

	int ret = smpla_to_ctlra_write(s, smpla_sample_state, &d, sizeof(d));
	printf("write msg %d\n", ret);

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

	/* poll ring for functions to execute in RT context */
	struct smpla_rt_msg m = {0};
	uint32_t r = zix_ring_read(s->to_rt_ring, &m, sizeof(m));
	if(r == 0) {
		/* empty ring == no events available */
	} else if(r != sizeof(m)) {
		/* TODO: error handle here? Programming issue detected */
	} else {
		/* we have a f2_msg now, with a function pointer and a
		 * size of data to consume. Call the function with the
		 * read head of the ringbuffer, allowing function to use
		 * data from the ring, then consume data_size from the data
		 * ring so the next message pulls the next block of info.
		 */
		/* TODO: suboptimal usage here, because we copy the data
		 * out and into a linear array - but it probably already is
		 * Use a bip-buffer mechanism to avoid the copy here */
		const uint32_t ds = 256;
		char buf[ds];
		uint32_t read_size = m.data_size > ds ? ds : m.data_size;
		uint32_t data_r = zix_ring_read(s->to_rt_data_ring,
						buf, read_size);
		if(m.func && data_r == read_size)
			m.func(s, buf);
	}

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

