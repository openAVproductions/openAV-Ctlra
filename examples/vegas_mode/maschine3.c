#include "global.h"

#include <stdlib.h>

#include "ctlra.h"

#include "emmintrin.h"

static int chan;
const char *patch_name;

/* for drawing graphics to screen */
#include <cairo/cairo.h>

static cairo_surface_t *surface = 0;
static cairo_t *cr = 0;

#define WIDTH  480
#define HEIGHT 272

#define SCALAR_ARGB32 1

void maschine3_screen_init()
{
	if(surface)
		return;

#ifdef SCALAR_ARGB32
	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
					      WIDTH, HEIGHT);
#else
	surface = cairo_image_surface_create (CAIRO_FORMAT_RGB16_565,
					      WIDTH, HEIGHT);
#endif
	if(!surface)
		printf("!surface\n");
	cr = cairo_create (surface);
	if(!cr)
		printf("!cr\n");
}

static inline
void pixel_convert_from_argb(int r, int g, int b, uint8_t *data)
{
	r = ((int)((r / 255.0) * 31)) & ((1<<5)-1);
	g = ((int)((g / 255.0) * 63)) & ((1<<6)-1);
	b = ((int)((b / 255.0) * 31)) & ((1<<5)-1);

	uint16_t combined = (b | g << 5 | r << 11);
	data[0] = combined >> 8;
	data[1] = combined & 0xff;
}

struct maschine3_t
{
	/* changes between note-ons and other functions */
	int mode;
	uint8_t pads[16];

	uint8_t file_selected;


	/* item browser cache */
	uint8_t items_init;
#define NUM_ITEMS 8
	cairo_surface_t *item_surfaces[NUM_ITEMS];

#define NUM_ENCODERS 8
	float encoder_value[NUM_ENCODERS];
};

static
void maschin3_item_browser(struct dummy_data *d,
			   cairo_t *cr,
			   const char *path,
			   uint32_t screen_idx)
{
	struct maschine3_t *m = d->maschine3;

	if(!m->items_init) {
		char filename[64];
		for(int i = 0; i < 4; i++) {
			snprintf(filename, 64, "item_%d.png", i + 1);
			m->item_surfaces[i] =
				cairo_image_surface_create_from_png(filename);
			printf("item load %d: ciaro surface status : %s\n", i,
			       cairo_status_to_string(cairo_surface_status(m->item_surfaces[i])));
		}
		/*
		cairo_surface_flush(st->surface);
		cairo_surface_destroy(surf);
		*/
		m->items_init = 1;
	}

	cairo_save(cr);
	cairo_scale(cr, 0.35, 0.35);
	for(int i = 0; i < 4; i++) {
		cairo_save(cr);
		int sel = m->file_selected + (4 * (screen_idx == 1));
		float off = ((sel % 8) == i) ? 0 : 1;
		//cairo_scale(cr, off, off);
		cairo_translate(cr, 100 * i, 150 + off * 100);
		cairo_set_source_surface(cr, m->item_surfaces[i],
					 i * 220, 0);
		cairo_paint(cr);
		cairo_restore(cr);
	}
	cairo_restore(cr);
}

static
void maschin3_file_browser(struct dummy_data *d,
			   cairo_t *cr,
			   const char *path)
{
	struct maschine3_t *m = d->maschine3;

	const uint8_t NUM_FILES = 12;
	/* contains filenames */
	char filenames[NUM_FILES*64];
	/* Table of pointers to filenames */
	char *files[NUM_FILES];
	for(int i = 0; i < NUM_FILES; i++) {
		files[i] = &filenames[i*64];
		snprintf(files[i], 64, "file number %d", i);
	}
	//int nfiles = djmxa_get_file_list(djmxa, "tunes/", files, NUM_FILES);
	int nfiles = 3;

	/* blank background */
	cairo_set_source_rgb(cr, 8/255.,8/255.,8/255.);
	cairo_rectangle(cr, 0, 0, 480, 272);
	cairo_fill(cr);

	cairo_set_source_rgb(cr, 12/255.,12/255.,12/255.);
	cairo_rectangle(cr, 1, 1, 478, 18);
	cairo_fill(cr);

	int selected = m->file_selected % nfiles;

	cairo_move_to(cr, 9, 15 );
	cairo_set_source_rgba(cr, 1,1,1, 0.8);
	cairo_show_text(cr, "Load File...");

	for(int i = 0; i < NUM_FILES; i++) {
		/* Draw each file */
		if(i == selected && i < nfiles)
			cairo_set_source_rgb(cr, 60/255., 60/255.,255/255.);
		else if(i & 1)
			cairo_set_source_rgb(cr, 18/255.,18/255.,18/255.);
		else
			cairo_set_source_rgb(cr, 28/255.,28/255.,28/255.);
		cairo_rectangle(cr, 5, 20 + (25 * i), 450, 25);
		cairo_fill(cr);
	}

	for(int i = 0; i < nfiles; i++) {
		cairo_move_to(cr, 9, 20 + 25*i + 18);
		cairo_set_source_rgba(cr, 1,1,1, 0.8);
		cairo_show_text(cr, files[i]);
	}
}

int draw_stuff(struct dummy_data *d, uint32_t screen_idx)
{
	//struct maschine3_t *m = d->maschine3;
	static uint64_t rev;
	if(d->revision == rev) {
		return 0;
	}

	rev = d->revision;

	/* blank background */
	cairo_set_source_rgb(cr, 2/255., 2/255., 2/255.);
	//cairo_set_source_rgb(cr, 0/255., 0/255., 8/255.);
	cairo_rectangle(cr, 0, 0, 480, 272);
	cairo_fill(cr);

#if 0
	/* draw on surface */
	cairo_set_source_rgb(cr, 1, 0, 0);
	cairo_rectangle(cr, 0, 0, 4, 4);
	cairo_fill(cr);

	cairo_set_source_rgb(cr, 0, 1, 0);
	cairo_rectangle(cr, 4, 0, 4, 4);
	cairo_fill(cr);

	cairo_set_source_rgb(cr, 0, 0, 1);
	cairo_rectangle(cr, 8, 0, 4, 4);
	cairo_fill(cr);

	cairo_set_source_rgb(cr, 2/255., 2/255., 2/255.);
	cairo_rectangle(cr, 0, 252, 480, 2);
	cairo_fill(cr);

	for(int i = 0; i < 16; i++) {
		cairo_set_source_rgb(cr, 0, 71, 1);
		cairo_rectangle(cr, 10 + 22 * i, 20, 20, 20);
		if(m->pads[i])
			cairo_fill(cr);
		else
			cairo_stroke(cr);
	}
	cairo_stroke(cr);
#endif

	if(0) maschin3_file_browser(d, cr, "test");
	if(1) maschin3_item_browser(d, cr, "blah", screen_idx);

	cairo_surface_flush(surface);

	/* redraw */
	return 1;
}

static inline uint64_t rdtsc(void)
{
	union {
		uint64_t tsc_64;
		struct {
			uint32_t lo_32;
			uint32_t hi_32;
		};
	} tsc;

	__asm__ volatile("rdtscp" :
		     "=a" (tsc.lo_32),
		     "=d" (tsc.hi_32));
	return tsc.tsc_64;
}

void convert_scalar(unsigned char *data, uint8_t *pixels, uint32_t stride)
{
	uint16_t *write_head = (uint16_t*)pixels;
	/* Copy the Cairo pixels to the usb buffer, taking the
	 * stride of the cairo memory into account, converting from
	 * RGB into the BGR that the screen expects */
	for(int j = 0; j < HEIGHT; j++) {
		for(int i = 0; i < WIDTH; i++) {
			uint8_t *p = &data[(j * stride) + (i*4)];
			int idx = (j * WIDTH) + (i);
			pixel_convert_from_argb(p[2], p[1], p[0],
						(uint8_t*)&write_head[idx]);
		}
	}
}

#define PRINT_U16(x) \
	for(int i = 0; i < 8; i++) printf("%04x ", ((uint16_t *)&x)[i]); \
	printf("\n")


void maschine3_update_state(struct ctlra_dev_t *dev, void *ud)
{
	struct dummy_data *d = ud;
	struct maschine3_t *m = d->maschine3;

	int i;
	for(i = 0; i < VEGAS_BTN_COUNT; i++)
		ctlra_dev_light_set(dev, i, 0);

	if(!m) {
		ctlra_dev_light_set(dev, 22, UINT32_MAX);
		return;
	}

	switch(m->mode) {
	case 0:
		ctlra_dev_light_set(dev, 22, UINT32_MAX);
		ctlra_dev_light_flush(dev, 0);
		break;
	default:
		ctlra_dev_light_set(dev, 22, 0);
		for(i = 0; i < VEGAS_BTN_COUNT; i++)
			ctlra_dev_light_set(dev, i, UINT32_MAX * d->buttons[i]);
		break;
	}
}

int maschine3_redraw_screen(uint32_t screen_idx, uint8_t *pixels,
			     uint32_t bytes, void *ud)
{
	struct dummy_data *d = ud;

	int flush;

	{
		uint64_t start = rdtsc();
		flush = draw_stuff(d, screen_idx);
		uint64_t end = rdtsc();
		if(0) printf("draw = %ld\n", end - start);
	}

	if(flush == 0)
		return 0;

	unsigned char * data = cairo_image_surface_get_data(surface);
	if(!data) {
		printf("error data == 0\n");
		return -1;
	}

	uint64_t start = rdtsc();
	int stride = cairo_image_surface_get_stride(surface);
	(void)stride;
#ifdef SCALAR_ARGB32
	convert_scalar(data, pixels, stride);
#else
	//convert_sse4(data, pixels, stride);
	/* rgb 565 -> bgr 565 */
	uint16_t *rgb565 = (uint16_t *)cairo_image_surface_get_data(surface);
	uint16_t *pixels_565 = (uint16_t *)pixels;
	__m128i red_mask;
	__m128i green_mask;
	__m128i blue_mask;
	uint16_t *rm = (uint16_t *)&red_mask;
	uint16_t *gm = (uint16_t *)&green_mask;
	uint16_t *bm = (uint16_t *)&blue_mask;

	for(int i = 0; i < 8; i++) {
		rm[i] = (0b11111000 << 8);
		gm[i] = (0b00000111 << 8) | 0b11100000;
		bm[i] =  0b00011111;
	}

	for(int i = 0; i < (HEIGHT * WIDTH); i += 8) {
		__m128i input = _mm_loadu_si128((__m128i*)&rgb565[i]);

		__m128i red   = _mm_and_si128(input, red_mask);
		__m128i blue  = _mm_and_si128(input, blue_mask);
		__m128i green = _mm_and_si128(input, green_mask);

		__m128i red_shift_amt = _mm_set1_epi64x(6);
		__m128i red_shifted = _mm_srl_epi16(red, red_shift_amt);

		__m128i green_shift_amt = _mm_set1_epi64x(5);
		__m128i green_shifted = _mm_srl_epi16(green, green_shift_amt);

		__m128i finished = _mm_or_si128(green_shifted, red_shifted);

		__m128i blue_shift_amt = _mm_set1_epi64x(11);
		__m128i blue_shifted = _mm_sll_epi16(blue, blue_shift_amt);

		finished = _mm_or_si128(blue_shifted, finished);
		_mm_storeu_si128((__m128i *)&pixels_565[i], finished);
	}

#endif
	uint64_t end = rdtsc();
	if(0) printf("convert = %ld\n", end - start);

	return flush;
}

void maschine3_pads(struct ctlra_dev_t* dev,
	     uint32_t num_events,
	     struct ctlra_event_t** events,
	     void *userdata)
{
	struct dummy_data *dummy = (void *)userdata;
	struct maschine3_t *m = dummy->maschine3;
	(void)chan;

	for(uint32_t i = 0; i < num_events; i++) {
		struct ctlra_event_t *e = events[i];
		switch(e->type) {
		case CTLRA_EVENT_BUTTON:
			ctlra_dev_light_set(dev, e->button.id, UINT32_MAX);
			break;
		case CTLRA_EVENT_ENCODER:
			//printf("e %d, %f\n", e->encoder.id, e->encoder.delta_float);
			if(e->encoder.id == 0)
				m->file_selected += e->encoder.delta;
			break;
		case CTLRA_EVENT_GRID:
			if(e->grid.flags & CTLRA_EVENT_GRID_FLAG_BUTTON) {
				dummy->buttons[e->grid.pos] = e->grid.pressed;
				if(e->grid.pressed)
					soffa_note_on(dummy->soffa, 0, 36 + e->grid.pos,
						      0.3 + 0.7 * e->grid.pressure);
				else
					soffa_note_off(dummy->soffa, 0, 36 + e->grid.pos);
				m->pads[e->grid.pos] = e->grid.pressed;
			}
			break;
		default:
			break;
		};
		dummy->revision++;
	}
}

int maschine3_screen_redraw_cb(struct ctlra_dev_t *dev,
				uint32_t screen_idx,
				uint8_t *pixel_data,
				uint32_t bytes,
				void *userdata)
{
	return maschine3_redraw_screen(screen_idx, pixel_data, bytes, userdata);
}

void maschine3_func(struct ctlra_dev_t* dev,
	     uint32_t num_events,
	     struct ctlra_event_t** events,
	     void *userdata)
{
	struct dummy_data *dummy = (void *)userdata;

	if(!dummy->maschine3) {
		dummy->maschine3 = calloc(1, sizeof(struct maschine3_t));
		if(!dummy->maschine3) {
			printf("failed to allocate maschine3 struct\n");
			exit(-1);
		}

		int32_t ret = ctlra_dev_screen_register_callback(dev,
								 30,
						 maschine3_screen_redraw_cb,
								 dummy);
		if(ret)
			printf("WARNING: Failed to register screen callback\n");
	}

	maschine3_screen_init();

	struct maschine3_t *m = dummy->maschine3;
	if(m->mode == 0) {
		maschine3_pads(dev, num_events, events, userdata);
	}
}

