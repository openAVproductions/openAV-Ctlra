
#include "global.h"

#include <jack/jack.h>
#include <sndfile.h>
#include <stdlib.h>
#include <string.h>

#include <fluidsynth.h>

struct soffa_t {
	fluid_synth_t*    synth;
	fluid_settings_t* settings;
	int sf_id;
};

struct soffa_t *
soffa_init(uint32_t sr)
{
	struct soffa_t *s = calloc(1, sizeof(struct soffa_t));
	if(!s) return 0;

	s->settings = new_fluid_settings();
	s->synth = new_fluid_synth(s->settings);
	fluid_synth_set_sample_rate(s->synth, sr);

	/* TODO: fluid_synth_sfunload(); */

	s->sf_id = fluid_synth_sfload(s->synth, "example.sf2", 0);
	if(s->sf_id == -1)
		goto fail;

	int midi_chan = 0;
	int bank = 0;
	int patch = 0;
	fluid_synth_program_select(s->synth, midi_chan, s->sf_id, bank, patch);
	fluid_synth_program_select(s->synth, midi_chan+1, s->sf_id, bank, patch + 4);

	fluid_synth_program_change(s->synth, 0, patch);

	return s;

fail:
	if(s)
		free(s);
	return 0;
}

void
soffa_note_on(struct soffa_t *s, uint8_t chan, uint8_t note, float vel)
{
	fluid_synth_noteon(s->synth, chan, note, vel * 127.f);
	printf("note on %d\n", note);
}

void
soffa_note_off(struct soffa_t *s, uint8_t chan, uint8_t note)
{
	fluid_synth_noteoff(s->synth, chan, note);
}

void
soffa_set_patch(struct soffa_t *s, uint8_t chan,
		uint8_t bank, uint8_t patch, const char **name)
{
	fluid_synth_program_select(s->synth, chan, s->sf_id, bank, patch);

	if(name) {
		fluid_synth_channel_info_t channelInfo;
		fluid_synth_get_channel_info(s->synth, 0, &channelInfo);
		*name = channelInfo.name;
	}
}

void soffa_process(struct soffa_t *s, int nframes, float** output)
{
	int ignored = 0;
	void *ignore_ptr = 0;
	fluid_synth_process(s->synth, nframes, ignored, ignore_ptr,
			    2, output);
}

jack_port_t* outputPort = 0;

static int playbackIndex = 0;

static uint32_t sample_size;
float* sample;

static jack_client_t* client;

int process(jack_nframes_t nframes, void* arg)
{
	struct dummy_data *d = arg;

	float* outputBuffer= (float*)jack_port_get_buffer ( outputPort, nframes);
	memset(outputBuffer, 0, nframes * sizeof(float));

	if(sample_size) {
		for ( int i = 0; i < (int) nframes; i++) {
			outputBuffer[i] = sample[playbackIndex % sample_size] * d->volume;
			playbackIndex++;
		}

		/* 0 - 1 ranged progress */
		float idx = playbackIndex % sample_size;
		d->progress = ((float)idx) / sample_size;
	}

	float *buf2[] = {outputBuffer, outputBuffer};
	soffa_process(d->soffa, nframes, buf2);

	for (int i = 0; i < (int) nframes; i++) {
		outputBuffer[i] *= 2;
	}

	/* mark dummy data updated, so controllers get updates */
	d->revision++;

	return 0;
}

int audio_init(void *dummy)
{
	/* Load a mono soundfile */
	SNDFILE *sf;
	SF_INFO info = {0};
	sf = sf_open("sample.wav", SFM_READ, &info);
	sample_size = info.frames;
	sample = malloc(sample_size*sizeof(float));
	uint32_t num = sf_read_float(sf, sample, sample_size);
	if(num != sample_size)
		printf("warning, load of sample returned %d, expected %d\n",
		       num, sample_size);
	sf_close(sf);

	/* setup JACK */
	client = jack_client_open("CtlraPlayer", JackNullOption, 0, 0 );
	int sr = jack_get_sample_rate(client);
	if(jack_set_process_callback(client, process, dummy)) {
		printf("error setting JACK callback\n");
	}
	outputPort = jack_port_register(client, "output",
	                                JACK_DEFAULT_AUDIO_TYPE,
	                                JackPortIsOutput, 0 );

	struct dummy_data *d = dummy;
	//int sr = 44100;
	d->soffa = soffa_init(sr);
	if(!d->soffa) {
		printf("soffa init() failed\n");
		exit(-1);
	}

	jack_activate(client);
	return 0;
}

void audio_exit()
{
	jack_deactivate(client);
	jack_client_close(client);
}
