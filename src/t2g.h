#ifndef _T2G_H_
#define _T2G_H_

#include <stdio.h>
#include <stdint.h>

/* resolved at runtime; see t2g_find_default_font() */

struct _t2gcolor_t {
	unsigned char a;
	unsigned char r;
	unsigned char g;
	unsigned char b;
};
typedef struct _t2gcolor_t t2gcolor_t;

struct _t2g_t {
	/* Canvas */
	int width;
	int height;

	/* Timeline line */
	int timeline_pos_y;
	t2gcolor_t timeline_color;

	/* Theme */
	t2gcolor_t theme_background;   /* gradient top */
	t2gcolor_t theme_background2;  /* gradient bottom */
	t2gcolor_t theme_accent;       /* dots, line, connector */
	t2gcolor_t theme_text;         /* label and time text */

	/* Fonts (Pango family names, e.g. "Sans" or "Arial") */
	char *time_font;
	int   time_font_size;
	char *description_font;
	int   description_font_size;

	/* Animation speed (centiseconds) */
	int speed_nextitem;
	int speed_frames;
	int speed_loop_pause;  /* extra hold on the very last frame before looping (0 = none) */

	/* Layout */
	int item_spacing;   /* pixels between event x positions (world space) */

	/* Camera */
	int camera_scroll;  /* boolean: pan to reveal each event */

	/* Automatic x-positioning from parsed time strings.
	   When time_format is set, event.x is derived from the event's time
	   string rather than sequential index.
	     time.format year    — extract a 4-digit year  (e.g. "Jan 1990" → 1990)
	     time.format number  — parse the first number  (e.g. "Phase 3" → 3)
	   time.origin (default 0 = auto from first event): the time value that
	   maps to world x = item_spacing (i.e. where the first event sits). */
	char  *time_format;
	int    has_time_origin;
	double time_origin;

	/* Transition between events */
	char *transition_style;    /* "none", "fade", "wipe", "dissolve" */
	int   transition_frames;   /* number of transition frames (default 8) */
	int   transition_block_size; /* dissolve block size in pixels (0 = default 8) */

	/* Callout overlay — spotlight shown before each event joins the timeline */
	char      *callout_shape;        /* "rectangle" | "rounded" | "cloud" | NULL=off */
	char      *callout_effect;       /* exit animation: "none" | "funnel" | "zoom" | "float" */
	int        callout_pause;        /* hold duration in centiseconds (0 → 200) */
	int        has_callout_color;
	t2gcolor_t callout_color;        /* box fill */
	int        has_callout_border;
	t2gcolor_t callout_border;       /* border / glow color */

	/* Per-event callout effect override */
	int        has_ev_callout_effect;
	char      *ev_callout_effect;

	/* Progress bar overlay */
	int        progress_show;         /* boolean */
	int        has_progress_color;
	t2gcolor_t progress_color;        /* fill / foreground */
	int        has_progress_background;
	t2gcolor_t progress_background;   /* track / unfilled */
	int        progress_height;       /* bar thickness in px (0 → default 4) */

	/* Output */
	char *output_format;  /* "gif", "webp", "apng" */

	/* Per-event text */
	char *time_text;
	char *label_text;
	t2gcolor_t mark_color;

	/* Per-event color overrides (has_* = 0 → use theme defaults) */
	int        has_dot_color;
	t2gcolor_t dot_color;
	int        has_text_color;
	t2gcolor_t text_color;
	int        has_line_color;
	t2gcolor_t line_color;
	/* Color of the main timeline line segment leading into this event */
	int        has_timeline_color;
	t2gcolor_t ev_timeline_color;

	/* Background gradient override from this event onward (persists until
	   another event sets a new background). */
	int        has_ev_background;
	t2gcolor_t ev_background;    /* gradient top    */
	t2gcolor_t ev_background2;   /* gradient bottom */

	/* Per-event world-space x position (0 = use sequential default) */
	int x_pos;

	/* Per-event image to draw instead of the standard dot */
	char *dot_image;       /* path to PNG or SVG file */
	int   dot_image_size;  /* icon diameter in pixels (0 = use DOT_RADIUS*4) */

	struct _t2g_t *root;
	struct _t2g_t *next;
};
typedef struct _t2g_t t2g_t;

/* Parser interface (see parser.y / lexer.l) */
void t2g_parser_init(void);
void t2g_parser_terminate(void);
void t2grestart(FILE *);

t2g_t *t2g_new(void);
int    t2g_append(t2g_t *t2g_root, t2g_t *next);
void   t2g_free(t2g_t *timeline);

/* Set the base directory used to resolve relative paths in a .tig file.
   Call with argv[1] (the .tig path) before parsing. */
void        t2g_set_basedir(const char *tig_path);
const char *t2g_get_basedir(void);

char *t2g_find_default_font(void);
char *t2g_get_description_font(t2g_t *t2g);
int   t2g_get_description_font_size(t2g_t *t2g);
char *t2g_get_time_font(t2g_t *t2g);
int   t2g_get_time_font_size(t2g_t *t2g);

#endif /* _T2G_H_ */
