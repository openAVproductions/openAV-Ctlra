
#include "midi.h"

#include <alsa/asoundlib.h>

/* for snd_seq_port_info_alloca() macro expansion */
#include <alloca.h>

struct ctlra_midi_t {
	snd_seq_t *seq;
	snd_midi_event_t *encoder;
	snd_midi_event_t *decoder;
	int port_in;
	int port_out;
	ctlra_midi_input_cb input_cb;
	void *input_cb_ud;
};

/* Create a single input and single output port for communicating with
 * MIDI controllers */
struct ctlra_midi_t *ctlra_midi_open(const char *name,
				     ctlra_midi_input_cb cb,
				     void *userdata)
{
	printf("%s : name = %s\n", __func__, name);
	struct ctlra_midi_t *m = calloc(1, sizeof(struct ctlra_midi_t));
	if(!m) return 0;
	int res = snd_seq_open(&m->seq, "default",
	                       SND_SEQ_OPEN_DUPLEX, SND_SEQ_NONBLOCK);
	if (res < 0) {
		fprintf(stderr, "%s: failed to open seq\n", __func__);
		return 0;
	}

	snd_seq_set_client_name(m->seq, name);
	snd_seq_port_info_t *pinfo;
	snd_seq_port_info_alloca(&pinfo);

	/* input port */
	snd_seq_port_info_set_capability(pinfo,
	                                 SND_SEQ_PORT_CAP_WRITE |
	                                 SND_SEQ_PORT_CAP_SUBS_WRITE);
	snd_seq_port_info_set_type(pinfo,
	                           SND_SEQ_PORT_TYPE_MIDI_GENERIC |
	                           SND_SEQ_PORT_TYPE_APPLICATION);
	snd_seq_port_info_set_midi_channels(pinfo, 16);

	char buf[64];
	snprintf(buf, sizeof(buf), "%s_input", name);
	snd_seq_port_info_set_name(pinfo, buf);
	m->port_in = snd_seq_create_port(m->seq, pinfo);
	if (m->port_in < 0) {
		fprintf(stderr, "%s: failed to open port: %d\n",
		        __func__, m->port_in);
		return 0;
	}

	/* output port */
	snprintf(buf, sizeof(buf), "%s_output", name);
	m->port_out = snd_seq_create_simple_port(m->seq, buf,
					   SND_SEQ_PORT_CAP_READ |
						SND_SEQ_PORT_CAP_SUBS_READ,
					   SND_SEQ_PORT_TYPE_MIDI_GENERIC |
						SND_SEQ_PORT_TYPE_APPLICATION);
	snd_seq_port_info_set_midi_channels(pinfo, 16);
	if (m->port_out < 0) {
		fprintf(stderr, "%s: failed to open port: %d\n",
		        __func__, m->port_out);
		return 0;
	}

	/* initialize encoder / decoders */
	res = snd_midi_event_new(0, &m->encoder);
	if(res < 0)
		printf("%s: error creating encoder\n", __func__);
	snd_midi_event_no_status(m->encoder, 1);

	res = snd_midi_event_new(32, &m->decoder);
	if (res < 0)
		printf("%s: error creating decoder\n", __func__);
	snd_midi_event_init(m->decoder);

	/* Keep callback / ud */
	m->input_cb = cb;
	m->input_cb_ud = userdata;

	return m;
}

void ctlra_midi_destroy(struct ctlra_midi_t *s)
{
	snd_seq_delete_port(s->seq, s->port_in);
	snd_seq_delete_port(s->seq, s->port_out);
	snd_midi_event_free(s->encoder);
	snd_midi_event_free(s->decoder);
	snd_seq_close(s->seq);
	free(s);
}

int ctlra_midi_output_write(struct ctlra_midi_t *s, uint8_t nbytes,
                            uint8_t * buffer)
{
	snd_seq_event_t seq_ev;
	snd_seq_ev_clear(&seq_ev);
	snd_seq_ev_set_source(&seq_ev, s->port_out);
	snd_seq_ev_set_subs(&seq_ev);
	snd_seq_ev_set_direct(&seq_ev);

	int res = snd_midi_event_encode(s->decoder, buffer, nbytes, &seq_ev);
	if (res < nbytes)
		return -EINVAL;

	res = snd_seq_event_output(s->seq, &seq_ev);
	if (res < 0)
		return -ENOSPC;

	res = snd_seq_drain_output(s->seq);
	return nbytes;
}

int ctlra_midi_input_poll(struct ctlra_midi_t *s)
{
	int res;
	int nbytes;
	uint8_t buffer[3];
	snd_seq_event_t *seq_ev;

	int input_pending = 1;

	while (input_pending) {
		res = snd_seq_event_input(s->seq, &seq_ev);
		if(res < 0)
			return 0;

		input_pending = snd_seq_event_input_pending(s->seq, 1);
		if (input_pending < 0) {
			snd_seq_free_event(seq_ev);
			return 0;
		}

		nbytes= snd_midi_event_decode(s->encoder, buffer, 3, seq_ev);
		if (nbytes < 0) {
			continue;
		}

		s->input_cb(nbytes, buffer, s->input_cb_ud);

		snd_seq_free_event(seq_ev);
	}

	return 0;
}
