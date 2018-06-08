#ifndef OPENAV_LOOP_H
#define OPENAV_LOOP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct loop_t;

/** Create / Destroy loops */
struct loop_t* loop_new(int samplerate);
void  loop_free(struct loop_t*);

/** Reset a loop to initial state, clearing content */
void  loop_reset(struct loop_t*);
void  loop_reset_playhead(struct loop_t*);

/** Process nframes of audio */
void loop_process(struct loop_t*, int nframes, const float** in, float** out);

/** Start playing a loop. If no content is available, it records */
void loop_play(struct loop_t*);
int loop_playing(struct loop_t*);
int loop_recording(struct loop_t*);

/** Play the loop, overdubbing the incoming audio */
void loop_overdub(struct loop_t*);

/** Stop a loop, aka muting the output */
void loop_stop(struct loop_t*);

/** Returns the number of used audio frames */
uint32_t loop_get_frames(struct loop_t*);
void loop_set_frames(struct loop_t*, uint32_t frames);

/** Returns the number of frames before end of loop */
uint32_t loop_get_remaining_frames(struct loop_t*);

/** Returns the audio data for the channel
 * Channel 0 is left
 * Channel 1 is right
 */
float* loop_get_audio(struct loop_t*, int channel);

/* Internal free to access loop struct */
void loop_free_internal(struct loop_t* s);

/* Defined as a macro to set the variable itself to NULL */
#define loop_free(s) do { loop_free_internal(s); s = 0x0; } while (0)

#ifdef __cplusplus
}
#endif

#endif /* OPENAV_LOOP_H */

