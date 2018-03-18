
#include <ctlra.h>
#include <string.h>

#include <cairo/cairo.h>

#include "config.h"
#include "impl.h"
#include "usb.h"

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

static void
ctlra_screen_cairo_565_to_dev(uint8_t *device_data, uint32_t device_bytes,
			      uint8_t *input_data, uint32_t width,
			      uint32_t height, uint32_t input_stride)
{
	/* Mush from RGB 565 to BGR 565 here */
	printf("in %s\n", __func__);
}

int
ctlra_screen_cairo_to_device(struct ctlra_dev_t *dev, uint32_t screen_idx,
			     uint8_t *pixel_data, uint32_t bytes,
			     struct ctlra_screen_zone_t *redraw_zone,
			     void *surf)
{
	/* TODO: optimize this to ctlra_create() and cache value */
	if(!strlen(CTLRA_OPT_CAIRO)) {
		CTLRA_WARN(dev->ctlra_context, "Cairo screen support: %d", 0);
		return -1;
	}

	if(CAIRO_SURFACE_TYPE_IMAGE != cairo_surface_get_type(surf)) {
		CTLRA_WARN(dev->ctlra_context,
			   "Cairo surface is not image surface, but reports %d\n",
			   cairo_surface_get_type(surf));
		return -1;
	}

	/* Calculate stride / pixel copy */
	int width = cairo_image_surface_get_width (surf);
	int height = cairo_image_surface_get_height (surf);
	int stride = cairo_image_surface_get_stride(surf);
	unsigned char * data = cairo_image_surface_get_data(surf);
	if(!data) {
		printf("error data == 0\n");
		return -2;
	}

	cairo_surface_flush(surf);

	cairo_format_t format = cairo_image_surface_get_format(surf);
	switch(format) {
	case CAIRO_FORMAT_ARGB32: /* 24 bytes of RGB at lower bits */
	case CAIRO_FORMAT_RGB24:  /* 24 bytes of RGB at lower bits */
		/* convert 24 byte RGB to destination */
		break;
	case CAIRO_FORMAT_RGB16_565:
		/* re-mush the RGB into BGR order */
		ctlra_screen_cairo_565_to_dev(pixel_data, bytes,
					      data, width, height,
					      stride);
		return 0;
	default:
		return -3;
	}

	/* TODO: Move to device function pointer implementation  */
	uint16_t *write_head = (uint16_t*)pixel_data;
	/* Copy the Cairo pixels to the usb buffer, taking the
	 * stride of the cairo memory into account, converting from
	 * RGB into the BGR that the screen expects */
	for(int j = 0; j < height; j++) {
		for(int i = 0; i < width; i++) {
			uint8_t *p = &data[(j * stride) + (i*4)];
			int idx = (j * width) + (i);
			pixel_convert_from_argb(p[2], p[1], p[0],
						(uint8_t*)&write_head[idx]);
		}
	}

	return 0;
}
