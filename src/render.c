#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#include <cairo/cairo.h>
#include <pango/pangocairo.h>
#include <librsvg/rsvg.h>

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

double render_event_world_x(t2g_t *root, t2g_t *ev, int index)
{
	if (ev && ev->x_pos > 0)
		return (double)ev->x_pos;
	return (double)(index + 1) * root->item_spacing;
}

double render_camera_target(t2g_t *root, t2g_t *ev, int index)
{
	/* Place the event at 40 % from left; allow negative camera_x. */
	return render_event_world_x(root, ev, index) - root->width * 0.4;
}

/* Above (dir=-1) for even indices, below (+1) for odd. */
static int connector_dir(int index)
{
	return (index % 2 == 0) ? -1 : 1;
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

/* ── Image rendering (PNG + SVG) ────────────────────────────────────── */

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

	if (ext && strcasecmp(ext, ".svg") == 0) {
		GError     *gerr   = NULL;
		RsvgHandle *handle = rsvg_handle_new_from_file(path, &gerr);
		if (!handle) {
			if (gerr) {
				fprintf(stderr, "SVG load error (%s): %s\n", path, gerr->message);
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
	} else {
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

static void draw_timeline_line(cairo_t *cr, t2g_t *root,
                                t2g_t *first_event, int committed_count,
                                double camera_x)
{
	double ty = root->timeline_pos_y;
	double w  = root->width;

	/* Glow */
	cairo_set_line_width(cr, 8);
	set_color(cr, root->theme_accent, 0.12);
	cairo_move_to(cr, 0, ty); cairo_line_to(cr, w, ty);
	cairo_stroke(cr);

	/* Main line */
	cairo_set_line_width(cr, 2);
	set_color(cr, root->theme_accent, 0.85);
	cairo_move_to(cr, 0, ty); cairo_line_to(cr, w, ty);
	cairo_stroke(cr);

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
		cairo_move_to(cr, sx, ty - 4); cairo_line_to(cr, sx, ty + 4);
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
					cairo_move_to(cr, sx1, ty);
					cairo_line_to(cr, sx2, ty);
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
		cairo_move_to(cr, ex, ty - 9); cairo_line_to(cr, ex, ty + 9);
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

/* ── Draw one fully-committed event ─────────────────────────────────── */

static void draw_committed_event(cairo_t *cr, t2g_t *root, t2g_t *ev,
                                  int index, double camera_x,
                                  double label_cx)
{
	double wx    = render_event_world_x(root, ev, index);
	double sx    = wx - camera_x;
	double ty    = root->timeline_pos_y;
	int    dir   = connector_dir(index);

	if (sx < -DEFAULT_ICON_SIZE || sx > root->width + DEFAULT_ICON_SIZE) return;

	double shelf_y  = ty + dir * CONNECTOR_H;   /* top of vertical connector */
	double label_y  = shelf_y + dir * 14;        /* label centre y */
	double time_y   = ty - dir * 20;             /* time string, opposite side */

	t2gcolor_t lc = line_color(root, ev);
	t2gcolor_t tc = text_color(root, ev);

	/* L-shaped connector */
	draw_connector(cr, sx, label_cx, ty + dir * DOT_RADIUS, shelf_y, lc, 1.0);

	/* Dot / icon */
	draw_dot_element(cr, root, ev, sx, ty, 1.0, 1.0);

	/* Label (at pushed position) */
	draw_text(cr, ev->label_text,
	          t2g_get_description_font(ev),
	          t2g_get_description_font_size(ev),
	          label_cx, label_y, tc, 1.0, root->width);

	/* Time (always near the dot, below the timeline) */
	draw_text(cr, ev->time_text,
	          t2g_get_time_font(ev),
	          t2g_get_time_font_size(ev),
	          sx, time_y, tc, 0.65, root->width);
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

	double shelf_y = ty + dir * CONNECTOR_H;
	double label_y = shelf_y + dir * 14;
	double time_y  = ty - dir * 20;

	double dot_t     = ease_out_elastic(clamp01(prog * 2.2));
	double line_frac = ease_out_cubic(clamp01((prog - 0.20) / 0.45));
	double text_a    = ease_out_cubic(clamp01((prog - 0.62) / 0.38));

	t2gcolor_t lc = line_color(root, ev);
	t2gcolor_t tc = text_color(root, ev);

	/* Pulse ring */
	if (dot_t > 0.05 && dot_t < 1.2 && !ev->dot_image) {
		double pulse = dot_t > 1.0 ? 2.0 - dot_t : dot_t;
		cairo_arc(cr, sx, ty, DOT_RADIUS + 8 * dot_t, 0, 2 * M_PI);
		cairo_set_line_width(cr, 1.5);
		set_color(cr, dot_color(root, ev), 0.4 * (1.0 - pulse * 0.7));
		cairo_stroke(cr);
	}

	/* L-connector grows in: first vertical, then horizontal */
	if (line_frac > 0.01) {
		double grown_shelf = ty + dir * (CONNECTOR_H * line_frac);
		double horiz_cx    = lerp(sx, label_cx, clamp01((line_frac - 0.7) / 0.3));
		draw_connector(cr, sx, horiz_cx,
		               ty + dir * DOT_RADIUS, grown_shelf, lc, line_frac);
	}

	/* Dot / icon */
	draw_dot_element(cr, root, ev, sx, ty, dot_t, dot_t);

	/* Text fades in at final position */
	if (text_a > 0.01) {
		draw_text(cr, ev->label_text,
		          t2g_get_description_font(ev),
		          t2g_get_description_font_size(ev),
		          label_cx, label_y, tc, text_a, root->width);

		draw_text(cr, ev->time_text,
		          t2g_get_time_font(ev),
		          t2g_get_time_font_size(ev),
		          sx, time_y, tc, text_a * 0.65, root->width);
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

	/* If the animating event changes the background, fade it in */
	if (frame >= 0) {
		double prog = (double)frame / (FRAMES_PER_ITEM - 1);
		/* advance to the animating event */
		t2g_t *animev = first_event;
		for (int i = 0; i < committed_count && animev; i++, animev = animev->next)
			;
		if (animev && animev->has_ev_background) {
			double fade = ease_out_cubic(clamp01((prog - 0.1) / 0.7));
			bg1 = lerp_color(bg1, animev->ev_background,  fade);
			bg2 = lerp_color(bg2, animev->ev_background2, fade);
		}
	}

	draw_background_colors(cr, root, bg1, bg2);
	draw_timeline_line(cr, root, first_event, committed_count, camera_x);

	/* Pre-compute non-overlapping label positions for all visible events */
	int total = committed_count + (frame >= 0 ? 1 : 0);
	double label_cx[MAX_LABEL_EVENTS];
	double half_w[MAX_LABEL_EVENTS];
	compute_label_cx(cr, root, first_event, total, camera_x, label_cx, half_w);

	/* Draw committed events */
	t2g_t *iter = first_event;
	for (int i = 0; i < committed_count && iter; i++, iter = iter->next)
		draw_committed_event(cr, root, iter, i, camera_x, label_cx[i]);

	/* Draw animating event (if not a transit frame) */
	if (frame >= 0 && iter)
		draw_animating_event(cr, root, iter, current_index, frame, camera_x,
		                     label_cx[committed_count]);
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

/* ── Transition compositing ─────────────────────────────────────────── */

#define MAX_DISSOLVE_BLOCKS 32768
#define DISSOLVE_BLOCK_PX   20

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
                                   double t, int w, int h)
{
	int bx = (w + DISSOLVE_BLOCK_PX - 1) / DISSOLVE_BLOCK_PX;
	int by = (h + DISSOLVE_BLOCK_PX - 1) / DISSOLVE_BLOCK_PX;
	int total = bx * by;
	if (total > MAX_DISSOLVE_BLOCKS) total = MAX_DISSOLVE_BLOCKS;
	int draw = (int)(t * total);
	dissolve_ensure_order(total);
	for (int k = 0; k < draw; k++) {
		int b = _dissolve_order[k];
		cairo_save(cr);
		cairo_rectangle(cr, (b % bx) * DISSOLVE_BLOCK_PX,
		                    (b / bx) * DISSOLVE_BLOCK_PX,
		                    DISSOLVE_BLOCK_PX, DISSOLVE_BLOCK_PX);
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
                                          int width, int height)
{
	cairo_surface_t *out =
		cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
	cairo_t *cr = cairo_create(out);

	int is_wipe     = style && strcmp(style, "wipe")     == 0;
	int is_dissolve = style && strcmp(style, "dissolve") == 0;

	if (is_wipe) {
		cairo_set_source_surface(cr, from, 0, 0); cairo_paint(cr);
		cairo_rectangle(cr, 0, 0, width * ease_in_out_cubic(t), height);
		cairo_clip(cr);
		cairo_set_source_surface(cr, to, 0, 0); cairo_paint(cr);
	} else if (is_dissolve) {
		cairo_set_source_surface(cr, from, 0, 0); cairo_paint(cr);
		paint_dissolve_blocks(cr, to, ease_in_out_cubic(t), width, height);
	} else {
		cairo_set_source_surface(cr, from, 0, 0); cairo_paint(cr);
		cairo_set_source_surface(cr, to, 0, 0);
		cairo_paint_with_alpha(cr, ease_in_out_cubic(t));
	}

	cairo_destroy(cr);
	return out;
}
