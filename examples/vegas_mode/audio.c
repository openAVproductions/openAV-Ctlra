
#include "global.h"

#include <jack/jack.h>
#include <sndfile.h>
#include <stdlib.h>

jack_port_t* outputPort = 0;

int playbackIndex = 0;

static uint32_t sample_size;
float* sample;

static jack_client_t* client;

int process(jack_nframes_t nframes, void* arg)
{
	float* outputBuffer= (float*)jack_port_get_buffer ( outputPort, nframes);
	for ( int i = 0; i < (int) nframes; i++) {
		outputBuffer[i] = sample[playbackIndex % 48000];
		playbackIndex++;
	}
	return 0;
}

int audio_init()
{
	// load a sample or something here
	sample = calloc(1, 48000*10);
	sample_size = 48000;

	client = jack_client_open("CtlraPlayer",
	                        JackNullOption, 0, 0 );

	jack_set_process_callback(client, process , 0);

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
