
#include "audio.h"

#include <stdlib.h>
#include <unistd.h>

#include <ctlra/ctlra.h>


/* ctlra thread */
static void *
ctlra_thread_func(void *ud)
{
	struct smpla_t *s = ud;
	usleep(500 * 1000);

	while(1) {
		if(s->ctlra_thread_quit_now)
			break;

		/* TODO: replace with audio-thread time handling */
		const uint32_t sleep_time = 2000 * 1000 / (48000 / 128);

		if(!s->ctlra_thread_running) {
			usleep(sleep_time);
			continue;
		}

		ctlra_idle_iter(s->ctlra);
		usleep(sleep_time);
	}

	s->ctlra_thread_quit_done = 1;
	return 0;
}

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

	ZixStatus status = zix_thread_create(&s->ctlra_thread,
					     80000,
					     ctlra_thread_func,
					     s);
	if(status != ZIX_STATUS_SUCCESS) {
		printf("ERROR launching zix thread!!\n");
	}

	s->ctlra_thread_running = 1;

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

	for(int i = 0; i < 16; i++) {
		struct Sequencer *sequencer = s->sequencers[i];
		sequencer_process(sequencer, nframes);
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
smpla_to_rt_write(struct smpla_t *s,
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

