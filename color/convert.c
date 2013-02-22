/*
 * Modified from stagefright colorspace conversion.
 * - <zhix.wang@intel.com>
 */

#include <stdio.h>
#include <stdint.h>

static uint8_t *initClip(void)
{
	static const signed kClipMin = -278;
	static const signed kClipMax = 535;
	static uint8_t mClip[535 + 278 + 1];

	signed i;

	for (i = kClipMin; i <= kClipMax; ++i) {
		mClip[i - kClipMin] = (i < 0) ? 0 : (i > 255) ? 255 : (uint8_t)i;
	}

	return &mClip[-kClipMin];
}

int nv12_to_rgb888(unsigned int src_w, unsigned int src_h, 
		unsigned int y_stride, unsigned int uv_stride,
		unsigned char *src_y, unsigned char *src_uv, uint32_t *dst)
{
	uint8_t *kAdjustedClip = initClip();
	size_t x, y;

	for (y = 0; y < src_h; ++y) {
		for (x = 0; x < src_w; x += 2) {
			signed y1 = (signed)src_y[x] - 16;
			signed y2 = (signed)src_y[x + 1] - 16;

			signed v = (signed)src_uv[x & ~1] - 128;
			signed u = (signed)src_uv[(x & ~1) + 1] - 128;

			signed u_b = u * 517;
			signed u_g = -u * 100;
			signed v_g = -v * 208;
			signed v_r = v * 409;
			signed tmp1 = y1 * 298;
			signed b1 = (tmp1 + u_b) / 256;
			signed g1 = (tmp1 + v_g + u_g) / 256;
			signed r1 = (tmp1 + v_r) / 256;

			signed tmp2 = y2 * 298;
			signed b2 = (tmp2 + u_b) / 256;
			signed g2 = (tmp2 + v_g + u_g) / 256;
			signed r2 = (tmp2 + v_r) / 256;

			uint32_t rgb1 =
				(kAdjustedClip[b1] << 16)
				| (kAdjustedClip[g1] << 8)
				| (kAdjustedClip[r1]);

			uint32_t rgb2 =
				(kAdjustedClip[b2] << 16)
				| (kAdjustedClip[g2] << 8)
				| (kAdjustedClip[r2]);

			dst[x] = rgb1;
			dst[x + 1] = rgb2;
		}

		src_y += y_stride;

		if (y & 1) {
			src_uv += uv_stride;
		}

		dst += src_w;
	}

	return 0;
}

int yuv420p_to_rgb888(unsigned int src_w, unsigned int src_h, 
		unsigned int y_stride, unsigned int u_stride, unsigned int v_stride,
		unsigned char *src_y, unsigned char *src_u, unsigned char *src_v, 
		uint32_t *dst)
{
	unsigned int x, y;
	uint8_t *kAdjustedClip = initClip();

	for (y = 0; y < src_h; ++y) {
		for (x = 0; x < src_w; x += 2) {
			// B = 1.164 * (Y - 16) + 2.018 * (U - 128)
			// G = 1.164 * (Y - 16) - 0.813 * (V - 128) - 0.391 * (U - 128)
			// R = 1.164 * (Y - 16) + 1.596 * (V - 128)

			// B = 298/256 * (Y - 16) + 517/256 * (U - 128)
			// G = .................. - 208/256 * (V - 128) - 100/256 * (U - 128)
			// R = .................. + 409/256 * (V - 128)

			// min_B = (298 * (- 16) + 517 * (- 128)) / 256 = -277
			// min_G = (298 * (- 16) - 208 * (255 - 128) - 100 * (255 - 128)) / 256 = -172
			// min_R = (298 * (- 16) + 409 * (- 128)) / 256 = -223

			// max_B = (298 * (255 - 16) + 517 * (255 - 128)) / 256 = 534
			// max_G = (298 * (255 - 16) - 208 * (- 128) - 100 * (- 128)) / 256 = 432
			// max_R = (298 * (255 - 16) + 409 * (255 - 128)) / 256 = 481

			// clip range -278 .. 535

			signed y1 = (signed)src_y[x] - 16;
			signed y2 = (signed)src_y[x + 1] - 16;

			signed u = (signed)src_u[x / 2] - 128;
			signed v = (signed)src_v[x / 2] - 128;

			signed u_b = u * 517;
			signed u_g = -u * 100;
			signed v_g = -v * 208;
			signed v_r = v * 409;

			signed tmp1 = y1 * 298;
			signed b1 = (tmp1 + u_b) / 256;
			signed g1 = (tmp1 + v_g + u_g) / 256;
			signed r1 = (tmp1 + v_r) / 256;

			signed tmp2 = y2 * 298;
			signed b2 = (tmp2 + u_b) / 256;
			signed g2 = (tmp2 + v_g + u_g) / 256;
			signed r2 = (tmp2 + v_r) / 256;

			uint32_t rgb1 =
				(kAdjustedClip[r1] << 16)
				| (kAdjustedClip[g1] << 8)
				| (kAdjustedClip[b1]);

			uint32_t rgb2 =
				(kAdjustedClip[r2] << 16)
				| (kAdjustedClip[g2] << 8)
				| (kAdjustedClip[b2]);

			dst[x] = rgb1;
			dst[x + 1] = rgb2;
		}

		src_y += y_stride;

		if (y & 1) {
			src_u += u_stride;
			src_v += v_stride;
		}

		dst += src_w;
	}

	return 0;
}
