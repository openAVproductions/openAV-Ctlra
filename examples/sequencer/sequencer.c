/*
 * Author: Harry van Haaren 2016
 *         harryhaaren@gmail.com
 *         OpenAV Productions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include "sequencer.h"

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct Sequencer {
	SeqEventCb cb;
	void* cb_user_data;

	int sr;
	int length;
	int counter;

	int note;

	int cur_step;
	int num_steps;
	int steps[MAX_STEPS];
};


Sequencer* sequencer_new( int sr )
{
	Sequencer* s = calloc( 1, sizeof(Sequencer) );
	if( !s ) return 0x0;

	s->sr = sr;
	sequencer_reset( s );
	return s;
}

void sequencer_process( Sequencer* s, int nframes )
{
	assert( s );
	assert( s->num_steps );

	// avoid stupidly short loops
	if( s->length < s->sr / 8 )
		return;

	int stepSize = s->length / s->num_steps;
	int nextBeatFrames = stepSize + s->cur_step * stepSize;

	s->counter += nframes;

	if( s->counter >= nextBeatFrames ) {
		/* check if step has value */
		if( s->steps[s->cur_step] ) {
			int frame = s->counter - nextBeatFrames;
			s->cb( frame, s->note, s->steps[s->cur_step], s->cb_user_data );
		}
		s->cur_step++;
	}

	if( s->counter > s->length ) {
		s->counter -= s->length;
		s->cur_step = 0;
	}

	/*
	int fpb = sequencer_bpm_to_frames( s->samplerate, s->bpm );
	s->frame_counter += nframes;
	if( s->frame_counter >= fpb )
	{
	  // play the step
	  s->frame_counter -= fpb;
	  // callback to handle event
	  // next step with wrap
	  if( ++s->cur_step >= s->num_steps )
	    s->cur_step = 0;
	}
	*/
}

void sequencer_reset_playhead( Sequencer* s )
{
	assert( s );
	s->cur_step = 0;
	s->counter = 0;
}

void sequencer_reset( Sequencer* s )
{
	assert( s );
	s->num_steps = 32;
	s->length = 0;
	s->cur_step = 0;
	s->counter = 0;

	int i;
	for( i = 0; i < MAX_STEPS; i++ ) {
		s->steps[i] = 0;
	}
}

void sequencer_set_callback( Sequencer* s, SeqEventCb cb, void* ud )
{
	assert( s );
	assert( cb );

	s->cb = cb;
	s->cb_user_data = ud;
}

void sequencer_set_num_steps( Sequencer* s, int steps )
{
	assert( s );
	assert( steps );
	s->num_steps = steps;
}

int sequencer_get_num_steps( Sequencer* s )
{
	assert( s );
	return s->num_steps;
}

void sequencer_set_length( Sequencer* s, int length )
{
	assert( s );
	s->length = length;
}

int sequencer_get_length( Sequencer* s )
{
	assert( s );
	return s->length;
}

void sequencer_set_note( Sequencer* s, int note )
{
	assert( s );
	s->note = note;
}

int sequencer_get_note( Sequencer* s )
{
	assert( s );
	return s->note;
}

void sequencer_set_step( Sequencer* s, int step, int value )
{
	assert( s );
	assert( step >= 0 );
	assert( step < MAX_STEPS );

	s->steps[step] = value;
}

void sequencer_toggle_step( Sequencer* s, int step )
{
	assert( s );
	assert( step >= 0 );
	assert( step < MAX_STEPS );
	s->steps[step] = !s->steps[step];
}

int sequencer_get_step( Sequencer* s, int step )
{
	assert( s );
	assert( step >= 0 );
	assert( step < s->num_steps );
	return s->steps[step];
}

int sequencer_get_current_step( Sequencer* s )
{
	assert( s );
	assert( s->cur_step >= 0 );
	assert( s->cur_step < MAX_STEPS );
	return s->cur_step;
}


void sequencer_free( Sequencer* s )
{
	assert( s );

	free( s );
}
