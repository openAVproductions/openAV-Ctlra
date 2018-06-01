#include "audio.h"

struct buffer_t {
	/* interleaved audio */
	float *audio;
	uint32_t frames;
	uint8_t channels;
	uint8_t loaded;
};

struct voice_t {
	/* when non-zero, voice should be processed */
	uint8_t active;
	/* the buffer to play */
	uint8_t buffer_id;
	/* the frame of current playing */
	uint32_t playhead;
	/* frame to end play on */
	uint32_t end;
	/* volume of the sample */
	float volume;
};

#define MAX_BUFFERS 32
#define MAX_VOICES 64

struct sampler_t {
	struct buffer_t buffers[MAX_BUFFERS];
	struct voice_t voices[MAX_VOICES];
};

struct sampler_t *
sampler_init(int rate)
{
	struct sampler_t *s = calloc(1, sizeof(struct sampler_t));
	if(!s)
		return 0;

	return s;
}

void
smpla_sample_state(struct smpla_t *s, struct smpla_sample_state_t *d)
{
	assert(s);
	assert(d);
}

static inline int32_t
voice_process(struct voice_t *v,
	      struct buffer_t *buffer,
	      float *audio[],
	      uint32_t nframes)
{
	/* just quit if we're nearing end of sample */
	/* TODO: Add ADSR and glitch handling */
	uint32_t last_frame = v->playhead + nframes;
	if(last_frame >= v->end || last_frame >= buffer->frames) {
		v->active = 0;
		return -1;
	}

	float *l = audio[0];
	float *r = audio[1];

	/* interleaved stereo */
	uint32_t playhead = v->playhead * 2;
	v->playhead += nframes;

	for(int i = 0; i < nframes; i++) {
		l[i] += buffer->audio[playhead++];
		r[i] += buffer->audio[playhead++];
	}
	return 0;
}

uint32_t sampler_process(struct sampler_t *s,
			 float *audio[],
			 uint32_t nframes)
{
	for(int v = 0; v < MAX_VOICES; v++) {
		if(s->voices[v].active) {
			uint8_t buf = s->voices[v].buffer_id;
			if(!s->buffers[buf].loaded)
				continue;

			voice_process(&s->voices[v], &s->buffers[buf],
				      audio, nframes);
		}
	}

	return 0;
}
