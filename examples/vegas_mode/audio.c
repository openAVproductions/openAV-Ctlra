
#include "global.h"

#include <jack/jack.h>
#include <sndfile.h>
#include <stdlib.h>
#include <string.h>

jack_port_t* outputPort = 0;

static int playbackIndex = 0;

static uint32_t sample_size;
float* sample;

static jack_client_t* client;

int process(jack_nframes_t nframes, void* arg)
{
	float* outputBuffer= (float*)jack_port_get_buffer ( outputPort, nframes);
	memset(outputBuffer, 0, nframes * sizeof(float));

	if(!sample_size)
		return 0;

	struct dummy_data *d = arg;
	for ( int i = 0; i < (int) nframes; i++) {
		outputBuffer[i] = sample[playbackIndex % sample_size] * d->volume;
		playbackIndex++;
	}
	/* 0 - 1 ranged progress */
	float idx = playbackIndex % sample_size;
	d->progress = ((float)idx) / sample_size;
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
	jack_set_process_callback(client, process , dummy);
	outputPort = jack_port_register(client, "output",
	                                JACK_DEFAULT_AUDIO_TYPE,
	                                JackPortIsOutput, 0 );

	jack_activate(client);
	return 0;
}

void audio_exit()
{
	jack_deactivate(client);
	jack_client_close(client);
}
