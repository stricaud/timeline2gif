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
	int has_speed_pause;
	int speed_pause;       /* global per-event hold; overrides speed_nextitem; overridden by event.pause */

	/* Split-screen: left panel lists all events, right panel shows animation */
	int split_show;
	int split_width;       /* left panel width in pixels (default 260) */
	int has_split_bg;
	t2gcolor_t split_bg;   /* panel fill color (default: theme.background) */

	/* Layout */
	int item_spacing;   /* pixels between event x positions (world space) */

	/* Camera */
	int camera_scroll;  /* boolean: pan to reveal each event */

	/* Timeline "drop" wobble: when a new event lands, the line dips and
	   oscillates like a heavy object was dropped on it, then settles. */
	int timeline_drop;        /* boolean: enable the effect */
	int timeline_drop_amount; /* peak vertical displacement in px (default 12) */

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
	char      *callout_image;        /* default image shown in all callout boxes */
	int        callout_image_size;   /* 0 = auto-fit to box height */

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

	/* Per-event hold duration override (centiseconds; 0 = use speed.nextitem) */
	int has_event_pause;
	int event_pause;

	/* Per-event "heavy drop" wobble override of timeline.drop.
	   has_ev_drop=0 → inherit global; otherwise ev_drop is the on/off value.
	   ev_drop_amount=0 → inherit global timeline.drop_amount. */
	int has_ev_drop;
	int ev_drop;
	int ev_drop_amount;

	/* Per-event image to draw instead of the standard dot */
	char *dot_image;       /* path to PNG or SVG file */
	int   dot_image_size;  /* icon diameter in pixels (0 = use DOT_RADIUS*4) */

	/* Per-event callout image override */
	int   has_ev_callout_image;
	char *ev_callout_image;       /* image shown inside the callout box */
	int   ev_callout_image_size;  /* 0 = auto-fit to box height */

	/* Per-event label image (shown at the label position) */
	char *label_image;
	int   label_image_size;       /* 0 = DEFAULT_LABEL_IMAGE_SIZE */

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
void        t2g_set_basedir_dir(const char *dir);
const char *t2g_get_basedir(void);

/* Named inline SVG images declared with `define.svg name << … `.
   t2g_lookup_svg returns the stored markup, or NULL if no such name. */
void        t2g_define_svg(const char *name, const char *data);
const char *t2g_lookup_svg(const char *name);
void        t2g_clear_svg_defs(void);

char *t2g_find_default_font(void);
char *t2g_get_description_font(t2g_t *t2g);
int   t2g_get_description_font_size(t2g_t *t2g);
char *t2g_get_time_font(t2g_t *t2g);
int   t2g_get_time_font_size(t2g_t *t2g);

/* Resolve the "heavy drop" wobble for the event currently animating in,
   honouring a per-event event.drop / event.drop_amount override and
   falling back to the global timeline.drop / timeline.drop_amount. */
int   t2g_event_drop(t2g_t *root, t2g_t *ev);
int   t2g_event_drop_amount(t2g_t *root, t2g_t *ev);

#endif /* _T2G_H_ */
