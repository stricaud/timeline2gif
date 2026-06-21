#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#include <cairo/cairo.h>
#include <pango/pangocairo.h>
#include <librsvg/rsvg.h>
#include <gd.h>

#include "render.h"
#include "easing.h"
#include "t2g.h"

/* ── Color helpers ──────────────────────────────────────────────────── */

static void set_color(cairo_t *cr, t2gcolor_t c, double alpha)
{
	cairo_set_source_rgba(cr,
	                      c.r / 255.0,
	                      c.g / 255.0,
	                      c.b / 255.0,
	                      alpha * (c.a / 255.0));
}

static t2gcolor_t dot_color (t2g_t *root, t2g_t *ev)
{ return ev->has_dot_color  ? ev->dot_color  : root->theme_accent; }

static t2gcolor_t line_color(t2g_t *root, t2g_t *ev)
{ return ev->has_line_color ? ev->line_color : root->theme_accent; }

static t2gcolor_t text_color(t2g_t *root, t2g_t *ev)
{ return ev->has_text_color ? ev->text_color : root->theme_text; }

/* ── Geometry ───────────────────────────────────────────────────────── */

/* Extract a numeric time value from a string using the declared format.
 *   "year"   — first 4-digit run that looks like a year (1000–9999)
 *   "number" — first numeric token (integer or decimal, optional leading sign)
 */
static double parse_time_value(const char *text, const char *fmt)
{
	if (!text || !fmt) return 0.0;

	if (strcmp(fmt, "year") == 0 || strcmp(fmt, "YYYY") == 0) {
		for (const char *p = text; *p; p++) {
			if (*p >= '0' && *p <= '9') {
				int v = 0;
				const char *q = p;
				while (*q >= '0' && *q <= '9') v = v * 10 + (*q++ - '0');
				if (v >= 1000 && v <= 9999) return (double)v;
				p = q - 1;
			}
		}
	} else if (strcmp(fmt, "number") == 0) {
		const char *p = text;
		while (*p && !((*p >= '0' && *p <= '9') || *p == '-' || *p == '.')) p++;
		if (*p) return atof(p);
	}
	return 0.0;
}

double render_event_world_x(t2g_t *root, t2g_t *ev, int index)
{
	/* Explicit per-event override always wins */
	if (ev && ev->x_pos > 0)
		return (double)ev->x_pos;

	/* Time-based auto-positioning */
	if (root->time_format) {
		double tv = parse_time_value(ev ? ev->time_text : NULL,
		                             root->time_format);

		/* Origin: explicit value, or auto-detect from first event */
		double origin = root->has_time_origin
			? root->time_origin
			: (root->next
				? parse_time_value(root->next->time_text, root->time_format)
				: 0.0);

		/* First event lands at item_spacing; others are proportional */
		return (tv - origin) * root->item_spacing + root->item_spacing;
	}

	/* Sequential fallback */
	return (double)(index + 1) * root->item_spacing;
}

double render_camera_target(t2g_t *root, t2g_t *ev, int index)
{
	double x = render_event_world_x(root, ev, index);
	if (root->split_show) {
		/* In split mode the animation occupies the right portion of the
		   canvas.  Place the event dot at 40 % of that right portion. */
		int pw = root->split_width;
		return x - (pw + (root->width - pw) * 0.4);
	}
	return x - root->width * 0.4;
}

/* Above (dir=-1) for even indices, below (+1) for odd. */
static int connector_dir(int index)
{
	return (index % 2 == 0) ? -1 : 1;
}

/*
 * Connector length (timeline → label anchor) varies per event so labels
 * land at different y positions rather than two fixed heights.
 * Even indices go above, odd below; each side cycles its own sequence.
 */
static double connector_h(int index)
{
	static const double above[] = { 85, 115, 60, 105, 75 };
	static const double below[] = { 90, 125, 65, 110, 80 };
	int group = index / 2;
	int n     = 5;
	return (index % 2 == 0) ? above[group % n] : below[group % n];
}

/* ── Pango text ─────────────────────────────────────────────────────── */

static void draw_text(cairo_t    *cr,
                      const char *text,
                      const char *family,
                      int         pt_size,
                      double      cx,
                      double      cy,
                      t2gcolor_t  color,
                      double      alpha,
                      int         canvas_w)
{
	if (!text || !*text || alpha <= 0.0) return;

	PangoLayout *layout = pango_cairo_create_layout(cr);
	char desc_str[256];
	snprintf(desc_str, sizeof(desc_str), "%s %d", family, pt_size);
	PangoFontDescription *desc = pango_font_description_from_string(desc_str);
	pango_layout_set_font_description(layout, desc);
	pango_font_description_free(desc);
	pango_layout_set_text(layout, text, -1);
	pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);

	int pw, ph;
	pango_layout_get_pixel_size(layout, &pw, &ph);

	/* Keep within canvas edges */
	if (canvas_w > 0) {
		double half = pw / 2.0, margin = 6.0;
		if (cx - half < margin)            cx = margin + half;
		if (cx + half > canvas_w - margin) cx = canvas_w - margin - half;
	}

	cairo_move_to(cr, cx - pw / 2.0, cy - ph / 2.0);
	set_color(cr, color, alpha);
	pango_cairo_show_layout(cr, layout);
	g_object_unref(layout);
}

/* Return half the pixel width of ev->label_text. */
static double label_half_width(cairo_t *cr, t2g_t *ev, t2g_t *root)
{
	const char *text = ev->label_text;
	if (!text || !*text) return 0;
	PangoLayout *layout = pango_cairo_create_layout(cr);
	char d[256];
	snprintf(d, sizeof(d), "%s %d",
	         t2g_get_description_font(ev),
	         t2g_get_description_font_size(ev));
	PangoFontDescription *pd = pango_font_description_from_string(d);
	pango_layout_set_font_description(layout, pd);
	pango_font_description_free(pd);
	pango_layout_set_text(layout, text, -1);
	int pw, ph;
	pango_layout_get_pixel_size(layout, &pw, &ph);
	g_object_unref(layout);
	(void)root;
	return pw / 2.0;
}

/*
 * Pre-compute non-overlapping label centre-x positions.
 *
 * Default: centred on the dot's screen x.
 * Same-side labels (above = even index, below = odd) are pushed rightward
 * until no two labels overlap.  If a label is pushed, an L-shaped connector
 * links the dot to the label (drawn by the caller).
 *
 * out[]  — array of [total_events] adjusted label_cx values.
 * hw[]   — array of [total_events] label half-widths (filled in).
 */
#define MAX_LABEL_EVENTS 64
#define LABEL_GAP        8.0   /* minimum gap between adjacent label edges */

static void compute_label_cx(cairo_t *cr, t2g_t *root,
                               t2g_t *first_event, int n,
                               double camera_x,
                               double *out, double *hw)
{
	if (n > MAX_LABEL_EVENTS) n = MAX_LABEL_EVENTS;

	/* Default positions and widths */
	t2g_t *iter = first_event;
	for (int i = 0; i < n && iter; i++, iter = iter->next) {
		out[i] = render_event_world_x(root, iter, i) - camera_x;
		hw[i]  = label_half_width(cr, iter, root);
	}

	/* Push "above" labels (even indices 0,2,4…) rightward */
	double prev_right = 6.0;
	for (int i = 0; i < n; i += 2) {
		double left = out[i] - hw[i];
		if (left < prev_right + LABEL_GAP)
			out[i] = prev_right + LABEL_GAP + hw[i];
		if (out[i] + hw[i] > root->width - 6)
			out[i] = root->width - 6 - hw[i];
		prev_right = out[i] + hw[i];
	}

	/* Push "below" labels (odd indices 1,3,5…) rightward */
	prev_right = 6.0;
	for (int i = 1; i < n; i += 2) {
		double left = out[i] - hw[i];
		if (left < prev_right + LABEL_GAP)
			out[i] = prev_right + LABEL_GAP + hw[i];
		if (out[i] + hw[i] > root->width - 6)
			out[i] = root->width - 6 - hw[i];
		prev_right = out[i] + hw[i];
	}
}

/* ── Image rendering (SVG + PNG + GIF + JPEG) ───────────────────────── */

/* Convert a GD truecolor image (already scaled) to a Cairo ARGB32 surface.
   GD alpha: 0 = opaque, 127 = transparent  (inverted, 7-bit).
   Cairo ARGB32: 255 = opaque, 0 = transparent, RGB premultiplied by alpha. */
static cairo_surface_t *gd_to_cairo(gdImagePtr gd, int w, int h)
{
	cairo_surface_t *surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
	cairo_surface_flush(surf);
	unsigned char *cdata   = cairo_image_surface_get_data(surf);
	int            cstride = cairo_image_surface_get_stride(surf);

	for (int y = 0; y < h; y++) {
		uint32_t *row = (uint32_t *)(cdata + y * cstride);
		for (int x = 0; x < w; x++) {
			int     px = gdImageGetTrueColorPixel(gd, x, y);
			uint8_t r  = (uint8_t)gdTrueColorGetRed(px);
			uint8_t g  = (uint8_t)gdTrueColorGetGreen(px);
			uint8_t b  = (uint8_t)gdTrueColorGetBlue(px);
			int     ga = gdTrueColorGetAlpha(px);          /* 0–127 */
			uint8_t a  = (uint8_t)(255 - ga * 255 / 127);
			row[x] = ((uint32_t)a        << 24) |
			         ((uint32_t)(r*a/255) << 16) |
			         ((uint32_t)(g*a/255) <<  8) |
			          (uint32_t)(b*a/255);
		}
	}
	cairo_surface_mark_dirty(surf);
	return surf;
}

static void draw_image(cairo_t    *cr,
                       const char *path,
                       int         target_size,
                       double      cx,
                       double      cy,
                       double      scale,
                       double      alpha)
{
	if (!path || scale <= 0.0 || alpha <= 0.0) return;

	const char *ext = strrchr(path, '.');
	double sz = target_size * scale;
	double hx = cx - sz / 2.0;
	double hy = cy - sz / 2.0;

	/* A define.svg reference stores the SVG markup itself; detect it by the
	   leading '<' (after any whitespace) rather than a file extension. */
	const char *q = path;
	while (*q == ' ' || *q == '\t' || *q == '\n' || *q == '\r') q++;
	int inline_svg = (*q == '<');

	if (inline_svg || (ext && strcasecmp(ext, ".svg") == 0)) {
		/* ── SVG via librsvg (from memory or file) ── */
		GError     *gerr   = NULL;
		RsvgHandle *handle = inline_svg
			? rsvg_handle_new_from_data((const guint8 *)path, strlen(path), &gerr)
			: rsvg_handle_new_from_file(path, &gerr);
		if (!handle) {
			if (gerr) {
				fprintf(stderr, "SVG load error (%s): %s\n",
				        inline_svg ? "<inline>" : path, gerr->message);
				g_error_free(gerr);
			}
			return;
		}
		RsvgRectangle vp = { hx, hy, sz, sz };
		cairo_push_group(cr);
		rsvg_handle_render_document(handle, cr, &vp, NULL);
		cairo_pop_group_to_source(cr);
		cairo_paint_with_alpha(cr, alpha);
		g_object_unref(handle);

	} else if (ext && strcasecmp(ext, ".png") == 0) {
		/* ── PNG via Cairo native ── */
		cairo_surface_t *img = cairo_image_surface_create_from_png(path);
		if (cairo_surface_status(img) != CAIRO_STATUS_SUCCESS) {
			fprintf(stderr, "PNG load error (%s)\n", path);
			cairo_surface_destroy(img);
			return;
		}
		int iw = cairo_image_surface_get_width(img);
		int ih = cairo_image_surface_get_height(img);
		cairo_save(cr);
		cairo_translate(cr, hx, hy);
		cairo_scale(cr, sz / iw, sz / ih);
		cairo_set_source_surface(cr, img, 0, 0);
		cairo_paint_with_alpha(cr, alpha);
		cairo_restore(cr);
		cairo_surface_destroy(img);

	} else {
		/* ── GIF / JPEG (and any other raster) via GD ── */
		FILE *f = fopen(path, "rb");
		if (!f) { fprintf(stderr, "Cannot open image: %s\n", path); return; }

		gdImagePtr src = NULL;
		if (ext && strcasecmp(ext, ".gif") == 0) {
			src = gdImageCreateFromGif(f);
		} else if (ext && (strcasecmp(ext, ".jpg")  == 0 ||
		                   strcasecmp(ext, ".jpeg") == 0)) {
			src = gdImageCreateFromJpeg(f);
		}
		fclose(f);

		if (!src) { fprintf(stderr, "Failed to load image: %s\n", path); return; }

		int isz = (int)(sz > 0.5 ? sz : 1.0);
		gdImagePtr scaled = gdImageCreateTrueColor(isz, isz);
		gdImageSaveAlpha(scaled, 1);
		gdImageAlphaBlending(scaled, 0);
		gdImageCopyResampled(scaled, src, 0, 0, 0, 0,
		                     isz, isz, gdImageSX(src), gdImageSY(src));
		gdImageDestroy(src);

		cairo_surface_t *surf = gd_to_cairo(scaled, isz, isz);
		gdImageDestroy(scaled);

		cairo_save(cr);
		cairo_translate(cr, hx, hy);
		cairo_set_source_surface(cr, surf, 0, 0);
		cairo_paint_with_alpha(cr, alpha);
		cairo_restore(cr);
		cairo_surface_destroy(surf);
	}
}

/* ── Background ─────────────────────────────────────────────────────── */

static t2gcolor_t lerp_color(t2gcolor_t a, t2gcolor_t b, double t)
{
	t2gcolor_t r;
	r.a = (unsigned char)((double)a.a + ((double)b.a - a.a) * t);
	r.r = (unsigned char)((double)a.r + ((double)b.r - a.r) * t);
	r.g = (unsigned char)((double)a.g + ((double)b.g - a.g) * t);
	r.b = (unsigned char)((double)a.b + ((double)b.b - a.b) * t);
	return r;
}

/* Find the background colors that are active after committed_count events. */
static void resolve_background(t2g_t *root,
                                t2g_t *first_event, int committed_count,
                                t2gcolor_t *out_bg1, t2gcolor_t *out_bg2)
{
	*out_bg1 = root->theme_background;
	*out_bg2 = root->theme_background2;
	t2g_t *iter = first_event;
	for (int i = 0; i < committed_count && iter; i++, iter = iter->next) {
		if (iter->has_ev_background) {
			*out_bg1 = iter->ev_background;
			*out_bg2 = iter->ev_background2;
		}
	}
}

static void draw_background_colors(cairo_t *cr, t2g_t *root,
                                    t2gcolor_t bg1, t2gcolor_t bg2)
{
	cairo_pattern_t *pat = cairo_pattern_create_linear(0, 0, 0, root->height);
	cairo_pattern_add_color_stop_rgba(pat, 0.0,
	    bg1.r / 255.0, bg1.g / 255.0, bg1.b / 255.0, 1.0);
	cairo_pattern_add_color_stop_rgba(pat, 1.0,
	    bg2.r / 255.0, bg2.g / 255.0, bg2.b / 255.0, 1.0);
	cairo_set_source(cr, pat);
	cairo_paint(cr);
	cairo_pattern_destroy(pat);
}

/* ── Timeline line with ticks and per-segment colour ────────────────── */

/* ── Spring-drop effect ─────────────────────────────────────────────────
   A new event's dot falls from above, lands on the timeline, and the line
   springs back and forth locally around the impact point before settling.

   Two pieces work together:
     • spring_drop()  — the temporal motion (fall, then damped oscillation)
     • line_disp()    — the spatial falloff that keeps the wobble local to
                        the "drop zone" rather than moving the whole line.   */

#define SPRING_IMPACT_T 0.34   /* fraction of the entrance spent falling */

/* Temporal motion for a spring drop at progress `prog` (0..1).
   Outputs the dot's vertical offset from the line, and the line's local
   dip amplitude at the impact point (0 until the dot lands).
     fall_h  — how far above the line the dot starts (px)
     dip_amp — peak local dip of the line on impact (px) */
static void spring_drop(double prog, double fall_h, double dip_amp,
                        double *dot_dy, double *line_amp)
{
	double t = clamp01(prog);
	if (t < SPRING_IMPACT_T) {
		/* Fall: ease-in (gravity-like accel) from fall_h above down to 0. */
		double f = t / SPRING_IMPACT_T;
		*dot_dy   = -fall_h * (1.0 - f * f);
		*line_amp = 0.0;
	} else {
		/* Spring: damped oscillation. sin(3π·s) is 0 at s=0 and s=1, so the
		   line starts and ends exactly at rest; e^(−3s) damps the bounces. */
		double s   = (t - SPRING_IMPACT_T) / (1.0 - SPRING_IMPACT_T);
		double osc = sin(3.0 * M_PI * s) * exp(-3.0 * s);
		*line_amp = dip_amp * osc;
		*dot_dy   = dip_amp * osc;   /* the dot rides the springing line */
	}
}

/* Spatial falloff: a bell curve centred on the impact x so the deformation
   stays in the drop zone and decays to nothing further along the line. */
static double line_disp(double x, double cx, double amp, double sigma)
{
	if (amp == 0.0) return 0.0;
	double d = x - cx;
	return amp * exp(-(d * d) / (2.0 * sigma * sigma));
}

static void draw_timeline_line(cairo_t *cr, t2g_t *root,
                                t2g_t *first_event, int committed_count,
                                double camera_x,
                                double disp_cx, double disp_amp,
                                double disp_sigma)
{
	double ty = root->timeline_pos_y;
	double w  = root->width;

	/* Lay out the line path. When the drop zone is deforming (disp_amp != 0)
	   the line is a curve sampled across x; otherwise it's a fast straight
	   segment (keeps non-drop frames byte-identical). */
	#define TIMELINE_PATH() do {                                           \
		if (disp_amp == 0.0) {                                             \
			cairo_move_to(cr, 0, ty);                                      \
			cairo_line_to(cr, w, ty);                                      \
		} else {                                                           \
			cairo_move_to(cr, 0, ty + line_disp(0, disp_cx, disp_amp, disp_sigma)); \
			for (double px = 4; px < w; px += 4)                           \
				cairo_line_to(cr, px, ty + line_disp(px, disp_cx, disp_amp, disp_sigma)); \
			cairo_line_to(cr, w, ty + line_disp(w, disp_cx, disp_amp, disp_sigma)); \
		}                                                                  \
	} while (0)

	/* Glow */
	cairo_set_line_width(cr, 8);
	set_color(cr, root->theme_accent, 0.12);
	TIMELINE_PATH();
	cairo_stroke(cr);

	/* Main line */
	cairo_set_line_width(cr, 2);
	set_color(cr, root->theme_accent, 0.85);
	TIMELINE_PATH();
	cairo_stroke(cr);
	#undef TIMELINE_PATH

	/* Minor tick marks */
	int minor_step = root->item_spacing / 4;
	if (minor_step < 4) minor_step = 4;
	double world_left  = camera_x - root->item_spacing;
	double world_right = camera_x + w + root->item_spacing;
	int first_tick = (int)(world_left / minor_step) * minor_step;
	cairo_set_line_width(cr, 1.0);
	set_color(cr, root->theme_accent, 0.25);
	for (int wx = first_tick; wx < (int)world_right; wx += minor_step) {
		double sx = wx - camera_x;
		if (sx < -1 || sx > w + 1) continue;
		double dy = line_disp(sx, disp_cx, disp_amp, disp_sigma);
		cairo_move_to(cr, sx, ty - 4 + dy); cairo_line_to(cr, sx, ty + 4 + dy);
		cairo_stroke(cr);
	}

	/* Per-segment colour overrides */
	{
		t2g_t *seg    = first_event;
		double prev_wx = 0;
		for (int i = 0; i < committed_count && seg; i++, seg = seg->next) {
			double curr_wx = render_event_world_x(root, seg, i);
			if (seg->has_timeline_color) {
				double sx1 = prev_wx - camera_x;
				double sx2 = curr_wx - camera_x;
				if (sx2 > 0 && sx1 < w) {
					if (sx1 < 0) sx1 = 0;
					if (sx2 > w) sx2 = w;
					cairo_set_line_width(cr, 2.5);
					set_color(cr, seg->ev_timeline_color, 0.9);
					if (disp_amp == 0.0) {
						cairo_move_to(cr, sx1, ty);
						cairo_line_to(cr, sx2, ty);
					} else {
						cairo_move_to(cr, sx1, ty + line_disp(sx1, disp_cx, disp_amp, disp_sigma));
						for (double px = sx1 + 4; px < sx2; px += 4)
							cairo_line_to(cr, px, ty + line_disp(px, disp_cx, disp_amp, disp_sigma));
						cairo_line_to(cr, sx2, ty + line_disp(sx2, disp_cx, disp_amp, disp_sigma));
					}
					cairo_stroke(cr);
				}
			}
			prev_wx = curr_wx;
		}
	}

	/* Major ticks at committed event positions */
	cairo_set_line_width(cr, 1.5);
	set_color(cr, root->theme_accent, 0.5);
	t2g_t *iter = first_event;
	for (int i = 0; i < committed_count && iter; i++, iter = iter->next) {
		double ex = render_event_world_x(root, iter, i) - camera_x;
		if (ex < -1 || ex > w + 1) continue;
		double dy = line_disp(ex, disp_cx, disp_amp, disp_sigma);
		cairo_move_to(cr, ex, ty - 9 + dy); cairo_line_to(cr, ex, ty + 9 + dy);
		cairo_stroke(cr);
	}
}

/* ── Dot / icon element ─────────────────────────────────────────────── */

static void draw_dot_element(cairo_t *cr, t2g_t *root, t2g_t *ev,
                              double sx, double ty,
                              double scale, double alpha)
{
	if (ev->dot_image) {
		int sz = ev->dot_image_size > 0 ? ev->dot_image_size : DEFAULT_ICON_SIZE;
		draw_image(cr, ev->dot_image, sz, sx, ty, scale, alpha);
		return;
	}
	t2gcolor_t dc = dot_color(root, ev);
	double r = DOT_RADIUS * scale;
	if (r < 0.5) return;

	cairo_arc(cr, sx + 1.5 * scale, ty + 1.5 * scale, r, 0, 2 * M_PI);
	cairo_set_source_rgba(cr, 0, 0, 0, 0.35 * alpha);
	cairo_fill(cr);

	cairo_arc(cr, sx, ty, r, 0, 2 * M_PI);
	set_color(cr, dc, alpha);
	cairo_fill(cr);

	cairo_arc(cr, sx, ty, r + 1.5 * scale, 0, 2 * M_PI);
	cairo_set_line_width(cr, 1.0);
	set_color(cr, dc, 0.3 * alpha);
	cairo_stroke(cr);
}

/*
 * Draw L-shaped connector from (dot_sx, timeline_y) to (label_cx, label_y).
 *
 * The connector goes straight up/down from the dot for CONNECTOR_H pixels,
 * then horizontally to the label's x position.
 */
static void draw_connector(cairo_t *cr,
                            double dot_sx, double label_cx,
                            double ty, double label_shelf_y,
                            t2gcolor_t lc, double alpha)
{
	cairo_set_line_width(cr, 1.5);
	set_color(cr, lc, 0.55 * alpha);

	/* Vertical leg */
	cairo_move_to(cr, dot_sx,   ty);
	cairo_line_to(cr, dot_sx,   label_shelf_y);

	/* Horizontal leg (only if label was pushed) */
	if (fabs(label_cx - dot_sx) > 2.0) {
		cairo_line_to(cr, label_cx, label_shelf_y);
	}
	cairo_stroke(cr);
}

/* When an event has both a label image and label text, the caption is dropped
   below the icon so the two don't overlap. Returns the y at which to centre
   the label text given the icon's centre y. With no image, text stays put. */
static double label_text_cy(t2g_t *ev, double icon_cy)
{
	if (!ev->label_image || !ev->label_text || !*ev->label_text)
		return icon_cy;
	int isz = ev->label_image_size > 0 ? ev->label_image_size : DEFAULT_LABEL_IMAGE_SIZE;
	int fs  = t2g_get_description_font_size(ev);
	return icon_cy + isz / 2.0 + 6 + fs * 0.7;   /* below the icon, small gap */
}

/* ── Draw one fully-committed event ─────────────────────────────────── */

static void draw_committed_event(cairo_t *cr, t2g_t *root, t2g_t *ev,
                                  int index, double camera_x,
                                  double label_cx, double alpha)
{
	double wx    = render_event_world_x(root, ev, index);
	double sx    = wx - camera_x;
	double ty    = root->timeline_pos_y;
	int    dir   = connector_dir(index);

	if (sx < -DEFAULT_ICON_SIZE || sx > root->width + DEFAULT_ICON_SIZE) return;

	double ch      = connector_h(index);
	double shelf_y = ty + dir * ch;
	double label_y = shelf_y + dir * 14;
	double time_y  = ty - dir * 20;

	t2gcolor_t lc = line_color(root, ev);
	t2gcolor_t tc = text_color(root, ev);

	draw_connector(cr, sx, label_cx, ty + dir * DOT_RADIUS, shelf_y, lc, alpha);
	draw_dot_element(cr, root, ev, sx, ty, 1.0, alpha);
	if (ev->label_image) {
		int sz = ev->label_image_size > 0 ? ev->label_image_size : DEFAULT_LABEL_IMAGE_SIZE;
		draw_image(cr, ev->label_image, sz, label_cx, label_y, 1.0, alpha);
	}
	draw_text(cr, ev->label_text,
	          t2g_get_description_font(ev),
	          t2g_get_description_font_size(ev),
	          label_cx, label_text_cy(ev, label_y), tc, alpha, root->width);
	draw_text(cr, ev->time_text,
	          t2g_get_time_font(ev),
	          t2g_get_time_font_size(ev),
	          sx, time_y, tc, 0.65 * alpha, root->width);
}

/* ── Draw the currently-animating event ─────────────────────────────── */

static void draw_animating_event(cairo_t *cr, t2g_t *root, t2g_t *ev,
                                  int index, int frame, double camera_x,
                                  double label_cx)
{
	double prog = (double)frame / (FRAMES_PER_ITEM - 1);
	double wx   = render_event_world_x(root, ev, index);
	double sx   = wx - camera_x;
	double ty   = root->timeline_pos_y;
	int    dir  = connector_dir(index);

	double ch      = connector_h(index);
	double shelf_y = ty + dir * ch;
	double label_y = shelf_y + dir * 14;
	double time_y  = ty - dir * 20;

	double dot_t, line_frac, text_a;
	double dy = 0.0;        /* dot's vertical offset (fall + spring) */
	int    drop_on = t2g_event_drop(root, ev);

	if (drop_on) {
		/* Spring drop: the dot falls from above, lands, and rides the line
		   as it springs. Connector and label wait until it has landed so the
		   drop reads clearly before the rest assembles. */
		double fall_h = t2g_event_drop_amount(root, ev);
		double line_amp;
		spring_drop(prog, fall_h, fall_h * 0.30, &dy, &line_amp);
		dot_t     = ease_out_cubic(clamp01(prog / (SPRING_IMPACT_T * 0.7)));
		line_frac = ease_out_cubic(clamp01((prog - (SPRING_IMPACT_T + 0.06)) / 0.30));
		text_a    = ease_out_cubic(clamp01((prog - (SPRING_IMPACT_T + 0.22)) / 0.40));
	} else {
		dot_t     = ease_out_elastic(clamp01(prog * 2.2));
		line_frac = ease_out_cubic(clamp01((prog - 0.20) / 0.45));
		/* Text overlaps connector growth (starts at 42% not 62%) for a livelier
		   sequence, and slides into place from the connector direction. */
		text_a    = ease_out_cubic(clamp01((prog - 0.42) / 0.58));
	}

	t2gcolor_t lc = line_color(root, ev);
	t2gcolor_t tc = text_color(root, ev);

	/* Pulse ring */
	if (dot_t > 0.05 && dot_t < 1.2 && !ev->dot_image) {
		double pulse = dot_t > 1.0 ? 2.0 - dot_t : dot_t;
		cairo_arc(cr, sx, ty + dy, DOT_RADIUS + 8 * dot_t, 0, 2 * M_PI);
		cairo_set_line_width(cr, 1.5);
		set_color(cr, dot_color(root, ev), 0.4 * (1.0 - pulse * 0.7));
		cairo_stroke(cr);
	}

	/* L-connector grows in: first vertical, then horizontal */
	if (line_frac > 0.01) {
		double grown_shelf = ty + dir * (ch * line_frac);
		double horiz_cx    = lerp(sx, label_cx, clamp01((line_frac - 0.7) / 0.3));
		draw_connector(cr, sx, horiz_cx,
		               ty + dy + dir * DOT_RADIUS, grown_shelf, lc, line_frac);
	}

	/* Dot / icon */
	draw_dot_element(cr, root, ev, sx, ty + dy, dot_t, dot_t);

	/* Label image + text slide in from the connector side while fading. */
	if (text_a > 0.01) {
		double label_slide = 12.0 * (1.0 - text_a);
		double time_slide  =  6.0 * (1.0 - text_a);

		double icon_cy = label_y - dir * label_slide;
		if (ev->label_image) {
			int sz = ev->label_image_size > 0 ? ev->label_image_size : DEFAULT_LABEL_IMAGE_SIZE;
			draw_image(cr, ev->label_image, sz,
			           label_cx, icon_cy, 1.0, text_a);
		}

		draw_text(cr, ev->label_text,
		          t2g_get_description_font(ev),
		          t2g_get_description_font_size(ev),
		          label_cx, label_text_cy(ev, icon_cy),
		          tc, text_a, root->width);

		draw_text(cr, ev->time_text,
		          t2g_get_time_font(ev),
		          t2g_get_time_font_size(ev),
		          sx, time_y + dir * time_slide,
		          tc, text_a * 0.65, root->width);
	}
}

/* ── Callout overlay ────────────────────────────────────────────────── */

/* Rounded-rectangle path (r=0 → sharp corners). */
static void rounded_rect_path(cairo_t *cr,
                               double x, double y, double w, double h, double r)
{
	cairo_new_path(cr);
	if (r <= 0.0) {
		cairo_rectangle(cr, x, y, w, h);
		return;
	}
	cairo_arc(cr, x + r,     y + r,     r, M_PI,       M_PI * 1.5);
	cairo_arc(cr, x + w - r, y + r,     r, M_PI * 1.5, 0.0);
	cairo_arc(cr, x + w - r, y + h - r, r, 0.0,        M_PI * 0.5);
	cairo_arc(cr, x + r,     y + h - r, r, M_PI * 0.5, M_PI);
	cairo_close_path(cr);
}

/* Fill a cloud silhouette: body with rounded bottom + bump row on top.
   The body is the same width as the bump span so nothing sticks out.
   The caller must set the Cairo source before calling. */
static void cloud_fill(cairo_t *cr, double cx, double cy, double bw, double bh)
{
	double bump_r = bh * 0.36;
	double top_cy = cy - bh * 0.16;   /* bump row centre */

	int    n    = (int)((bw * 0.82) / (bump_r * 1.25)) + 1;
	if (n < 3) n = 3;
	double span  = bw * 0.82 - 2.0 * bump_r;
	double step  = (n > 1) ? span / (n - 1) : 0.0;
	double x0    = cx - span * 0.5;

	/* Body: exact same width as the bump span, only bottom corners rounded. */
	double bx      = x0 - bump_r;
	double bw_body = span + 2.0 * bump_r;
	double by      = top_cy;                      /* starts at bump centres */
	double bh_body = (cy + bh * 0.46) - by;
	double cr_r    = bh_body * 0.38;              /* bottom corner radius */

	cairo_new_path(cr);
	cairo_move_to(cr, bx, by);
	cairo_line_to(cr, bx + bw_body, by);
	cairo_line_to(cr, bx + bw_body, by + bh_body - cr_r);
	cairo_arc(cr, bx + bw_body - cr_r, by + bh_body - cr_r, cr_r, 0.0, M_PI * 0.5);
	cairo_arc(cr, bx + cr_r,           by + bh_body - cr_r, cr_r, M_PI * 0.5, M_PI);
	cairo_line_to(cr, bx, by);
	cairo_close_path(cr);
	cairo_fill(cr);

	/* Bump row drawn on top — upper hemispheres stick above the body,
	   lower halves overlap and cover the body's flat top edge. */
	for (int i = 0; i < n; i++) {
		cairo_arc(cr, x0 + i * step, top_cy, bump_r, 0, 2 * M_PI);
		cairo_fill(cr);
	}
}

/* Draw the callout overlay (dim + shape + text) on top of whatever is already
   on `cr`.
 *   ev     - event being previewed (provides label/time and per-event effect)
 *   fade   - overall opacity (0=invisible, 1=full)
 *   exit_t - 0 during fade-in/hold; eased 0→1 during fade-out for exit effects
 */
static void draw_callout(cairo_t *cr, t2g_t *root, t2g_t *ev,
                          double fade, double exit_t)
{
	if (fade <= 0.0) return;

	const char *label    = ev ? ev->label_text : "";
	const char *time_str = ev ? ev->time_text  : "";

	/* Resolve effect: per-event override beats global setting */
	const char *effect = (ev && ev->has_ev_callout_effect && ev->ev_callout_effect)
		? ev->ev_callout_effect : root->callout_effect;

	int    W  = root->width;
	int    H  = root->height;
	double cx = W * 0.5;
	double cy = H * 0.5;
	double bw = W * 0.62;
	double bh = H * 0.30;

	double fan_angle = 0.0;   /* non-zero only for the "fan" exit effect */

	/* ── Exit effect transform ────────────────────────────────────────── */
	if (exit_t > 0.0 && effect && strcmp(effect, "none") != 0) {
		double et = ease_in_cubic(exit_t);

		if (strcmp(effect, "funnel") == 0) {
			/* Contract toward the event dot, which always lands at 40 % of
			   canvas width (that's how render_camera_target is defined). */
			double dot_x = W * 0.40;
			double dot_y = (double)root->timeline_pos_y;
			cx = lerp(cx, dot_x, et);
			cy = lerp(cy, dot_y, et);
			bw = bw * (1.0 - et);
			bh = bh * (1.0 - et);

		} else if (strcmp(effect, "zoom") == 0) {
			/* Scale to zero at the centre */
			bw = bw * (1.0 - et);
			bh = bh * (1.0 - et);

		} else if (strcmp(effect, "float") == 0) {
			/* Drift upward and narrow slightly */
			cy -= bh * 0.60 * et;
			bw  = bw * (1.0 - et * 0.25);

		} else if (strcmp(effect, "fan") == 0) {
			/* Spin toward the dot while shrinking — the rotation is applied via
			   a Cairo transform after the dim overlay is drawn. */
			double dot_x = W * 0.40;
			double dot_y = (double)root->timeline_pos_y;
			cx = lerp(cx, dot_x, et);
			cy = lerp(cy, dot_y, et);
			bw = bw * (1.0 - et);
			bh = bh * (1.0 - et);
			fan_angle = exit_t * M_PI * 5.0;  /* 2.5 full rotations */
		}
	}

	/* Guard: never render a degenerate shape */
	if (bw < 2.0 || bh < 2.0) return;

	const char *shape = root->callout_shape;
	int is_cloud   = shape && strcmp(shape, "cloud")   == 0;
	int is_rounded = shape && strcmp(shape, "rounded") == 0;

	t2gcolor_t fill_col = root->has_callout_color
		? root->callout_color : root->theme_background;
	t2gcolor_t bord_col = root->has_callout_border
		? root->callout_border : root->theme_accent;
	t2gcolor_t text_col = root->theme_text;

	/* Dim the scene behind the callout; exit clears dim as shape leaves */
	cairo_rectangle(cr, 0, 0, W, H);
	cairo_set_source_rgba(cr, 0, 0, 0, 0.68 * fade * (1.0 - exit_t * 0.7));
	cairo_fill(cr);

	/* For the "fan" effect, rotate the shape+image+text around (cx, cy) */
	if (fan_angle != 0.0) {
		cairo_save(cr);
		cairo_translate(cr, cx, cy);
		cairo_rotate(cr, fan_angle);
		cairo_translate(cr, -cx, -cy);
	}

	/* ── Shape ── */
	if (is_cloud) {
		cairo_push_group(cr);
		set_color(cr, bord_col, 1.0);
		cloud_fill(cr, cx, cy, bw + 20, bh + 12);
		cairo_pop_group_to_source(cr);
		cairo_paint_with_alpha(cr, 0.22 * fade);

		cairo_push_group(cr);
		set_color(cr, fill_col, 1.0);
		cloud_fill(cr, cx, cy, bw, bh);
		cairo_pop_group_to_source(cr);
		cairo_paint_with_alpha(cr, 0.93 * fade);

	} else {
		double r = is_rounded ? 20.0 : 0.0;
		rounded_rect_path(cr, cx - bw * 0.5, cy - bh * 0.5, bw, bh, r);

		cairo_set_line_width(cr, 14.0);
		set_color(cr, bord_col, 0.12 * fade);
		cairo_stroke_preserve(cr);

		set_color(cr, fill_col, 0.93 * fade);
		cairo_fill_preserve(cr);

		cairo_set_line_width(cr, 1.5);
		set_color(cr, bord_col, 0.65 * fade);
		cairo_stroke(cr);
	}

	/* ── Image inside callout box (per-event override beats global) ── */
	const char *cimg_path = NULL;
	int cimg_size = 0;
	if (ev && ev->has_ev_callout_image && ev->ev_callout_image) {
		cimg_path = ev->ev_callout_image;
		cimg_size = ev->ev_callout_image_size;
	} else if (root->callout_image) {
		cimg_path = root->callout_image;
		cimg_size = root->callout_image_size;
	}

	double content_alpha = fade * clamp01(1.0 - exit_t * 2.5);

	int has_ctext = (label && *label) || (time_str && *time_str);

	if (cimg_path && content_alpha > 0.01) {
		/* When text follows below, use a smaller auto-size and shift upward */
		int sz_px = cimg_size > 0 ? cimg_size :
		            (has_ctext ? (int)(bh * 0.40) : (int)(bh * 0.62));
		double img_scale = (exit_t > 0.0) ? (bw / (root->width * 0.62)) : 1.0;
		if (img_scale < 0.01) img_scale = 0.01;
		double img_cy = has_ctext ? cy - bh * 0.12 : cy;
		cairo_push_group(cr);
		draw_image(cr, cimg_path, sz_px, cx, img_cy, img_scale, 1.0);
		cairo_pop_group_to_source(cr);
		cairo_paint_with_alpha(cr, content_alpha);
	}

	/* ── Text: below image when image present, centred otherwise ── */
	if (content_alpha > 0.01) {
		int desc_fs = t2g_get_description_font_size(root) + 4;
		int time_fs = t2g_get_time_font_size(root);

		double text_y, time_y;
		if (cimg_path && has_ctext) {
			/* Below the image — shift down into the lower portion of the box */
			text_y = cy + bh * 0.24 + (is_cloud ? bh * 0.04 : 0.0);
			time_y = text_y + desc_fs * 1.30;
		} else {
			text_y = cy + (is_cloud ? bh * 0.08 : 0.0)
			         - (double)(desc_fs + time_fs) * 0.35;
			time_y = text_y + desc_fs * 1.30;
		}

		draw_text(cr, label, t2g_get_description_font(root), desc_fs,
		          cx, text_y, text_col, content_alpha, W);
		draw_text(cr, time_str, t2g_get_time_font(root), time_fs,
		          cx, time_y, text_col, 0.75 * content_alpha, W);
	}

	/* Undo the fan rotation transform */
	if (fan_angle != 0.0)
		cairo_restore(cr);
}

/* ── Progress bar ───────────────────────────────────────────────────── */

static void draw_progress_bar(cairo_t *cr, t2g_t *root,
                               t2g_t *first_event,
                               int committed_count, int frame)
{
	if (!root->progress_show) return;

	int total = 0;
	for (t2g_t *e = first_event; e; e = e->next) total++;
	if (total == 0) return;

	double within = (frame >= 0)
		? ease_out_cubic((double)frame / (FRAMES_PER_ITEM - 1))
		: 0.0;
	double progress = clamp01(((double)committed_count + within) / total);

	int    bar_h   = root->progress_height > 0 ? root->progress_height : 4;
	double mx      = 12.0;
	double my      = 10.0;
	double bar_x   = root->split_show ? (double)root->split_width + mx : mx;
	double total_w = root->width - bar_x - mx;
	double bar_y   = root->height - my  - (double)bar_h;
	double fill_w  = total_w * progress;

	t2gcolor_t fill_col  = root->has_progress_color
		? root->progress_color : root->theme_accent;
	t2gcolor_t track_col = root->has_progress_background
		? root->progress_background : fill_col;

	/* Track */
	cairo_rectangle(cr, bar_x, bar_y, total_w, bar_h);
	set_color(cr, track_col, 0.12);
	cairo_fill(cr);

	if (fill_w < 1.0) return;

	/* Glow halo */
	cairo_rectangle(cr, bar_x, bar_y - 2.0, fill_w, (double)bar_h + 4.0);
	set_color(cr, fill_col, 0.20);
	cairo_fill(cr);

	/* Solid fill */
	cairo_rectangle(cr, bar_x, bar_y, fill_w, (double)bar_h);
	set_color(cr, fill_col, 0.88);
	cairo_fill(cr);

	/* Glowing leading-edge cap */
	double tip_x = bar_x + fill_w;
	double tip_y = bar_y + (double)bar_h * 0.5;
	double tip_r = (double)bar_h * 0.5 + 1.0;

	cairo_arc(cr, tip_x, tip_y, tip_r + 5.0, 0, 2.0 * M_PI);
	set_color(cr, fill_col, 0.10);
	cairo_fill(cr);

	cairo_arc(cr, tip_x, tip_y, tip_r + 2.5, 0, 2.0 * M_PI);
	set_color(cr, fill_col, 0.25);
	cairo_fill(cr);

	cairo_arc(cr, tip_x, tip_y, tip_r, 0, 2.0 * M_PI);
	set_color(cr, fill_col, 1.0);
	cairo_fill(cr);
}

/* ── Shared render body ─────────────────────────────────────────────── */

/* ── Split-screen panel ─────────────────────────────────────────────── */

/* Draw a single panel text line, left-aligned, single-paragraph, ellipsized. */
static void panel_line(cairo_t *cr, const char *text,
                        const char *font, int fsize,
                        double x, double y_top, int max_w,
                        t2gcolor_t col, double alpha)
{
	if (!text || !*text || alpha <= 0.0) return;
	PangoLayout *pl = pango_cairo_create_layout(cr);
	char fd[256];
	snprintf(fd, sizeof(fd), "%s %d", font, fsize);
	PangoFontDescription *pfd = pango_font_description_from_string(fd);
	pango_layout_set_font_description(pl, pfd);
	pango_font_description_free(pfd);
	pango_layout_set_text(pl, text, -1);
	pango_layout_set_alignment(pl, PANGO_ALIGN_LEFT);
	pango_layout_set_width(pl, max_w * PANGO_SCALE);
	pango_layout_set_ellipsize(pl, PANGO_ELLIPSIZE_END);
	pango_layout_set_single_paragraph_mode(pl, TRUE);
	set_color(cr, col, alpha);
	cairo_move_to(cr, x, y_top);
	pango_cairo_show_layout(cr, pl);
	g_object_unref(pl);
}

/* Returns pixel height of a single text line at the given font/size. */
static int panel_line_height(cairo_t *cr, const char *font, int fsize)
{
	PangoLayout *pl = pango_cairo_create_layout(cr);
	char fd[256];
	snprintf(fd, sizeof(fd), "%s %d", font, fsize);
	PangoFontDescription *pfd = pango_font_description_from_string(fd);
	pango_layout_set_font_description(pl, pfd);
	pango_font_description_free(pfd);
	pango_layout_set_text(pl, "Ag", -1);
	int lw, lh;
	pango_layout_get_pixel_size(pl, &lw, &lh);
	g_object_unref(pl);
	return lh;
}

/*
 * Left panel: bullet list of all events.
 *   committed_count  — events that have fully animated in
 *   current_index    — event currently animating (-1 = transit, no highlight)
 */
static void draw_split_panel(cairo_t *cr, t2g_t *root, t2g_t *first_event,
                              int committed_count, int current_index)
{
	int pw = root->split_width;
	int ch = root->height;

	/* Count events */
	int n = 0;
	for (t2g_t *e = first_event; e; e = e->next) n++;
	if (n == 0) return;

	/* Panel background */
	t2gcolor_t bg1, bg2;
	resolve_background(root, first_event, committed_count, &bg1, &bg2);
	t2gcolor_t panel_bg = root->has_split_bg ? root->split_bg : bg1;
	cairo_set_source_rgba(cr, panel_bg.r/255.0, panel_bg.g/255.0,
	                          panel_bg.b/255.0, 1.0);
	cairo_rectangle(cr, 0, 0, pw, ch);
	cairo_fill(cr);
	if (!root->has_split_bg) {
		/* slight overlay to distinguish panel from the main canvas */
		cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.05);
		cairo_rectangle(cr, 0, 0, pw, ch);
		cairo_fill(cr);
	}

	/* Right separator */
	t2gcolor_t accent = root->theme_accent;
	cairo_set_source_rgba(cr, accent.r/255.0, accent.g/255.0,
	                          accent.b/255.0, 0.18);
	cairo_set_line_width(cr, 1.0);
	cairo_move_to(cr, pw - 0.5, 0);
	cairo_line_to(cr, pw - 0.5, ch);
	cairo_stroke(cr);

	/* Row layout */
	int top = 28, bot = 16;
	int avail = ch - top - bot;
	int row_h = avail / n;
	if (row_h < 20) row_h = 20;
	if (row_h > 50) row_h = 50;
	int total_list_h = row_h * n;
	int start_y = top + (avail - total_list_h) / 2;
	if (start_y < top) start_y = top;

	int bx      = 14;          /* bullet circle x center */
	int tx      = 27;          /* text start x */
	int text_w  = pw - tx - 8; /* available text width */

	/* Font sizes: scale down for dense event lists */
	int fsize   = (row_h >= 34) ? 11 : (row_h >= 27) ? 10 : 9;
	int time_fs = fsize - 1;
	int two_line = (row_h >= 30);  /* show time + label on separate lines */

	const char *font = t2g_get_description_font(root);
	int lh_label = panel_line_height(cr, font, fsize);
	int lh_time  = two_line ? panel_line_height(cr, font, time_fs) : 0;

	t2g_t *ev = first_event;
	for (int i = 0; i < n && ev; i++, ev = ev->next) {
		int row_top = start_y + i * row_h;
		int row_cy  = row_top + row_h / 2;

		int is_done    = (i < committed_count);
		int is_current = (i == current_index);

		double text_a   = is_current ? 1.0 : (is_done ? 0.72 : 0.24);
		double bullet_a = is_current ? 1.0 : (is_done ? 0.55 : 0.18);
		double bullet_r = is_current ? 5.5 : 4.0;

		/* Left accent bar for the currently animating event */
		if (is_current) {
			cairo_set_source_rgba(cr, accent.r/255.0, accent.g/255.0,
			                          accent.b/255.0, 0.65);
			cairo_rectangle(cr, 0, row_top, 3, row_h);
			cairo_fill(cr);
		}

		/* Glow behind the bullet for the current event */
		if (is_current) {
			double ar = accent.r/255.0, ag = accent.g/255.0, ab = accent.b/255.0;
			cairo_pattern_t *glow = cairo_pattern_create_radial(
				bx, row_cy, 0, bx, row_cy, bullet_r * 3.2);
			cairo_pattern_add_color_stop_rgba(glow, 0.0, ar, ag, ab, 0.40);
			cairo_pattern_add_color_stop_rgba(glow, 1.0, ar, ag, ab, 0.0);
			cairo_set_source(cr, glow);
			cairo_arc(cr, bx, row_cy, bullet_r * 3.2, 0, 2*M_PI);
			cairo_fill(cr);
			cairo_pattern_destroy(glow);
		}

		/* Bullet: filled circle (done/current) or outline (future) */
		t2gcolor_t dc = ev->has_dot_color ? ev->dot_color : accent;
		if (!is_done && !is_current) {
			cairo_set_source_rgba(cr, dc.r/255.0, dc.g/255.0,
			                          dc.b/255.0, bullet_a);
			cairo_set_line_width(cr, 1.0);
			cairo_arc(cr, bx, row_cy, bullet_r - 0.5, 0, 2*M_PI);
			cairo_stroke(cr);
		} else {
			cairo_set_source_rgba(cr, dc.r/255.0, dc.g/255.0,
			                          dc.b/255.0, bullet_a);
			cairo_arc(cr, bx, row_cy, bullet_r, 0, 2*M_PI);
			cairo_fill(cr);
		}

		/* Text: two lines (time above, label below) when row is tall enough */
		t2gcolor_t tc = text_color(root, ev);
		if (two_line && ev->time_text && *ev->time_text) {
			int block_h = lh_time + 2 + lh_label;
			double ty   = row_cy - block_h / 2.0;
			panel_line(cr, ev->time_text, font, time_fs,
			           tx, ty, text_w, accent, text_a * 0.65);
			panel_line(cr, ev->label_text, font, fsize,
			           tx, ty + lh_time + 2, text_w, tc, text_a);
		} else {
			panel_line(cr, ev->label_text, font, fsize,
			           tx, row_cy - lh_label / 2.0, text_w, tc, text_a);
		}
	}
}

/* ── Shared render body ─────────────────────────────────────────────── */

static void render_body(cairo_t *cr, t2g_t *root,
                         t2g_t *first_event,
                         int committed_count, int current_index,
                         int frame,           /* -1 = transit (no animating event) */
                         double camera_x)
{
	/* Resolve background from the last committed event that set one */
	t2gcolor_t bg1, bg2;
	resolve_background(root, first_event, committed_count, &bg1, &bg2);

	/* Advance to the event currently animating in (if any). */
	t2g_t *animev = NULL;
	if (frame >= 0) {
		animev = first_event;
		for (int i = 0; i < committed_count && animev; i++, animev = animev->next)
			;
	}

	/* If the animating event changes the background, fade it in */
	if (animev && animev->has_ev_background) {
		double prog = (double)frame / (FRAMES_PER_ITEM - 1);
		double fade = ease_out_cubic(clamp01((prog - 0.1) / 0.7));
		bg1 = lerp_color(bg1, animev->ev_background,  fade);
		bg2 = lerp_color(bg2, animev->ev_background2, fade);
	}

	draw_background_colors(cr, root, bg1, bg2);

	/* Spring drop: while the animating event falls and lands, deform the line
	   locally around its x. The deformation is confined to the drop zone via
	   line_disp()'s bell falloff; the rest of the line stays straight. */
	double disp_cx = 0, disp_amp = 0, disp_sigma = 1;
	if (animev && t2g_event_drop(root, animev)) {
		double prog    = (double)frame / (FRAMES_PER_ITEM - 1);
		double fall_h  = t2g_event_drop_amount(root, animev);
		double dot_dy, line_amp;
		spring_drop(prog, fall_h, fall_h * 0.30, &dot_dy, &line_amp);
		disp_amp   = line_amp;
		disp_cx    = render_event_world_x(root, animev, committed_count) - camera_x;
		disp_sigma = root->item_spacing * 0.5;
		if (disp_sigma < 55)  disp_sigma = 55;
		if (disp_sigma > 130) disp_sigma = 130;
	}

	draw_timeline_line(cr, root, first_event, committed_count, camera_x,
	                   disp_cx, disp_amp, disp_sigma);

	/* Pre-compute non-overlapping label positions for all visible events */
	int total = committed_count + (frame >= 0 ? 1 : 0);
	double label_cx[MAX_LABEL_EVENTS];
	double half_w[MAX_LABEL_EVENTS];
	compute_label_cx(cr, root, first_event, total, camera_x, label_cx, half_w);

	/* Dim previously committed events while a new one is animating in */
	double committed_alpha = (frame >= 0) ? 0.4 : 1.0;

	/* Draw committed events */
	t2g_t *iter = first_event;
	for (int i = 0; i < committed_count && iter; i++, iter = iter->next)
		draw_committed_event(cr, root, iter, i, camera_x, label_cx[i], committed_alpha);

	/* Draw animating event (if not a transit frame) */
	if (frame >= 0 && iter)
		draw_animating_event(cr, root, iter, current_index, frame, camera_x,
		                     label_cx[committed_count]);

	draw_progress_bar(cr, root, first_event, committed_count, frame);

	if (root->split_show)
		draw_split_panel(cr, root, first_event, committed_count, current_index);
}

/* ── Public API ─────────────────────────────────────────────────────── */

cairo_surface_t *render_frame(t2g_t *root,
                               t2g_t *first_event,
                               int    committed_count,
                               int    current_index,
                               int    frame,
                               double camera_x_start,
                               double camera_x_end)
{
	cairo_surface_t *surface =
		cairo_image_surface_create(CAIRO_FORMAT_ARGB32, root->width, root->height);
	cairo_t *cr = cairo_create(surface);

	double cam_t    = ease_in_out_cubic((double)frame / (FRAMES_PER_ITEM - 1));
	double camera_x = root->camera_scroll
		? lerp(camera_x_start, camera_x_end, cam_t)
		: 0.0;

	render_body(cr, root, first_event, committed_count, current_index,
	            frame, camera_x);

	cairo_destroy(cr);
	return surface;
}

cairo_surface_t *render_transit_frame(t2g_t *root,
                                       t2g_t *first_event,
                                       int    committed_count,
                                       double camera_x)
{
	cairo_surface_t *surface =
		cairo_image_surface_create(CAIRO_FORMAT_ARGB32, root->width, root->height);
	cairo_t *cr = cairo_create(surface);

	render_body(cr, root, first_event, committed_count, -1, -1, camera_x);

	cairo_destroy(cr);
	return surface;
}

cairo_surface_t *render_callout_frame(t2g_t *root,
                                       t2g_t *first_event,
                                       int    committed_count,
                                       double camera_x,
                                       t2g_t *ev,
                                       double fade,
                                       double exit_t)
{
	cairo_surface_t *surface =
		cairo_image_surface_create(CAIRO_FORMAT_ARGB32, root->width, root->height);
	cairo_t *cr = cairo_create(surface);

	/* Highlight the upcoming event in the split panel during callout */
	render_body(cr, root, first_event, committed_count, committed_count, -1, camera_x);
	draw_callout(cr, root, ev, fade, exit_t);

	cairo_destroy(cr);
	return surface;
}

/* ── Transition compositing ─────────────────────────────────────────── */

#define MAX_DISSOLVE_BLOCKS 65536
#define DEFAULT_DISSOLVE_BLOCK_PX 8

static int   _dissolve_order[MAX_DISSOLVE_BLOCKS];
static int   _dissolve_n = 0;

static void dissolve_ensure_order(int n)
{
	if (_dissolve_n == n) return;
	for (int i = 0; i < n; i++) _dissolve_order[i] = i;
	uint32_t lcg = 0xDEADBEEFu;
	for (int i = n - 1; i > 0; i--) {
		lcg = lcg * 1664525u + 1013904223u;
		int j = (int)((uint32_t)(lcg >> 1) % (uint32_t)(i + 1));
		int tmp = _dissolve_order[i];
		_dissolve_order[i] = _dissolve_order[j];
		_dissolve_order[j] = tmp;
	}
	_dissolve_n = n;
}

static void paint_dissolve_blocks(cairo_t *cr, cairo_surface_t *to,
                                   double t, int block_px, int w, int h)
{
	if (block_px <= 0) block_px = DEFAULT_DISSOLVE_BLOCK_PX;
	int bx = (w + block_px - 1) / block_px;
	int by = (h + block_px - 1) / block_px;
	int total = bx * by;
	if (total > MAX_DISSOLVE_BLOCKS) total = MAX_DISSOLVE_BLOCKS;
	int draw = (int)(t * total);
	dissolve_ensure_order(total);
	for (int k = 0; k < draw; k++) {
		int b = _dissolve_order[k];
		cairo_save(cr);
		cairo_rectangle(cr, (b % bx) * block_px,
		                    (b / bx) * block_px,
		                    block_px, block_px);
		cairo_clip(cr);
		cairo_set_source_surface(cr, to, 0, 0);
		cairo_paint(cr);
		cairo_restore(cr);
	}
}

cairo_surface_t *render_transition_frame(cairo_surface_t *from,
                                          cairo_surface_t *to,
                                          double t,
                                          const char *style,
                                          int block_size,
                                          int width, int height)
{
	cairo_surface_t *out =
		cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
	cairo_t *cr = cairo_create(out);

	int is_wipe     = style && strcmp(style, "wipe")     == 0;
	int is_dissolve = style && (strcmp(style, "dissolve") == 0 ||
	                             strcmp(style, "pixelize") == 0);

	if (is_wipe) {
		cairo_set_source_surface(cr, from, 0, 0); cairo_paint(cr);
		cairo_rectangle(cr, 0, 0, width * ease_in_out_cubic(t), height);
		cairo_clip(cr);
		cairo_set_source_surface(cr, to, 0, 0); cairo_paint(cr);
	} else if (is_dissolve) {
		cairo_set_source_surface(cr, from, 0, 0); cairo_paint(cr);
		paint_dissolve_blocks(cr, to, ease_in_out_cubic(t),
		                      block_size, width, height);
	} else {
		cairo_set_source_surface(cr, from, 0, 0); cairo_paint(cr);
		cairo_set_source_surface(cr, to, 0, 0);
		cairo_paint_with_alpha(cr, ease_in_out_cubic(t));
	}

	cairo_destroy(cr);
	return out;
}
