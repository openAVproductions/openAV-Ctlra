
#include "loop.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BUSKA_LOOP_MAX_LEN_SECS 30

#if 0
#define LOOP_DEBUG(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define LOOP_DEBUG(fmt, ...)
#endif

static int buska_loop_id = 0;

enum loop_state {
	LOOP_STOP = 0,
	LOOP_REC,
	LOOP_PLAY,
	LOOP_OVERDUB,
};

struct loop_t {
	/** meta data */
	uint32_t id;
	uint32_t sr;

	/** playback */
	int hasContent;
	uint32_t index;
	enum loop_state state;

	/** audio data */
	uint32_t length;
	float*   dataL;
	float*   dataR;
};

static void
loop_imp_record(struct loop_t* s, int nframes, const float** in, float** out)
{
	out = out;
	LOOP_DEBUG("loop %d process() : RECORD, has content = %d\n", s->id, s->hasContent);
	if( s->index + nframes > BUSKA_LOOP_MAX_LEN_SECS  * s->sr ) {
		s->state = LOOP_STOP;
		return;
	}

	const float* l = in[0];
	const float* r = in[1];
	float* L = &s->dataL[s->index];
	float* R = &s->dataR[s->index];

	int n = nframes;
	while( n-- ) {
		*L++ += *l++;
		*R++ += *r++;
	}

	if( s->state != LOOP_OVERDUB ) {
		s->length += nframes;
		s->hasContent = 1;
	}
}

static void
loop_imp_play( struct loop_t* s, int nframes, const float** in, float** out )
{
	in = in;
	if( !s->hasContent ) {
		s->state = LOOP_STOP;
		return;
	}
	LOOP_DEBUG("loop %d process() PLAY %d\n", s->id, s->hasContent);

	float* l = out[0];
	float* r = out[1];
	float* L = &s->dataL[s->index];
	float* R = &s->dataR[s->index];

	// n counts down processed samples
	int n = nframes;
	uint32_t fadeSamps = s->sr / 500;
	if(s->index < fadeSamps) {
		//LOOP_DEBUG("using fade up loop\n");
		uint32_t idx = s->index;
		float div = fadeSamps;
		while( n && idx < fadeSamps ) {
			float v = idx / (float)(div);
			*l++ = *L++ * v;
			*r++ = *R++ * v;
			idx++;
			n--;
		}
	}
	/* index + fade + nframes is boundary to *start* fade out */
	else if( s->index + fadeSamps + nframes >= s->length ) {
		// length - fade == startOfFade
		// so when index == startOfFade, switch loops
		int startFade = s->length - (s->index + fadeSamps + 1);

		// normal loop
		while( n && startFade ) {
			*l++ = *L++;
			*r++ = *R++;
			n--;
			startFade--;
		}

		//LOOP_DEBUG("using fade down loop : startFade = %d \n", startFade);
		int idx = fadeSamps;
		float div = fadeSamps;
		while( n && idx ) {
			float v = idx / (float)(div);
			*l++ = *L++ * v;
			*r++ = *R++ * v;
			idx--;
			n--;
		}
	}

	/* is samples left, do a full volume straight copy */
	while( n-- ) {
		*l++ = *L++;
		*r++ = *R++;
	}

	if( s->index >= s->length ) {
		s->index -= s->length;
	}
}

void loop_process(struct loop_t* s, int nframes, const float** in, float** out)
{
	assert( s );

	//LOOP_DEBUG("process with state %d, nframes %d\n", s->state, nframes);

	switch( s->state ) {
	case LOOP_STOP:
		return;
		break;

	case LOOP_REC:
		loop_imp_record( s, nframes, in, out );
		s->index += nframes;
		break;

	case LOOP_PLAY:
		loop_imp_play( s, nframes, in, out );
		s->index += nframes;
		break;

	case LOOP_OVERDUB: {
		loop_imp_play  ( s, nframes, in, out );
		loop_imp_record( s, nframes, in, out );
		s->index += nframes;
	}
	default:
		break;
	}
}

void loop_overdub( struct loop_t* s)
{
	assert( s );
	s->state = LOOP_OVERDUB;
}

uint32_t loop_get_remaining_frames( struct loop_t* s )
{
	assert( s );
	if( s->state == LOOP_PLAY )
		return s->length - s->index;

	return 0;
}

void loop_reset_playhead( struct loop_t* s )
{
	assert( s );
	s->index = 0;
}

void loop_reset( struct loop_t* s )
{
	LOOP_DEBUG("%s\n", __func__);
	assert( s );
	s->hasContent = 0;
	s->state = LOOP_STOP;
	s->length = 0;
	s->index = 0;
	memset( s->dataL, 0, sizeof(float) * s->sr * BUSKA_LOOP_MAX_LEN_SECS );
	memset( s->dataR, 0, sizeof(float) * s->sr * BUSKA_LOOP_MAX_LEN_SECS );
}


void loop_set_frames( struct loop_t* s, uint32_t frames)
{
	// TODO : Stub
}

void loop_play( struct loop_t* s )
{
	LOOP_DEBUG("%s\n", __func__);
	assert( s );
	if( s->hasContent )
		s->state = LOOP_PLAY;
	else
		s->state = LOOP_REC;

	s->index = 0;
}

int loop_playing(struct loop_t* s)
{
	return s->state != LOOP_STOP;
}

int loop_recording(struct loop_t* s)
{
	return s->state == LOOP_REC;
}

void loop_stop( struct loop_t* s )
{
	assert( s );
	s->state = LOOP_STOP;
	LOOP_DEBUG("stopped loop, state %d, length %d\n", s->state, s->length);
}

struct loop_t* loop_new( int sr )
{
	struct loop_t* s = (struct loop_t*)calloc( 1, sizeof(struct loop_t) );
	if( !s ) return NULL;

	s->id = buska_loop_id++;
	s->sr = sr;

	s->dataL = (float*)calloc( sr*BUSKA_LOOP_MAX_LEN_SECS, sizeof(float) );
	s->dataR = (float*)calloc( sr*BUSKA_LOOP_MAX_LEN_SECS, sizeof(float) );

	return s;
}

uint32_t loop_get_frames( struct loop_t* s )
{
	assert( s );
	return s->length;
}

float* loop_get_audio( struct loop_t* s, int chan )
{
	assert( s );
	if( chan == 0 ) return s->dataL;
	if( chan == 1 ) return s->dataR;
	return 0;
}

void loop_free_internal(struct loop_t* s)
{
	assert( s );
	if( s->dataL ) free( s->dataL );
	if( s->dataR ) free( s->dataR );
	free( s );
	s = 0x0;
}
