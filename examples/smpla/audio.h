#pragma once

#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>


#include "sequencer.h"

#include "dsp_forga.h"

#include "zix/ring.h"
#include "zix/thread.h"

struct smpla_t {
	forga_t forga;
	struct sampler_t *sampler;
	Sequencer *sequencers[16];


	void *controller_data;
	void *cairo_img;
	void *cairo_cr;

	struct ctlra_t *ctlra;
	/* rings for passing smpla_rt_msg structs */
	ZixRing *to_rt_data_ring;
	ZixRing *to_rt_ring;
	ZixThread ctlra_thread;
	volatile uint32_t ctlra_thread_running;
	volatile uint32_t ctlra_thread_quit_now;
	volatile uint32_t ctlra_thread_quit_done;
};

/* message function prototype */
typedef void (*smpla_rt_msg_func)(struct smpla_t *self, void *func_data);
/* message structure to pass through ring */
struct smpla_rt_msg {
	/* function to call */
	smpla_rt_msg_func func;
	/* size of data to consume */
	uint32_t data_size;
	/* padding to 16B */
	uint32_t padding;
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
void smpla_sample_state(struct smpla_t *s, void *data);

/* cross-thread message passing */
int smpla_to_rt_write(struct smpla_t *s, smpla_rt_msg_func func,
		      void *data, uint32_t size);



/* SAMPLER functions */
struct sampler_t *sampler_init(int rate);
uint32_t sampler_process(struct sampler_t *s,
			 float *audio[],
			 uint32_t nframes);

/* SMPLA container functions */
struct smpla_t *smpla_init(int rate);
void smpla_free(struct smpla_t *s);

int smpla_process(struct smpla_t *s,
		  uint32_t nframes,
		  const float *inputs[],
		  float *outputs[]);

