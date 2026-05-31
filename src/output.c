#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <cairo/cairo.h>
#include <gd.h>
#include <webp/encode.h>
#include <webp/mux.h>
#include <zlib.h>

#include "output.h"
#include "render.h"
#include "easing.h"
#include "t2g.h"

/* ═══════════════════════════════════════════════════════════════════════
   Cairo surface → GD TrueColor image
   ═══════════════════════════════════════════════════════════════════════ */

static gdImagePtr surface_to_gd(cairo_surface_t *surface)
{
	cairo_surface_flush(surface);
	int w      = cairo_image_surface_get_width(surface);
	int h      = cairo_image_surface_get_height(surface);
	int stride = cairo_image_surface_get_stride(surface);
	unsigned char *data = cairo_image_surface_get_data(surface);

	gdImagePtr im = gdImageCreateTrueColor(w, h);
	gdImageAlphaBlending(im, 0);
	gdImageSaveAlpha(im, 1);

	for (int y = 0; y < h; y++) {
		uint32_t *row = (uint32_t *)(data + y * stride);
		for (int x = 0; x < w; x++) {
			uint32_t px = row[x];
			int a = (px >> 24) & 0xff;
			int r = (px >> 16) & 0xff;
			int g = (px >>  8) & 0xff;
			int b =  px        & 0xff;
			/* Un-premultiply Cairo's premultiplied alpha */
			if (a > 0 && a < 255) {
				r = r * 255 / a;
				g = g * 255 / a;
				b = b * 255 / a;
			}
			/* GD alpha: 0 = opaque, 127 = transparent */
			int gd_a = (255 - a) >> 1;
			gdImageSetPixel(im, x, y, gdTrueColorAlpha(r, g, b, gd_a));
		}
	}
	return im;
}

/* ═══════════════════════════════════════════════════════════════════════
   GIF state
   ═══════════════════════════════════════════════════════════════════════ */

typedef struct {
	FILE       *f;
	gdImagePtr  first;   /* keep reference for gdImageGifAnimEnd */
	int         started;
} gif_state_t;

static void gif_add(gif_state_t *s, cairo_surface_t *surface, int delay_ms)
{
	gdImagePtr im = surface_to_gd(surface);
	/* Reduce to 256-colour palette with dithering for better quality */
	gdImageTrueColorToPalette(im, 1, 256);

	int delay_cs = delay_ms / 10;  /* GIF delay is in centiseconds */

	if (!s->started) {
		gdImageGifAnimBegin(im, s->f, 1, 0);
		s->first   = im;
		s->started = 1;
		gdImageGifAnimAdd(im, s->f, 1, 0, 0, delay_cs, 1, NULL);
	} else {
		gdImageGifAnimAdd(im, s->f, 1, 0, 0, delay_cs, 1, s->first);
		gdImageDestroy(im);
	}
}

static int gif_finish(gif_state_t *s)
{
	gdImageGifAnimEnd(s->f);
	fclose(s->f);
	if (s->first) gdImageDestroy(s->first);
	return 0;
}

/* ═══════════════════════════════════════════════════════════════════════
   WebP (animated) via libwebpmux WebPAnimEncoder
   ═══════════════════════════════════════════════════════════════════════ */

typedef struct {
	WebPAnimEncoder *enc;
	FILE            *f;
	int              timestamp_ms;
	int              width, height;
} webp_state_t;

static void webp_add(webp_state_t *s, cairo_surface_t *surface, int delay_ms)
{
	cairo_surface_flush(surface);
	int stride = cairo_image_surface_get_stride(surface);
	unsigned char *data = cairo_image_surface_get_data(surface);

	WebPPicture frame;
	WebPPictureInit(&frame);
	frame.width    = s->width;
	frame.height   = s->height;
	frame.use_argb = 1;
	WebPPictureAlloc(&frame);

	/* Cairo: BGRA in memory (little-endian ARGB32) → import as BGRA */
	WebPPictureImportBGRA(&frame, data, stride);

	WebPConfig config;
	WebPConfigInit(&config);
	config.lossless = 1;
	config.quality  = 100;

	WebPAnimEncoderAdd(s->enc, &frame, s->timestamp_ms, &config);
	WebPPictureFree(&frame);

	s->timestamp_ms += delay_ms;
}

static int webp_finish(webp_state_t *s)
{
	/* Flush with a sentinel (NULL picture) at final timestamp */
	WebPAnimEncoderAdd(s->enc, NULL, s->timestamp_ms, NULL);

	WebPData webp_data;
	WebPDataInit(&webp_data);
	if (!WebPAnimEncoderAssemble(s->enc, &webp_data)) {
		fprintf(stderr, "WebPAnimEncoderAssemble failed\n");
		WebPAnimEncoderDelete(s->enc);
		fclose(s->f);
		return 1;
	}
	WebPAnimEncoderDelete(s->enc);

	fwrite(webp_data.bytes, 1, webp_data.size, s->f);
	WebPDataClear(&webp_data);
	fclose(s->f);
	return 0;
}

/* ═══════════════════════════════════════════════════════════════════════
   APNG — minimal hand-written encoder (zlib + PNG chunks)
   ═══════════════════════════════════════════════════════════════════════ */

typedef struct {
	uint8_t **frames;
	size_t   *frame_sizes;
	int      *delays_ms;
	int       count;
	int       capacity;
	int       width, height;
	char     *filename;
} apng_state_t;

static void write_be32(uint8_t *p, uint32_t v)
{
	p[0] = (v >> 24) & 0xff;
	p[1] = (v >> 16) & 0xff;
	p[2] = (v >>  8) & 0xff;
	p[3] =  v        & 0xff;
}

static void write_be16(uint8_t *p, uint16_t v)
{
	p[0] = (v >> 8) & 0xff;
	p[1] =  v       & 0xff;
}

static void png_write_chunk(FILE *f, const char *type,
                             const uint8_t *data, uint32_t len)
{
	uint8_t hdr[4];
	write_be32(hdr, len);
	fwrite(hdr, 1, 4, f);
	fwrite(type, 1, 4, f);
	if (len && data) fwrite(data, 1, len, f);

	uLong crc = crc32(0, NULL, 0);
	crc = crc32(crc, (const Bytef *)type, 4);
	if (len && data) crc = crc32(crc, data, len);
	uint8_t crc_buf[4];
	write_be32(crc_buf, (uint32_t)crc);
	fwrite(crc_buf, 1, 4, f);
}

/* Compress one frame from Cairo ARGB32 → PNG-filtered RGBA → zlib */
static uint8_t *compress_surface(cairo_surface_t *surface,
                                  int w, int h, size_t *out_size)
{
	cairo_surface_flush(surface);
	int stride = cairo_image_surface_get_stride(surface);
	unsigned char *data = cairo_image_surface_get_data(surface);

	/* Filter byte (0 = None) + RGBA per pixel */
	size_t row_bytes   = 1 + (size_t)w * 4;
	size_t total_bytes = (size_t)h * row_bytes;
	uint8_t *filtered  = malloc(total_bytes);
	if (!filtered) return NULL;

	for (int y = 0; y < h; y++) {
		uint8_t *dst = filtered + y * row_bytes;
		dst[0] = 0;  /* filter = None */
		uint8_t *src = data + y * stride;
		for (int x = 0; x < w; x++) {
			/* Cairo ARGB32 (little-endian): [B][G][R][A] in memory */
			uint8_t b = src[x * 4 + 0];
			uint8_t g = src[x * 4 + 1];
			uint8_t r = src[x * 4 + 2];
			uint8_t a = src[x * 4 + 3];
			/* Un-premultiply */
			if (a > 0 && a < 255) {
				r = (uint8_t)((uint32_t)r * 255 / a);
				g = (uint8_t)((uint32_t)g * 255 / a);
				b = (uint8_t)((uint32_t)b * 255 / a);
			}
			dst[1 + x * 4 + 0] = r;
			dst[1 + x * 4 + 1] = g;
			dst[1 + x * 4 + 2] = b;
			dst[1 + x * 4 + 3] = a;
		}
	}

	uLong bound      = compressBound((uLong)total_bytes);
	uint8_t *compressed = malloc(bound);
	uLong compressed_sz = bound;
	compress2(compressed, &compressed_sz, filtered, (uLong)total_bytes,
	          Z_BEST_SPEED);
	free(filtered);

	*out_size = (size_t)compressed_sz;
	return compressed;
}

static void apng_add(apng_state_t *s, cairo_surface_t *surface, int delay_ms)
{
	if (s->count == s->capacity) {
		s->capacity = s->capacity ? s->capacity * 2 : 64;
		s->frames      = realloc(s->frames,      s->capacity * sizeof(uint8_t *));
		s->frame_sizes = realloc(s->frame_sizes, s->capacity * sizeof(size_t));
		s->delays_ms   = realloc(s->delays_ms,   s->capacity * sizeof(int));
	}
	size_t sz;
	s->frames[s->count]      = compress_surface(surface, s->width, s->height, &sz);
	s->frame_sizes[s->count] = sz;
	s->delays_ms[s->count]   = delay_ms;
	s->count++;
}

static int apng_finish(apng_state_t *s)
{
	FILE *f = fopen(s->filename, "wb");
	if (!f) { perror(s->filename); return 1; }

	int w = s->width, h = s->height, n = s->count;

	/* PNG signature */
	static const uint8_t sig[8] = {137, 80, 78, 71, 13, 10, 26, 10};
	fwrite(sig, 1, 8, f);

	/* IHDR */
	uint8_t ihdr[13] = {0};
	write_be32(ihdr + 0, (uint32_t)w);
	write_be32(ihdr + 4, (uint32_t)h);
	ihdr[8]  = 8;  /* bit depth */
	ihdr[9]  = 6;  /* colour type: RGBA */
	ihdr[10] = 0;  /* compression */
	ihdr[11] = 0;  /* filter */
	ihdr[12] = 0;  /* interlace */
	png_write_chunk(f, "IHDR", ihdr, 13);

	/* acTL — animation control */
	uint8_t actl[8];
	write_be32(actl + 0, (uint32_t)n);
	write_be32(actl + 4, 0);  /* loop count: 0 = infinite */
	png_write_chunk(f, "acTL", actl, 8);

	uint32_t seq = 0;
	for (int i = 0; i < n; i++) {
		/* fcTL — frame control */
		uint8_t fctl[26] = {0};
		write_be32(fctl + 0, seq++);
		write_be32(fctl + 4,  (uint32_t)w);
		write_be32(fctl + 8,  (uint32_t)h);
		write_be32(fctl + 12, 0);  /* x offset */
		write_be32(fctl + 16, 0);  /* y offset */
		/* delay: delay_num / delay_den seconds */
		uint16_t dnum = (uint16_t)(s->delays_ms[i]);
		uint16_t dden = 1000;
		write_be16(fctl + 20, dnum);
		write_be16(fctl + 22, dden);
		fctl[24] = 1;  /* dispose_op: APNG_DISPOSE_OP_BACKGROUND */
		fctl[25] = 0;  /* blend_op:   APNG_BLEND_OP_SOURCE */
		png_write_chunk(f, "fcTL", fctl, 26);

		if (i == 0) {
			/* First frame uses regular IDAT */
			png_write_chunk(f, "IDAT", s->frames[0], (uint32_t)s->frame_sizes[0]);
		} else {
			/* Subsequent frames use fdAT (4-byte sequence prefix) */
			uint32_t fdat_len = 4 + (uint32_t)s->frame_sizes[i];
			uint8_t *fdat = malloc(fdat_len);
			write_be32(fdat, seq++);
			memcpy(fdat + 4, s->frames[i], s->frame_sizes[i]);
			png_write_chunk(f, "fdAT", fdat, fdat_len);
			free(fdat);
		}
	}

	/* IEND */
	png_write_chunk(f, "IEND", NULL, 0);
	fclose(f);

	for (int i = 0; i < n; i++) free(s->frames[i]);
	free(s->frames);
	free(s->frame_sizes);
	free(s->delays_ms);
	free(s->filename);
	return 0;
}

/* ═══════════════════════════════════════════════════════════════════════
   Public output_ctx_t API
   ═══════════════════════════════════════════════════════════════════════ */

struct output_ctx_t {
	output_format_t fmt;
	int width, height;
	union {
		gif_state_t  gif;
		webp_state_t webp;
		apng_state_t apng;
	};
};

output_format_t output_detect_format(t2g_t *root, const char *filename)
{
	/* Explicit setting takes precedence */
	if (root->output_format) {
		if (strcmp(root->output_format, "webp") == 0) return OUTPUT_WEBP;
		if (strcmp(root->output_format, "apng") == 0) return OUTPUT_APNG;
		return OUTPUT_GIF;
	}
	/* Fallback: sniff the file extension */
	const char *ext = strrchr(filename, '.');
	if (ext) {
		if (strcasecmp(ext, ".webp") == 0) return OUTPUT_WEBP;
		if (strcasecmp(ext, ".apng") == 0) return OUTPUT_APNG;
		if (strcasecmp(ext, ".png")  == 0) return OUTPUT_APNG;
	}
	return OUTPUT_GIF;
}

output_ctx_t *output_create(const char *filename, output_format_t fmt,
                             int width, int height)
{
	output_ctx_t *ctx = calloc(1, sizeof(output_ctx_t));
	ctx->fmt    = fmt;
	ctx->width  = width;
	ctx->height = height;

	switch (fmt) {
	case OUTPUT_GIF:
		ctx->gif.f = fopen(filename, "wb");
		if (!ctx->gif.f) { perror(filename); free(ctx); return NULL; }
		break;

	case OUTPUT_WEBP: {
		ctx->webp.f = fopen(filename, "wb");
		if (!ctx->webp.f) { perror(filename); free(ctx); return NULL; }
		WebPAnimEncoderOptions opts;
		WebPAnimEncoderOptionsInit(&opts);
		opts.anim_params.loop_count = 0;  /* infinite */
		ctx->webp.enc = WebPAnimEncoderNew(width, height, &opts);
		ctx->webp.width        = width;
		ctx->webp.height       = height;
		ctx->webp.timestamp_ms = 0;
		break;
	}

	case OUTPUT_APNG:
		ctx->apng.width    = width;
		ctx->apng.height   = height;
		ctx->apng.filename = strdup(filename);
		break;
	}

	return ctx;
}

void output_add_frame(output_ctx_t *ctx, cairo_surface_t *surface, int delay_ms)
{
	switch (ctx->fmt) {
	case OUTPUT_GIF:  gif_add (&ctx->gif,  surface, delay_ms); break;
	case OUTPUT_WEBP: webp_add(&ctx->webp, surface, delay_ms); break;
	case OUTPUT_APNG: apng_add(&ctx->apng, surface, delay_ms); break;
	}
}

int output_finish(output_ctx_t *ctx)
{
	int ret = 0;
	switch (ctx->fmt) {
	case OUTPUT_GIF:  ret = gif_finish (&ctx->gif);  break;
	case OUTPUT_WEBP: ret = webp_finish(&ctx->webp); break;
	case OUTPUT_APNG: ret = apng_finish(&ctx->apng); break;
	}
	return ret;
}

void output_destroy(output_ctx_t *ctx)
{
	free(ctx);
}

/* ═══════════════════════════════════════════════════════════════════════
   High-level entry point
   ═══════════════════════════════════════════════════════════════════════ */

/* Returns the gap in world-space pixels between consecutive event positions. */
static double event_gap(t2g_t *root, t2g_t *prev_ev, int prev_idx,
                         t2g_t *curr_ev, int curr_idx)
{
	double prev_wx = (prev_idx >= 0)
		? render_event_world_x(root, prev_ev, prev_idx)
		: 0.0;
	double curr_wx = render_event_world_x(root, curr_ev, curr_idx);
	return curr_wx - prev_wx;
}

/* Number of fast-transit frames to insert before animating an event.
   Zero for normal gaps; scales with gap beyond 3× item_spacing. */
static int transit_frame_count(t2g_t *root, double gap)
{
	double threshold = root->item_spacing * 3.0;
	if (gap <= threshold) return 0;
	int n = (int)((gap - threshold) / (root->item_spacing * 2.0));
	return n < 2 ? 2 : (n > 10 ? 10 : n);
}

/* True if transitions are enabled. */
static int transitions_enabled(t2g_t *root)
{
	return root->transition_style
		&& strcmp(root->transition_style, "none") != 0
		&& root->transition_frames > 0;
}

int write_output(t2g_t *root, const char *filename)
{
	output_format_t fmt = output_detect_format(root, filename);
	output_ctx_t   *ctx = output_create(filename, fmt, root->width, root->height);
	if (!ctx) return 1;

	t2g_t *first_event = root->next;
	t2g_t *iter        = first_event;
	t2g_t *prev_iter   = NULL;
	int    event_index = 0;

	/* Start camera already centred on the first event so it never
	   appears flush against the left edge. */
	double camera_x_prev = first_event
		? render_camera_target(root, first_event, 0)
		: 0.0;

	while (iter) {
		double camera_x_target = render_camera_target(root, iter, event_index);

		/* ── Fast-transit frames for large time gaps ───────────────── */
		double gap     = event_gap(root, prev_iter, event_index - 1,
		                           iter, event_index);
		int    transit = transit_frame_count(root, gap);

		for (int tf = 0; tf < transit; tf++) {
			double t   = ease_in_out_cubic((double)(tf + 1) / (transit + 1));
			double cam = lerp(camera_x_prev, camera_x_target, t);
			cairo_surface_t *s = render_transit_frame(
				root, first_event, event_index, cam);
			output_add_frame(ctx, s, root->speed_frames * 4);
			cairo_surface_destroy(s);
		}

		/* If a transit already moved the camera to target, keep it there
		   during the event animation (no second pan of the same distance). */
		double anim_cam_start = (transit > 0) ? camera_x_target : camera_x_prev;

		/* ── Standard event animation frames ──────────────────────── */
		for (int frame = 0; frame < FRAMES_PER_ITEM; frame++) {
			cairo_surface_t *surface = render_frame(
				root, first_event,
				event_index,    /* committed_count */
				event_index,    /* current_index   */
				frame,
				anim_cam_start,
				camera_x_target);

			int delay_ms = (frame == FRAMES_PER_ITEM - 1)
				? root->speed_nextitem * 10
				: root->speed_frames   * 10;

			output_add_frame(ctx, surface, delay_ms);
			cairo_surface_destroy(surface);
		}

		/* ── Transition to next event ──────────────────────────────── */
		if (iter->next && transitions_enabled(root)) {
			int    nf          = root->transition_frames;
			double next_cam    = render_camera_target(root, iter->next,
			                                          event_index + 1);

			/* "from": committed events at current camera position       */
			/* "to":   committed events at next event's camera position  */
			cairo_surface_t *from = render_transit_frame(
				root, first_event, event_index + 1, camera_x_target);
			cairo_surface_t *to   = render_transit_frame(
				root, first_event, event_index + 1, next_cam);

			for (int tf = 0; tf < nf; tf++) {
				double t = (double)(tf + 1) / (nf + 1);
				cairo_surface_t *trans = render_transition_frame(
					from, to, t,
					root->transition_style,
					root->width, root->height);
				output_add_frame(ctx, trans, root->speed_frames * 10);
				cairo_surface_destroy(trans);
			}

			cairo_surface_destroy(from);
			cairo_surface_destroy(to);

			/* Camera is already at next_cam after the transition;
			   the next event's animation starts from there. */
			camera_x_prev = next_cam;
		} else {
			camera_x_prev = camera_x_target;
		}

		prev_iter = iter;
		event_index++;
		iter = iter->next;
	}

	int ret = output_finish(ctx);
	output_destroy(ctx);
	return ret;
}
