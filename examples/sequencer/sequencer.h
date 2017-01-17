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


#ifndef OPENAV_SEQUENCER_H
#define OPENAV_SEQUENCER_H

#ifdef __cplusplus
extern "C"
{
#endif

#define MAX_STEPS 128
typedef struct Sequencer Sequencer;

/** Callback for when an event takes place */
typedef void (*SeqEventCb)( int frame,
                            int note,
                            int velocity,
                            void* user_data );

/** Creates a new Sequencer instance */
Sequencer* sequencer_new( int sr );

/** Reset the state of a sequencer */
void sequencer_reset( Sequencer* s );
void sequencer_reset_playhead( Sequencer* s );

/** Sequencer Process
 * The sequencer will process nframes worth of time, calling SeqEventCb
 * for each event to process. */
void sequencer_process( Sequencer*, int nframes );

/** Set the user data passed with callback */
void sequencer_set_callback( Sequencer*, SeqEventCb event_cb, void* ud );

/** Sets the lenght of the loop in audio frames*/
void sequencer_set_length( Sequencer*, int frames );
int  sequencer_get_length( Sequencer* );

/** Sets the number of steps in the sequencer */
void sequencer_set_num_steps( Sequencer*, int steps );
int  sequencer_get_num_steps( Sequencer* );

/** Sets this sequences note */
void sequencer_set_note( Sequencer*, int note );
int  sequencer_get_note( Sequencer* );

/** Sets a steps value */
void sequencer_set_step   ( Sequencer*, int step, int value );
void sequencer_toggle_step( Sequencer*, int step );
int  sequencer_get_step   ( Sequencer*, int step );

/** Get the current step value */
int sequencer_get_current_step( Sequencer* s );

/** free a sequencer instance */
void sequencer_free( Sequencer* );

#ifdef __cplusplus
}
#endif


#endif /* OPENAV_SEQUENCER_H */
