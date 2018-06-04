
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <ctlra/ctlra.h>
#include "audio.h"

#define SMPLA_JACK 1

/* for when to quit */
static volatile uint32_t done;

#ifdef SMPLA_JACK
#include <jack/jack.h>
static jack_client_t* client;
static jack_port_t* outputPortL;
static jack_port_t* outputPortR;

int process(jack_nframes_t nframes, void* arg)
{
	struct smpla_t *s = arg;

	float* buf_l = (float*)jack_port_get_buffer (outputPortL, nframes);
	memset(buf_l, 0, nframes * sizeof(float));
	float* buf_r = (float*)jack_port_get_buffer (outputPortR, nframes);
	memset(buf_r, 0, nframes * sizeof(float));

	const float *ins[2] = {buf_l, buf_r};
	float *outs[2] = {buf_l, buf_r};

	int ret = smpla_process(s, nframes, ins, outs);
	(void)ret;

	return 0;
}
#endif

void sighndlr(int signal)
{
	done = 1;
	printf("\n");
}

int main()
{
	signal(SIGINT, sighndlr);
	int sr = 48000;
#ifdef SMPLA_JACK
	/* setup JACK */
	client = jack_client_open("Smpla", JackNullOption, 0, 0 );
	sr = jack_get_sample_rate(client);
#endif

	struct smpla_t *s = smpla_init(sr);

	struct mm_t *mm = calloc(1, sizeof(struct mm_t));
	mm->mode = MODE_PADS;
	s->controller_data = mm;

	struct ctlra_t *ctlra = ctlra_create(NULL);
	int num_devs = ctlra_probe(ctlra, accept_dev_func, s);
	printf("sequencer: Connected controllers %d\n", num_devs);

	s->ctlra = ctlra;

#ifdef SMPLA_JACK
	if(jack_set_process_callback(client, process, s)) {
		printf("error setting JACK callback\n");
	}
	outputPortL = jack_port_register(client, "output_l",
	                                JACK_DEFAULT_AUDIO_TYPE,
	                                JackPortIsOutput, 0 );
	outputPortR = jack_port_register(client, "output_r",
	                                JACK_DEFAULT_AUDIO_TYPE,
	                                JackPortIsOutput, 0 );
	jack_activate(client);
#endif

	while(!done) {
		usleep(1000*1000);
	}

	ctlra_exit(ctlra);

	usleep(700);

	return 0;
}
