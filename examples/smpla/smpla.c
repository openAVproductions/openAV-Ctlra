#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <ctlra/ctlra.h>
#include <caira.h>
#include "audio.h"

struct col_t
{
	uint8_t green;
	uint8_t blue;
	uint8_t red;
};
static const struct col_t grp_col[] = {
	{0x00, 0x92,  254},
	{0xf9, 0x21, 0x00},
	{0x00, 0xf0, 0x00},
	{0x00, 0x00, 0xf0},
	{0xf0, 0xf0, 0xb0},
	{0x3f, 0x3f, 0xf0},
	{0xf0, 0x00, 0xf0},
	{0x00, 0xef, 0xd5},
	/* dark */
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
};

uint32_t col_dim(const struct col_t *col, float bri)
{
	struct col_t c;
	c.red   = col->red   * bri;
	c.green = col->green * bri;
	c.blue  = col->blue  * bri;
	uint32_t intCol = *(uint32_t *)&c;
	return intCol;
}

void machine3_feedback_func(struct ctlra_dev_t *dev, void *d)
{
	/* group, pads, sequencer modes */
	struct smpla_t *s = d;

	struct mm_t *mm = s->controller_data;
	int static_mute = 0;

	const struct col_t *col = &grp_col[mm->grp_id];
	struct Sequencer *sequencer = s->sequencers[mm->pattern_pad_id];

	switch(mm->mode) {
	case MODE_PATTERN: {
		/* layout all steps */
		for(int i = 0; i < 16; i++) {
			int on = sequencer_get_step(sequencer, i);
			ctlra_dev_light_set(dev, 87 + i,
					    col_dim(col, on ? 0.4 : 0.0));
		}

		/* highlight current step */
		int step = sequencer_get_current_step( sequencer );
		int on = sequencer_get_step(sequencer, step % 16);
		//printf("step %d on %d\n", step, on);
		if(on) {
			ctlra_dev_light_set(dev, 87 + step,
					    col_dim(&grp_col[mm->grp_id], 1.0));
		}

		if(mm->shift_pressed) {
			//printf("setting pattern %d\n", mm->pattern_pad_id);
			ctlra_dev_light_set(dev, 87 + mm->pattern_pad_id,
					    0xff0000ff);
		}
		break;
		}
	case MODE_GROUP:
		for(int i = 0; i < 16; i++) {
			ctlra_dev_light_set(dev, 87 + i,
				col_dim(&grp_col[i], i == mm->grp_id ? 1.0 : 0.05));
		}
		break;
	case MODE_PADS:
		/* pressed pads (manually - hence overrides */
		for(int i = 0; i < 16; i++) {
			ctlra_dev_light_set(dev, 87 + i,
					    col_dim(&grp_col[mm->grp_id],
						    mm->pads_pressed[i] ? 1.0 : 0));
		}
		/* sequencer playbacks */
		for(int i = 0; i < 16; i++) {
			if(mm->pads_seq_play[i] && !static_mute)
				ctlra_dev_light_set(dev, 87 + i, 0xffffffff);
		}
		break;
	}

	ctlra_dev_light_set(dev, 57, static_mute ? 0xffffffff : 0x3);
	ctlra_dev_light_set(dev, 46, mm->mode == MODE_PADS ? 0xffffffff : 0x3);
	ctlra_dev_light_set(dev, 50, mm->mode == MODE_GROUP ? 0xffffffff : 0x3);
	ctlra_dev_light_set(dev, 49, mm->mode == MODE_PATTERN ? 0xffffffff : 0x3);

	ctlra_dev_light_flush(dev, 1);
}

/* TODO: remove dependency on this */
#define SR_CHANNELS r, g, b, a
typedef union {
  unsigned int word;
  struct { unsigned char SR_CHANNELS; } rgba;
} sr_Pixel;

static inline void
screen_draw_mixer(struct smpla_t *s, caira_t *cr)
{
	struct mm_t *mm = s->controller_data;

	for(int i = 0; i < 16; i++) {
		Sequencer *seq = s->sequencers[i];
		int steps = sequencer_get_num_steps(seq);
		int note  = sequencer_get_note(seq);
		int cur   = sequencer_get_current_step(seq);

		float col[9] = {
			0.25, 0.25, 0.25,
			0, 0x71 / 255.f, 1,
			1, 1, 1,
		};

		float vol = smpla_sample_vol_get(s, i);
		caira_set_source_rgb(cr, 0, 0, 0);
		const float yoff = 80;
		caira_rectangle(cr, i * 30, yoff + 10, 25, 150);
		caira_fill(cr);
		caira_set_source_rgb(cr, col[0], col[1], col[2]);
		const float h = 150;
		const float vpx = h * (vol * 0.665);
		caira_rectangle(cr, i * 30, yoff + 10 + h - vpx, 25, vpx);
		caira_fill(cr);
	}
}

static inline void
screen_draw_sequencers(struct smpla_t *s, caira_t *cr)
{
	struct mm_t *mm = s->controller_data;

	for(int i = 0; i < 16; i++) {
		Sequencer *seq = s->sequencers[i];
		int steps = sequencer_get_num_steps(seq);
		int note  = sequencer_get_note(seq);
		int cur   = sequencer_get_current_step(seq);

		float col[9] = {
			0.25, 0.25, 0.25,
			0, 0x71 / 255.f, 1,
			1, 1, 1,
		};

		float vol = smpla_sample_vol_get(s, i);
		caira_set_source_rgb(cr, 0, 0, 0);
		caira_rectangle(cr, 100, i * 17, 100, 15);
		caira_fill(cr);
		caira_set_source_rgb(cr, col[0], col[1], col[2]);
		caira_rectangle(cr, 100, i * 17, 100 * (vol * (0.6665)), 15);
		caira_fill(cr);

		for(int j = 0; j < steps; j++) {
			int on = sequencer_get_step(seq, j) * 3;
			on += ((cur == j) * 3) * (on > 0);
			caira_set_source_rgb(cr, col[on+0], col[on+1], col[on+2]);
			caira_rectangle(cr, 208 + j * 17, i * 17, 15, 15);
			caira_fill(cr);
		}
	}
}

int32_t
machine3_screen_redraw_func(struct ctlra_dev_t *dev, uint32_t screen_idx,
			    uint8_t *pixel_data, uint32_t bytes,
			    struct ctlra_screen_zone_t *redraw_zone,
			    void *userdata)
{
	struct smpla_t *s = userdata;

	caira_surface_t *img = s->cairo_img;
	caira_t *cr = s->cairo_cr;

	caira_set_source_rgb(cr, 0.1, 0.1, 0.1);
	caira_rectangle(cr, 0, 0, 480, 272);
	caira_fill(cr);

	if(screen_idx == 0)
		screen_draw_sequencers(s, cr);
	else
		screen_draw_mixer(s, cr);

	int stride = caira_image_surface_get_stride(img);
	unsigned char *data = caira_image_surface_get_data(img);

	sr_Pixel *pixels = (sr_Pixel *)data;
	uint16_t *scn = (uint16_t *)pixel_data;
	for(int j = 0; j < 272; j++) {
		for(int i = 0; i < 480; i++) {
			sr_Pixel px = *pixels++;
			/* convert to BGR565 in byte-swapped LE */
			/* blue 5, green LSB 3 bits */
			uint16_t red = px.rgba.r;
			uint16_t green = px.rgba.g;
			uint16_t blue = px.rgba.b;

			/* mask and shift */
			uint16_t b = ((blue  >> 3) & 0x1f);
			uint16_t g = ((green >> 2) & 0x3f) << 5;
			uint16_t r = ((red   >> 3) & 0x1f) << 11;

			/* byte-swap and store */
			uint16_t tmp = (b | g | r);
			*scn = ((tmp & 0x00FF) << 8) | ((tmp & 0xFF00) >> 8);
			scn++;
		}
	}


	return 1;
}

void machine3_event_func(struct ctlra_dev_t* dev,
                     uint32_t num_events,
                     struct ctlra_event_t** events,
                     void *userdata)
{
	struct smpla_t *s = userdata;
	struct mm_t *mm = s->controller_data;
	uint8_t msg[3] = {0};
	int static_mute = 0;

	for(uint32_t i = 0; i < num_events; i++) {
		struct ctlra_event_t *e = events[i];
		int pr = 0;
		switch(e->type) {
		case CTLRA_EVENT_BUTTON:
			msg[0] = e->button.pressed ? 0x90 : 0x80;
			msg[1] = 60 + e->button.id;
			msg[2] = e->button.pressed ? 0x70 : 0;
			(void)msg;
			pr = e->button.pressed;
			switch(e->button.id) {
			case  5: mm->shift_pressed = pr; break;
			case 24: mm->mode = MODE_PATTERN; break;
			case 21: mm->mode = MODE_PADS; break;
			case 33: if(pr) static_mute = !static_mute; break;
			case 41: if(pr) mm->playing = 1; break;
			case 43: if(pr) mm->playing = 0; break;
			case 26:
				if(pr) {
					mm->mode_prev = mm->mode;
					mm->mode = MODE_GROUP;
				} else {
					mm->mode = mm->mode_prev;
				}
				break;
			default:
				printf("button %d\n", e->button.id);
				break;
			}
			//printf("button %d\n", e->button.id);
			break;

		case CTLRA_EVENT_ENCODER:
			break;

		case CTLRA_EVENT_SLIDER: {
			//printf("slider %f\n", e->slider.value);
			struct smpla_sample_vol_t d = {
				.sample_id = mm->pattern_pad_id,
				.vol = e->slider.value,
			};
			smpla_to_rt_write(s, smpla_sample_vol, &d, sizeof(d));
			} break;

		case CTLRA_EVENT_GRID: {
			pr = e->grid.pressed;
			int p = ((3 - (e->grid.pos / 4)) * 4) + (e->grid.pos % 4);
			//printf("pad %d, mode = %d\n", p, mm->mode);
			mm->pads_pressed[p] = pr;
			if(mm->mode == MODE_GROUP) {
				/* select new group here */
				if(e->grid.pressed && p < 8) {
					mm->grp_id = p;
					//printf("new group %d\n", p);
				}
			} else if(mm->mode == MODE_PADS && pr) {
				/* update the selected pad */
				mm->pattern_pad_id = p;
				/* trigger a sample now */
				struct smpla_sample_state_t d = {
					.pad_id = 0,
					/* actions:
					 * 0) stop (also stops recording)
					 * 1) start playback from frame_start, to frame_end
					 * 2) record:
					 */
					.action = 1,
					.sample_id = p,
					.frame_start = 0,
					.frame_end = -1,
				};
				smpla_to_rt_write(s, smpla_sample_state,
						  &d, sizeof(d));
			} else if(mm->mode == MODE_PATTERN && pr) {
				//printf("pattern pr\n");
				if(mm->shift_pressed) {
					mm->pattern_pad_id = p;
				} else {
					sequencer_toggle_step(s->sequencers[mm->pattern_pad_id], p);
				}
			}


			} break;

#if 0
			pr = e->grid.pressed;
			if(mm->mode == MODE_PATTERN && pr) {
				if(mm->shift_pressed) {
					mm->pattern_pad_id = e->grid.pos;
					//printf("new pattern pad id %d\n", mm->pattern_pad_id);
				}
				else
					sequencer_toggle_step(sequencer, e->grid.pos);
			}
			mm->pads_pressed[e->grid.pos] = e->grid.pressed;
			break;
#endif

		default:
			break;
		};
	}

	ctlra_dev_light_flush(dev, 0);
}

void machine3_remove_func(struct ctlra_dev_t *dev, int unexpected_removal,
		     void *userdata)
{
}

int accept_dev_func(struct ctlra_t *ctlra,
		    const struct ctlra_dev_info_t *info,
		    struct ctlra_dev_t *dev,
                    void *userdata)
{
	ctlra_dev_set_event_func(dev, machine3_event_func);
	ctlra_dev_set_feedback_func(dev, machine3_feedback_func);
	ctlra_dev_set_screen_feedback_func(dev, machine3_screen_redraw_func);
	ctlra_dev_set_remove_func(dev, machine3_remove_func);
	ctlra_dev_set_callback_userdata(dev, userdata);

	/* mk3 */
	if(info->device_id == 0x1600) {
	}

	printf("smpla: accepting %s %s\n", info->vendor, info->device);
	return 1;
}

void seqEventCb(int frame, int note, int velocity, void* userdata )
{
	//printf("%s: frame %d, note %d : vel %d\n", __func__, frame, note, velocity);
	int static_mute = 0;
	if(static_mute)
		return;

	struct smpla_t *s = userdata;
	struct mm_t *mm = s->controller_data;
	memset(mm->pads_seq_play, 0, sizeof(mm->pads_seq_play));
	mm->pads_seq_play[note] = velocity > 0 ? velocity : 0;

	struct smpla_sample_state_t d = {
		.pad_id = 0,
		.action = 1,
		.sample_id = note,
		.frame_start = 0,
		.frame_end = -1,
	};
	smpla_sample_state(s, &d);
}
