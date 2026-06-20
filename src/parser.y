%define api.prefix {t2g}
%parse-param {t2g_t *timeline}

%{
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "t2g.h"
#include <parser.h>

int t2glex(void);
extern void t2gerror(t2g_t *t2g, const char *p);
char *t2gget_text(void);
int   t2gget_lineno(void);

/* ── Pending per-event settings ───────────────────────────────────────
   Accumulated before a "time" "label" pair; applied when the item is
   created, then reset.                                               */
static struct {
	int        has_dot_color, has_text_color, has_line_color, has_timeline_color;
	t2gcolor_t dot_color, text_color, line_color, timeline_color;
	int        has_ev_background;
	t2gcolor_t ev_background, ev_background2;
	int        x_pos;
	int        has_event_pause;
	int        event_pause;
	int        has_drop;       /* per-event timeline.drop override */
	int        drop;
	int        drop_amount;    /* per-event drop_amount override (0 = inherit) */
	char      *dot_image;
	int        dot_image_size;
	char      *callout_effect;   /* per-event callout exit effect override */
	char      *callout_image;    /* per-event callout image override */
	int        callout_image_size;
	char      *label_image;      /* per-event label area image */
	int        label_image_size;
} pending;

static void pending_reset(void)
{
	free(pending.dot_image);
	free(pending.callout_effect);
	free(pending.callout_image);
	free(pending.label_image);
	memset(&pending, 0, sizeof(pending));
}

static void pending_apply(t2g_t *ev)
{
	if (pending.has_dot_color)      { ev->has_dot_color      = 1; ev->dot_color      = pending.dot_color; }
	if (pending.has_text_color)     { ev->has_text_color     = 1; ev->text_color     = pending.text_color; }
	if (pending.has_line_color)     { ev->has_line_color     = 1; ev->line_color     = pending.line_color; }
	if (pending.has_timeline_color) { ev->has_timeline_color = 1; ev->ev_timeline_color = pending.timeline_color; }
	if (pending.has_ev_background)  {
		ev->has_ev_background = 1;
		ev->ev_background  = pending.ev_background;
		ev->ev_background2 = pending.ev_background2;
	}
	if (pending.x_pos)            { ev->x_pos               = pending.x_pos; }
	if (pending.has_event_pause)  { ev->has_event_pause = 1; ev->event_pause = pending.event_pause; }
	if (pending.has_drop)         { ev->has_ev_drop = 1; ev->ev_drop = pending.drop; }
	if (pending.drop_amount)      { ev->ev_drop_amount = pending.drop_amount; }
	if (pending.dot_image)        { ev->dot_image            = pending.dot_image; pending.dot_image = NULL; }
	if (pending.dot_image_size)   { ev->dot_image_size       = pending.dot_image_size; }
	if (pending.callout_effect)   {
		ev->has_ev_callout_effect = 1;
		ev->ev_callout_effect     = pending.callout_effect;
		pending.callout_effect    = NULL;
	}
	if (pending.callout_image)    {
		ev->has_ev_callout_image  = 1;
		ev->ev_callout_image      = pending.callout_image;
		pending.callout_image     = NULL;
	}
	if (pending.callout_image_size) ev->ev_callout_image_size = pending.callout_image_size;
	if (pending.label_image)      {
		ev->label_image           = pending.label_image;
		pending.label_image       = NULL;
	}
	if (pending.label_image_size) ev->label_image_size = pending.label_image_size;
}

/* ── Color parser ─────────────────────────────────────────────────── */
static t2gcolor_t parse_argb(const char *str)
{
	t2gcolor_t c = {0, 0, 0, 0};
	int a, r, g, b;
	if (sscanf(str, "argb(%d,%d,%d,%d)", &a, &r, &g, &b) != 4)
		return c;
	c.a = (unsigned char)a;
	c.r = (unsigned char)r;
	c.g = (unsigned char)g;
	c.b = (unsigned char)b;
	return c;
}

%}

%union {
	char *string;
	int   integer;
}

%token <string>  TOK_STRING
%token <integer> TOK_INTEGER
%token <string>  TOK_WORD
%token           TOK_DOT
%token <string>  TOK_ARGB

%type <string> str_val

%start input
%%

input:
	  /* empty */
	| input setting_str
	| input setting_argb
	| input setting_int
	| input item
	;

/* Allow both bare words and quoted strings as setting values */
str_val:
	  TOK_WORD   { $$ = $1; }
	| TOK_STRING { $$ = $1; }
	;

/* ── String settings ─────────────────────────────────────── */
setting_str: TOK_WORD TOK_DOT TOK_WORD str_val
	{
		/* Global settings */
		if (!strcmp($1, "time") && !strcmp($3, "font")) {
			free(timeline->time_font);
			timeline->time_font = strdup($4);
		} else if (!strcmp($1, "description") && !strcmp($3, "font")) {
			free(timeline->description_font);
			timeline->description_font = strdup($4);
		} else if (!strcmp($1, "time") && !strcmp($3, "format")) {
			free(timeline->time_format);
			timeline->time_format = strdup($4);
		} else if (!strcmp($1, "camera") && !strcmp($3, "scroll")) {
			timeline->camera_scroll = (strcmp($4, "no")    != 0 &&
			                           strcmp($4, "false") != 0 &&
			                           strcmp($4, "0")     != 0);
		} else if (!strcmp($1, "timeline") && !strcmp($3, "drop")) {
			timeline->timeline_drop = (strcmp($4, "no")    != 0 &&
			                           strcmp($4, "false") != 0 &&
			                           strcmp($4, "0")     != 0);
		} else if (!strcmp($1, "output") && !strcmp($3, "format")) {
			free(timeline->output_format);
			timeline->output_format = strdup($4);
		} else if (!strcmp($1, "callout") && !strcmp($3, "shape")) {
			free(timeline->callout_shape);
			timeline->callout_shape = strdup($4);
		} else if (!strcmp($1, "callout") && !strcmp($3, "effect")) {
			free(timeline->callout_effect);
			timeline->callout_effect = strdup($4);
		} else if (!strcmp($1, "progress") && !strcmp($3, "show")) {
			timeline->progress_show = (strcmp($4, "no")    != 0 &&
			                           strcmp($4, "false") != 0 &&
			                           strcmp($4, "0")     != 0);
		} else if (!strcmp($1, "split") && !strcmp($3, "show")) {
			timeline->split_show = (strcmp($4, "no")    != 0 &&
			                        strcmp($4, "false") != 0 &&
			                        strcmp($4, "0")     != 0);
		} else if (!strcmp($1, "transition") && !strcmp($3, "style")) {
			free(timeline->transition_style);
			timeline->transition_style = strdup($4);

		/* Per-event "heavy drop" wobble override */
		} else if (!strcmp($1, "event") && !strcmp($3, "drop")) {
			pending.has_drop = 1;
			pending.drop = (strcmp($4, "no")    != 0 &&
			                strcmp($4, "false") != 0 &&
			                strcmp($4, "0")     != 0);

		/* Per-event callout exit effect override */
		} else if (!strcmp($1, "event") && !strcmp($3, "callout_effect")) {
			free(pending.callout_effect);
			pending.callout_effect = strdup($4);

		/* Per-event: image path — resolve relative to the .tig file */
		} else if (!strcmp($1, "event") && !strcmp($3, "image")) {
			free(pending.dot_image);
			if ($4[0] == '/' || $4[0] == '~') {
				pending.dot_image = strdup($4);
			} else {
				char resolved[4096];
				snprintf(resolved, sizeof(resolved), "%s/%s",
				         t2g_get_basedir(), $4);
				pending.dot_image = strdup(resolved);
			}

		/* Per-event: callout image override */
		} else if (!strcmp($1, "event") && !strcmp($3, "callout_image")) {
			free(pending.callout_image);
			if ($4[0] == '/' || $4[0] == '~') {
				pending.callout_image = strdup($4);
			} else {
				char resolved[4096];
				snprintf(resolved, sizeof(resolved), "%s/%s",
				         t2g_get_basedir(), $4);
				pending.callout_image = strdup(resolved);
			}

		/* Per-event: label area image */
		} else if (!strcmp($1, "event") && !strcmp($3, "label_image")) {
			free(pending.label_image);
			if ($4[0] == '/' || $4[0] == '~') {
				pending.label_image = strdup($4);
			} else {
				char resolved[4096];
				snprintf(resolved, sizeof(resolved), "%s/%s",
				         t2g_get_basedir(), $4);
				pending.label_image = strdup(resolved);
			}

		/* Global: callout image and size */
		} else if (!strcmp($1, "callout") && !strcmp($3, "image")) {
			free(timeline->callout_image);
			if ($4[0] == '/' || $4[0] == '~') {
				timeline->callout_image = strdup($4);
			} else {
				char resolved[4096];
				snprintf(resolved, sizeof(resolved), "%s/%s",
				         t2g_get_basedir(), $4);
				timeline->callout_image = strdup(resolved);
			}
		}

		free($1); free($3); free($4);
	}
	;

/* ── ARGB color settings ─────────────────────────────────── */
setting_argb: TOK_WORD TOK_DOT TOK_WORD TOK_ARGB
	{
		t2gcolor_t c = parse_argb($4);

		/* Global theme */
		if (!strcmp($1, "theme")) {
			if      (!strcmp($3, "background"))  timeline->theme_background  = c;
			else if (!strcmp($3, "background2")) timeline->theme_background2 = c;
			else if (!strcmp($3, "accent"))      timeline->theme_accent      = c;
			else if (!strcmp($3, "text"))        timeline->theme_text        = c;
		} else if (!strcmp($1, "timeline") && !strcmp($3, "color")) {
			timeline->timeline_color = c;
			timeline->theme_accent   = c;
		} else if (!strcmp($1, "mark") && !strcmp($3, "color")) {
			timeline->mark_color = c;
		} else if (!strcmp($1, "callout")) {
			if (!strcmp($3, "color")) {
				timeline->has_callout_color = 1;
				timeline->callout_color = c;
			} else if (!strcmp($3, "border")) {
				timeline->has_callout_border = 1;
				timeline->callout_border = c;
			}
		} else if (!strcmp($1, "progress")) {
			if (!strcmp($3, "color")) {
				timeline->has_progress_color = 1;
				timeline->progress_color = c;
			} else if (!strcmp($3, "background")) {
				timeline->has_progress_background = 1;
				timeline->progress_background = c;
			}
		} else if (!strcmp($1, "split") && !strcmp($3, "background")) {
			timeline->has_split_bg = 1;
			timeline->split_bg = c;

		/* Per-event colors */
		} else if (!strcmp($1, "event")) {
			if (!strcmp($3, "dot_color")) {
				pending.has_dot_color = 1;
				pending.dot_color = c;
			} else if (!strcmp($3, "text_color")) {
				pending.has_text_color = 1;
				pending.text_color = c;
			} else if (!strcmp($3, "line_color")) {
				pending.has_line_color = 1;
				pending.line_color = c;
			} else if (!strcmp($3, "timeline_color")) {
				pending.has_timeline_color = 1;
				pending.timeline_color = c;
			} else if (!strcmp($3, "background")) {
				if (!pending.has_ev_background) {
					/* default bottom to same unless overridden */
					pending.ev_background2 = c;
				}
				pending.has_ev_background = 1;
				pending.ev_background = c;
			} else if (!strcmp($3, "background2")) {
				pending.has_ev_background = 1;
				pending.ev_background2 = c;
			}
		}

		free($1); free($3); free($4);
	}
	;

/* ── Integer settings ────────────────────────────────────── */
setting_int: TOK_WORD TOK_DOT TOK_WORD TOK_INTEGER
	{
		/* Global */
		if (!strcmp($1, "image")) {
			if      (!strcmp($3, "width"))  timeline->width  = $4;
			else if (!strcmp($3, "height")) timeline->height = $4;
		} else if (!strcmp($1, "timeline") && !strcmp($3, "position")) {
			timeline->timeline_pos_y = $4;
		} else if (!strcmp($1, "timeline") && !strcmp($3, "drop_amount")) {
			timeline->timeline_drop_amount = $4;
		} else if (!strcmp($1, "speed")) {
			if      (!strcmp($3, "frames"))     timeline->speed_frames     = $4;
			else if (!strcmp($3, "nextitem"))   timeline->speed_nextitem   = $4;
			else if (!strcmp($3, "loop_pause")) timeline->speed_loop_pause = $4;
			else if (!strcmp($3, "pause"))    { timeline->has_speed_pause  = 1;
			                                    timeline->speed_pause      = $4; }
		} else if (!strcmp($1, "time") && !strcmp($3, "origin")) {
			timeline->has_time_origin = 1;
			timeline->time_origin = (double)$4;
		} else if (!strcmp($1, "item")) {
			if (!strcmp($3, "spacing") || !strcmp($3, "space"))
				timeline->item_spacing = $4;
		} else if (!strcmp($1, "time") && !strcmp($3, "font_size")) {
			timeline->time_font_size = $4;
		} else if (!strcmp($1, "description") && !strcmp($3, "font_size")) {
			timeline->description_font_size = $4;
		} else if (!strcmp($1, "transition") && !strcmp($3, "frames")) {
			timeline->transition_frames = $4;
		} else if (!strcmp($1, "transition") && !strcmp($3, "block_size")) {
			timeline->transition_block_size = $4;
		} else if (!strcmp($1, "callout") && !strcmp($3, "pause")) {
			timeline->callout_pause = $4;
		} else if (!strcmp($1, "callout") && !strcmp($3, "image_size")) {
			timeline->callout_image_size = $4;
		} else if (!strcmp($1, "progress") && !strcmp($3, "height")) {
			timeline->progress_height = $4;
		} else if (!strcmp($1, "split") && !strcmp($3, "width")) {
			timeline->split_width = $4;

		/* Per-event */
		} else if (!strcmp($1, "event")) {
			if      (!strcmp($3, "x"))                 pending.x_pos               = $4;
			else if (!strcmp($3, "image_size"))        pending.dot_image_size      = $4;
			else if (!strcmp($3, "callout_image_size")) pending.callout_image_size = $4;
			else if (!strcmp($3, "label_image_size"))  pending.label_image_size    = $4;
			else if (!strcmp($3, "pause"))           { pending.has_event_pause = 1; pending.event_pause = $4; }
			else if (!strcmp($3, "drop_amount"))       pending.drop_amount         = $4;
		}

		free($1); free($3);
	}
	;

/* ── Timeline items ──────────────────────────────────────── */
item: TOK_STRING TOK_STRING
	{
		t2g_t *ev = t2g_new();
		ev->time_text  = strdup($1);
		ev->label_text = strdup($2);
		pending_apply(ev);
		pending_reset();
		t2g_append(timeline, ev);
		free($1); free($2);
	}
	;

%%

void t2g_parser_init(void)
{
	pending_reset();
}

void t2g_parser_terminate(void)
{
	pending_reset();
}

void t2gerror(t2g_t *t2g, const char *str)
{
	(void)t2g;
	fprintf(stderr, "Parse error near '%s' line %d: %s\n",
	        t2gget_text(), t2gget_lineno() + 1, str);
	/* Do NOT exit() — when embedded in the GUI, exit() kills the whole app.
	   Bison will return a non-zero value from t2gparse() after this call. */
}
