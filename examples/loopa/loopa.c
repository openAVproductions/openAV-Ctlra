#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jack/jack.h>

#include "oav_reverb.h"

static jack_port_t *input_port;
static jack_port_t *output_port;
static jack_client_t *client;

#define AUDIO_MAX (48000 * 60 * 5)
static float audio[AUDIO_MAX];

#define NTRACKS 8
static float vols[NTRACKS];

static uint32_t playhead;
static uint32_t length;
static uint32_t audio_present;
static uint32_t recording;
static uint32_t playing;

roomy_t *roomy;

void loopa_reverb(float v)
{
	// dB gain for reverb
	roomy->fVslider1 = (v * 2.f) - 1.f;// - 30;
	printf("vslider1 = %f, v = %f\n", roomy->fVslider1, v);
}

int
process(jack_nframes_t nframes, void *arg)
{
	jack_default_audio_sample_t *in;
	in = jack_port_get_buffer (input_port, nframes);

	jack_default_audio_sample_t *out;
	out = jack_port_get_buffer (output_port, nframes);

	memset(out, 0, sizeof (jack_default_audio_sample_t) * nframes);

	for(int i = 0; i < nframes; i++) {
		if(recording) {
			audio[playhead] += in[i];
			if(!audio_present) {
				length++;
			}
		}
		if(playing && audio_present) {
			out[i] = audio[playhead] * vols[0] * 2;
		}

		if(playing || recording) {
			playhead++;
			if(playhead > length) {
				playhead = 0;
			}
		}

		/* reverb */
		float wasteL[1] = {0};
		float wasteR[1] = {0};
		float tmp_in = out[i];
		float tmp_out = 0;
		float *ins[] = {&tmp_in, wasteL};
		float *outs[] = {&tmp_out, wasteR};
		computeroomy_t(roomy, 1, ins, outs);

		out[i] = tmp_out;
	}


	return 0;
}

void loopa_playing(int p)
{
	printf("playing %d\n", p);
	playing = p;
	if(playing == 0)
		playhead = 0;
}

float loopa_vol_get(int t)
{
	return vols[t];
}

int loopa_play_get(int c)
{
	return playing;
}
int loopa_rec_get(int c)
{
	return recording;
}

void loopa_vol_set(int t, float v)
{
	printf("set vols %f\n", v);
	vols[t] = v;
}

void loopa_playing_toggle()
{
	loopa_playing(!playing);
}

void loopa_recording(int r)
{
	printf("recording %d\n", r);
	if(recording && r == 0)
		audio_present = 1;

	recording = r;
}

void loopa_record_toggle()
{
	loopa_recording(!recording);
}

void loopa_reset()
{
	printf("reset()\n");
	playhead = 0;
	length = 0;
	recording = 0;
	playing = 0;
	audio_present = 0;
	memset(audio, 0, sizeof(audio));
}

void
jack_shutdown (void *arg)
{
	exit (1);
}

int loopa_init()
{
	const char **ports;
	const char *client_name = "CtlraLoopa";
	const char *server_name = NULL;
	jack_options_t options = JackNullOption;
	jack_status_t status;

	/* open a client connection to the JACK server */
	client = jack_client_open (client_name, options, &status, server_name);
	if (client == NULL) {
		fprintf (stderr, "jack_client_open() failed, "
		         "status = 0x%2.0x\n", status);
		if (status & JackServerFailed) {
			fprintf (stderr, "Unable to connect to JACK server\n");
		}
		exit (1);
	}
	if (status & JackServerStarted) {
		fprintf (stderr, "JACK server started\n");
	}
	if (status & JackNameNotUnique) {
		client_name = jack_get_client_name(client);
		fprintf (stderr, "unique name `%s' assigned\n", client_name);
	}
	jack_set_process_callback (client, process, 0);
	jack_on_shutdown (client, jack_shutdown, 0);

	uint32_t sr = jack_get_sample_rate(client);
	printf ("engine sample rate: %" PRIu32 "\n", sr);

	input_port = jack_port_register (client, "input",
	                                 JACK_DEFAULT_AUDIO_TYPE,
	                                 JackPortIsInput, 0);
	output_port = jack_port_register (client, "output",
	                                  JACK_DEFAULT_AUDIO_TYPE,
	                                  JackPortIsOutput, 0);
	printf("JACK output port at init  %p\n", output_port);

	if ((input_port == NULL) || (output_port == NULL)) {
		fprintf(stderr, "no more JACK ports available\n");
		exit (1);
	}
	if (jack_activate (client)) {
		fprintf (stderr, "cannot activate client");
		exit (1);
	}

	ports = jack_get_ports (client, NULL, NULL,
	                        JackPortIsPhysical|JackPortIsInput);
	if (ports == NULL) {
		fprintf(stderr, "no physical playback ports\n");
		exit (1);
	}

	if (jack_connect (client, jack_port_name (output_port), ports[0])) {
		fprintf (stderr, "cannot connect output ports\n");
	}
	if (jack_connect (client, jack_port_name (output_port), ports[1])) {
		fprintf (stderr, "cannot connect output ports\n");
	}

	free (ports);


	roomy = newroomy_t();
	initroomy_t(roomy, sr);

	return 0;
}

void loopa_exit()
{
	// cleanup TODO
}

