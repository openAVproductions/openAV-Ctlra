/* Public header that exposes the Cairo -> device conversion function.
 */
#ifndef CTLRA_CAIRO
#define CTLRA_CAIRO

#include "ctlra.h"

#ifdef __cplusplus
extern "C" {
#endif

int ctlra_screen_cairo_to_device(struct ctlra_dev_t *dev,
				 uint32_t screen_idx,
				 uint8_t *pixel_data,
				 uint32_t bytes,
				 struct ctlra_screen_zone_t *redraw_zone,
				 void *cairo_image_surface);

#ifdef __cplusplus
}
#endif

#endif
