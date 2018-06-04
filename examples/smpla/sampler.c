#include "audio.h"

#include "audio_samples.h"

struct buffer_t {
	/* interleaved audio */
	const float *audio;
	uint32_t frames;
	uint8_t channels;
	uint8_t loaded;
	float gain;
};

struct voice_t {
	/* when non-zero, voice should be processed */
	uint8_t active;
	/* the buffer to play */
	uint8_t buffer;
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
	uint8_t voice_idx;
};

struct sampler_t *
sampler_init(int rate)
{
	struct sampler_t *s = calloc(1, sizeof(struct sampler_t));
	if(!s)
		return 0;

	s->buffers[0].audio = kick_wavetable;
	s->buffers[0].frames = kick_size/2;
	s->buffers[0].loaded = 1;

	s->buffers[1].audio = hat_wavetable;
	s->buffers[1].frames = hat_size/2;
	s->buffers[1].loaded = 1;

	return s;
}

float smpla_sample_vol_get(struct smpla_t *s, uint32_t sample_id)
{
	return s->sampler->buffers[sample_id].gain;
}
void smpla_sample_vol(struct smpla_t *s, void *data)
{
	struct smpla_sample_vol_t *d = data;
	s->sampler->buffers[d->sample_id].gain = d->vol * 1.5;
}

void
smpla_sample_state(struct smpla_t *smpla, void* data)
{
	struct smpla_sample_state_t *d = data;
	assert(smpla);
	assert(d);
	struct sampler_t *s = smpla->sampler;

	struct voice_t v = {
		.active = 1,
		.buffer = d->sample_id,
		.playhead = d->frame_start,
		.end = d->frame_end,
		.volume = s->buffers[d->sample_id].gain,
	};
	s->voices[s->voice_idx++] = v;
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

	const float g = v->volume;

	for(int i = 0; i < nframes; i++) {
		l[i] += buffer->audio[playhead++] * g;
		r[i] += buffer->audio[playhead++] * g;
	}
	return 0;
}

uint32_t sampler_process(struct sampler_t *s,
			 float *audio[],
			 uint32_t nframes)
{
	for(int v = 0; v < MAX_VOICES; v++) {
		if(s->voices[v].active) {
			uint8_t buf = s->voices[v].buffer;
			if(!s->buffers[buf].loaded)
				continue;

			voice_process(&s->voices[v], &s->buffers[buf],
				      audio, nframes);
		}
	}

	return 0;
}
