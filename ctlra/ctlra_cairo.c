
#include <ctlra.h>
#include <string.h>

#include <cairo/cairo.h>

#include "config.h"
#include "impl.h"
#include "usb.h"

#include "immintrin.h"

#ifdef __SSE4_2__
// warning, type promo through args
static inline __attribute__((always_inline)) void
pixel_convert_from_argb_vec(uint32_t input_stride, uint8_t *in_888, uint8_t *out_565)
{
	/* Load 16B of pixels RGB 888 bits format */
	/* TODO: support ARGB? */
	__m128i v_in_px = _mm_loadu_si128((void *)in_888);

	/* Convert u8 rgb values into u16's for conversion
	 * to 565, which needs multiplies */
	const __m128i v_zeros = _mm_setzero_si128();
	const __m128i v_254_u16 = _mm_set1_epi16(254);

	__m128i v_col_to_u16;
	static const uint8_t col_to_u16_stride3[16] = {
		   0, 0xFF,    3, 0xFF,
		   6, 0xFF,    9, 0xFF,
		  12, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
	};
	static const uint8_t col_to_u16_stride4[16] = {
		   0, 0xFF,    4, 0xFF,
		   8, 0xFF,   12, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
	};
	if (input_stride == 3) {
		v_col_to_u16 = _mm_loadu_si128((void *)col_to_u16_stride3);
	} else if (input_stride == 4) {
		v_col_to_u16 = _mm_loadu_si128((void *)col_to_u16_stride4);
	} else {
		printf("programming error, cannot have this stride\n");
		return;
	}

	/* Red */
	__m128i v_u16_r = _mm_shuffle_epi8(v_in_px, v_col_to_u16);
	/* Green */
	v_in_px = _mm_alignr_epi8(v_in_px, v_in_px, 1);
	__m128i v_u16_g = _mm_shuffle_epi8(v_in_px, v_col_to_u16);
	/* Blue */
	v_in_px = _mm_alignr_epi8(v_in_px, v_in_px, 1);
	__m128i v_u16_b = _mm_shuffle_epi8(v_in_px, v_col_to_u16);

	/* Multiply the converted u16 values to 65k */
	__m128i v_u16_mul_r = _mm_mullo_epi16(v_u16_r, v_254_u16);
	__m128i v_u16_mul_g = _mm_mullo_epi16(v_u16_g, v_254_u16);
	__m128i v_u16_mul_b = _mm_mullo_epi16(v_u16_b, v_254_u16);

	static const uint16_t green_mask[8] = {
		((1 << 6) - 1) << 5,
		((1 << 6) - 1) << 5,
		((1 << 6) - 1) << 5,
		((1 << 6) - 1) << 5,
		((1 << 6) - 1) << 5,
		((1 << 6) - 1) << 5,
		((1 << 6) - 1) << 5,
		((1 << 6) - 1) << 5,
	};
	__m128i v_green_mask = _mm_loadu_si128((void *)green_mask);

	/* Shift u16 to right, clearing high 11 bits */
	__m128i v_u16_done_red = _mm_srli_epi16(v_u16_mul_r, 11);

	/* Shift Green to middle 6 bits, use mask to clear */
	__m128i v_u16_done_green = _mm_srli_epi16(v_u16_mul_g, 5);
	v_u16_done_green = _mm_and_si128(v_u16_done_green, v_green_mask);

	/* Blue: mask with ANDNOT green mask, leave in place */
	__m128i v_u16_done_blue = _mm_andnot_si128(v_green_mask, v_u16_mul_b);

	/* OR together the masked results */
	__m128i v_u16_done = _mm_or_si128(v_u16_done_green,
					v_u16_done_blue);
	v_u16_done = _mm_or_si128(v_u16_done, v_u16_done_red);

	/* Byteswap each pixel */
	static const uint8_t byteswap[16] = {
		1, 0, 3, 2,
		5, 4, 7, 6,
		9, 8, 11, 10,
		13, 12, 15, 14,
	};
	__m128i v_byteswap_mask = _mm_loadu_si128((void *)byteswap);
	__m128i v_u16_byteswap = _mm_shuffle_epi8(v_u16_done, v_byteswap_mask);

	_mm_storeu_si128((void*)out_565, v_u16_byteswap);
}
#endif

void pixel_convert_from_argb(int r, int g, int b, uint8_t *data)
{
	uint16_t r_ = (((uint16_t)r) * 254) >> 11;
	uint16_t g_ = ((((uint16_t)g) * 254) >> 5) & (((1 << 6)-1) << 5);
	uint16_t b_ = (((uint16_t)b) * 254) & (((1 << 5)-1) << 11);
	uint16_t rgb565 = b_ | g_ | r_;
	uint16_t *out = (uint16_t *)data;
	*out = (rgb565 << 8) | (rgb565 >> 8);
}

void
ctlra_screen_cairo_888_to_dev(uint8_t *device_data, uint32_t device_bytes,
			      uint8_t *input_data, uint32_t width,
			      uint32_t height, uint32_t input_stride);

// Scalar Today: 44 cycles on ICL
__attribute__((noinline)) void
ctlra_screen_cairo_888_to_dev(uint8_t *device_data, uint32_t device_bytes,
			      uint8_t *input_data, uint32_t width,
			      uint32_t height, uint32_t input_stride)
{
	uint16_t *write_head = (uint16_t*)device_data;

#if 0 /* #ifdef __SSE4_2__ */
	int stride = input_stride / width;

	if(stride == 4) {
		printf("input stride %d, per px stride %d == 4?\n", input_stride, stride);
		for(int j = 0; j < height - 1; j++) {
			for(int i = 0; i < width; i++) {
				uint8_t *p = &input_data[(j * input_stride) + (i*4)];
				int idx = (j * width) + (i);
				pixel_convert_from_argb_vec(stride, &p[0],
							(uint8_t*)&write_head[idx]);
			}
		}
	} else {
		for(int j = 0; j < height; j++) {
			for(int i = 0; i < width - 16; i++) {
				uint8_t *p = &input_data[(j * input_stride) + (i*4)];
				int idx = (j * width) + (i);
				pixel_convert_from_argb_vec(stride, &p[0],
							(uint8_t*)&write_head[idx]);
			}
		}
	}

#else
	/* Copy the Cairo pixels to the usb buffer, taking the
	 * stride of the cairo memory into account, converting from
	 * RGB into the BGR that the screen expects */
	for(int j = 0; j < height; j++) {
		for(int i = 0; i < width; i++) {
			uint8_t *p = &input_data[(j * input_stride) + (i*4)];
			int idx = (j * width) + (i);
			pixel_convert_from_argb(p[2], p[1], p[0],
						(uint8_t*)&write_head[idx]);
		}
	}
#endif
}

static inline void
ctlra_screen_cairo_565_to_dev(uint8_t *device_data, uint32_t device_bytes,
			      uint8_t *input_data, uint32_t width,
			      uint32_t height, uint32_t input_stride)
{
	/* Mushing from rgb 565 -> bgr 565 */
	uint16_t *rgb565 = (uint16_t *)input_data;
	uint16_t *pixels_565 = (uint16_t *)device_data;

/* Use Vector implementation */
#ifdef __AVX512F__
	/* AVX-512 version:
	 *  - Same steps as SSE: one load, two shifts, one or, store
	 *  - Manually unrolled 4x to diminish loop overhead
	 */
	for(int i = 0; i < (480 * 272); i += 128) {
		__m512i input1 = _mm512_loadu_si512((__m512i *)&rgb565[i+ 0]);
		__m512i input2 = _mm512_loadu_si512((__m512i *)&rgb565[i+32]);
		__m512i input3 = _mm512_loadu_si512((__m512i *)&rgb565[i+64]);
		__m512i input4 = _mm512_loadu_si512((__m512i *)&rgb565[i+96]);

		__m512i high1 = _mm512_slli_epi16(input1, 8);
		__m512i low1  = _mm512_srli_epi16(input1, 8);
		__m512i blend1 = _mm512_or_si512(high1, low1);
		_mm512_storeu_si512((__m512i *)&pixels_565[i], blend1);

		__m512i high2 = _mm512_slli_epi16(input2, 8);
		__m512i low2  = _mm512_srli_epi16(input2, 8);
		__m512i blend2 = _mm512_or_si512(high2, low2);
		_mm512_storeu_si512((__m512i *)&pixels_565[i+32], blend2);

		__m512i high3 = _mm512_slli_epi16(input3, 8);
		__m512i low3  = _mm512_srli_epi16(input3, 8);
		__m512i blend3 = _mm512_or_si512(high3, low3);
		_mm512_storeu_si512((__m512i *)&pixels_565[i+64], blend3);

		__m512i high4 = _mm512_slli_epi16(input4, 8);
		__m512i low4  = _mm512_srli_epi16(input4, 8);
		__m512i blend4 = _mm512_or_si512(high4, low4);
		_mm512_storeu_si512((__m512i *)&pixels_565[i+96], blend4);
	}
#elif __AVX2__
	/* AVX2 version:
	 *  - See notes on SSE below
	 *  - Avx2 has 256 bit registers, so 2X as effective
	 *  - Same steps as SSE: one load, two shifts, one or, store
	 *  - Manually unrolled 4x to diminish loop overhead
	 */
	for(int i = 0; i < (480 * 272); i += 64) {
		__m256i input1 = _mm256_loadu_si256((__m256i *)&rgb565[i]);
		__m256i input2 = _mm256_loadu_si256((__m256i *)&rgb565[i+16]);
		__m256i input3 = _mm256_loadu_si256((__m256i *)&rgb565[i+32]);
		__m256i input4 = _mm256_loadu_si256((__m256i *)&rgb565[i+48]);

		__m256i high1 = _mm256_slli_epi16(input1, 8);
		__m256i low1  = _mm256_srli_epi16(input1, 8);
		__m256i blend1 = _mm256_or_si256(high1, low1);
		_mm256_storeu_si256((__m256i *)&pixels_565[i], blend1);

		__m256i high2 = _mm256_slli_epi16(input2, 8);
		__m256i low2  = _mm256_srli_epi16(input2, 8);
		__m256i blend2 = _mm256_or_si256(high2, low2);
		_mm256_storeu_si256((__m256i *)&pixels_565[i+16], blend2);

		__m256i high3 = _mm256_slli_epi16(input3, 8);
		__m256i low3  = _mm256_srli_epi16(input3, 8);
		__m256i blend3 = _mm256_or_si256(high3, low3);
		_mm256_storeu_si256((__m256i *)&pixels_565[i+32], blend3);

		__m256i high4 = _mm256_slli_epi16(input4, 8);
		__m256i low4  = _mm256_srli_epi16(input4, 8);
		__m256i blend4 = _mm256_or_si256(high4, low4);
		_mm256_storeu_si256((__m256i *)&pixels_565[i+48], blend4);
	}
#elif __SSE2__
	/* SSE version:
	 *  - 8 px per SIMD register (uint16_t per pixel)
	 *  - One load, two shifts, one or, store
	 *  - Manually unrolled 4x to diminish loop overhead
	 *  - All loads at start make compilers emit code that uses
	 *    multiple vector registers: xmm0, xmm1, xmm2, xmm3
	 */
	for(int i = 0; i < (width * height); i += 32) {
		__m128i input1 = _mm_loadu_si128((__m128i*)&rgb565[i]);
		__m128i input2 = _mm_loadu_si128((__m128i*)&rgb565[i+8]);
		__m128i input3 = _mm_loadu_si128((__m128i*)&rgb565[i+16]);
		__m128i input4 = _mm_loadu_si128((__m128i*)&rgb565[i+24]);

		__m128i high1 = _mm_slli_epi16(input1, 8);
		__m128i low1  = _mm_srli_epi16(input1, 8);
		__m128i blend1 = _mm_or_si128(high1, low1);
		_mm_storeu_si128((__m128i *)&pixels_565[i], blend1);

		__m128i high2 = _mm_slli_epi16(input2, 8);
		__m128i low2  = _mm_srli_epi16(input2, 8);
		__m128i blend2 = _mm_or_si128(high2, low2);
		_mm_storeu_si128((__m128i *)&pixels_565[i+8], blend2);

		__m128i high3 = _mm_slli_epi16(input3, 8);
		__m128i low3  = _mm_srli_epi16(input3, 8);
		__m128i blend3 = _mm_or_si128(high3, low3);
		_mm_storeu_si128((__m128i *)&pixels_565[i+16], blend3);

		__m128i high4 = _mm_slli_epi16(input4, 8);
		__m128i low4  = _mm_srli_epi16(input4, 8);
		__m128i blend4 = _mm_or_si128(high4, low4);
		_mm_storeu_si128((__m128i *)&pixels_565[i+24], blend4);
	}
	return;
#else
	for(int i = 0; i < (480 * 272); i++) {
		pixels_565[i] = (rgb565[i] << 8) | (rgb565[i] >> 8);
	}
	return;

#endif
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

	/* TODO: Move to device function pointer implementation  */
	cairo_format_t format = cairo_image_surface_get_format(surf);
	switch(format) {
	case CAIRO_FORMAT_ARGB32: /* 24 bytes of RGB at lower bits */
	case CAIRO_FORMAT_RGB24:  /* 24 bytes of RGB at lower bits */
		/* convert 24 byte RGB to destination */
		ctlra_screen_cairo_888_to_dev(pixel_data, bytes,
					      data, width, height,
					      stride);
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

	return 0;
}
