#pragma once

int loopa_init();
void loopa_exit();

void loopa_playing(int r);
void loopa_recording(int r);
void loopa_reset();

void loopa_reverb(float v);
void loopa_delay_time(float v);

float loopa_progress();
void loopa_progress_set(float p);

float loopa_input_max(int channel);

/* getters */
int loopa_play_get(int c);
int loopa_rec_get(int c);

void loopa_record_toggle();
void loopa_playing_toggle();

float loopa_vol_get(int t);
void loopa_vol_set(int t, float v);

