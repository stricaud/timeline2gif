#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "t2g.h"

/* ── Base directory for resolving relative paths ────────────────────── */

static char _t2g_basedir[4096] = ".";

void t2g_set_basedir(const char *tig_path)
{
	const char *last = strrchr(tig_path, '/');
	if (last && last != tig_path) {
		size_t len = (size_t)(last - tig_path);
		if (len >= sizeof(_t2g_basedir)) len = sizeof(_t2g_basedir) - 1;
		strncpy(_t2g_basedir, tig_path, len);
		_t2g_basedir[len] = '\0';
	} else {
		strcpy(_t2g_basedir, ".");
	}
}

const char *t2g_get_basedir(void)
{
	return _t2g_basedir;
}

static const char *_font_search_paths[] = {
	/* macOS */
	"/System/Library/Fonts/Supplemental/Arial.ttf",
	"/Library/Fonts/Arial.ttf",
	/* Linux – Liberation (metric-compatible with Arial) */
	"/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
	"/usr/share/fonts/liberation-sans/LiberationSans-Regular.ttf",
	"/usr/share/fonts/liberation/LiberationSans-Regular.ttf",
	/* Linux – DejaVu (widely pre-installed) */
	"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
	"/usr/share/fonts/dejavu/DejaVuSans.ttf",
	/* Homebrew Noto */
	"/opt/homebrew/share/fonts/noto/NotoSans-Regular.ttf",
	NULL
};

/* Returns a Pango font family name available on this system, or "Sans" as
   universal fallback (Pango resolves it via fontconfig). */
char *t2g_find_default_font(void)
{
	/* Check which TTF files exist and derive the family name. */
	struct { const char *path; const char *family; } candidates[] = {
		{ "/System/Library/Fonts/Supplemental/Arial.ttf", "Arial" },
		{ "/Library/Fonts/Arial.ttf",                     "Arial" },
		{ "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
		                                                   "Liberation Sans" },
		{ "/usr/share/fonts/liberation-sans/LiberationSans-Regular.ttf",
		                                                   "Liberation Sans" },
		{ "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", "DejaVu Sans" },
		{ "/usr/share/fonts/dejavu/DejaVuSans.ttf",          "DejaVu Sans" },
		{ NULL, NULL }
	};
	for (int i = 0; candidates[i].path; i++) {
		if (access(candidates[i].path, R_OK) == 0)
			return (char *)candidates[i].family;
	}
	return "Sans";
}

t2g_t *t2g_new(void)
{
	t2g_t *t2g = calloc(1, sizeof(t2g_t));
	if (!t2g) {
		fprintf(stderr, "Cannot initialize t2g!\n");
		return NULL;
	}

	/* Canvas */
	t2g->width  = 800;
	t2g->height = 500;
	t2g->timeline_pos_y = t2g->height / 2;

	/* Timeline color */
	t2g->timeline_color = (t2gcolor_t){ 255, 0, 0, 0 };

	/* Theme: dark navy with a bright blue accent */
	t2g->theme_background  = (t2gcolor_t){ 255,  22,  33,  62 }; /* #16213e */
	t2g->theme_background2 = (t2gcolor_t){ 255,  10,  25,  47 }; /* #0a1929 */
	t2g->theme_accent      = (t2gcolor_t){ 255,   0, 180, 216 }; /* #00b4d8 */
	t2g->theme_text        = (t2gcolor_t){ 255, 224, 224, 224 }; /* #e0e0e0 */

	/* Mark color */
	t2g->mark_color = (t2gcolor_t){ 255, 0, 98, 174 };

	/* Fonts */
	t2g->time_font            = NULL;
	t2g->time_font_size       = 0;
	t2g->description_font     = NULL;
	t2g->description_font_size = 0;

	/* Speed (centiseconds) */
	t2g->speed_frames   = 5;
	t2g->speed_nextitem = 50;

	/* Layout */
	t2g->item_spacing = 160;

	/* Camera */
	t2g->camera_scroll = 1;

	/* Transition */
	t2g->transition_style  = NULL;  /* NULL → "none" */
	t2g->transition_frames = 8;

	/* Output */
	t2g->output_format = NULL;

	/* Per-item data */
	t2g->time_text  = NULL;
	t2g->label_text = NULL;

	/* List */
	t2g->root = t2g;
	t2g->next = NULL;

	return t2g;
}

/* Append a new event item to the linked list and set its root pointer. */
int t2g_append(t2g_t *t2g_root, t2g_t *t2g_next)
{
	t2g_t *iter = t2g_root->root;
	while (iter->next)
		iter = iter->next;
	iter->next = t2g_next;
	t2g_next->root = t2g_root->root;  /* point every item back to the real root */
	return 0;
}

void t2g_free(t2g_t *t2g)
{
	t2g_t *iter = t2g->root;
	while (iter) {
		t2g_t *next = iter->next;
		free(iter->time_font);
		free(iter->description_font);
		free(iter->time_text);
		free(iter->label_text);
		free(iter->output_format);
		free(iter->transition_style);
		free(iter->time_format);
		free(iter->callout_shape);
		free(iter->callout_effect);
		free(iter->ev_callout_effect);
		free(iter->dot_image);
		free(iter);
		iter = next;
	}
}

char *t2g_get_description_font(t2g_t *t2g)
{
	if (t2g->description_font)
		return t2g->description_font;
	if (t2g->root->description_font)
		return t2g->root->description_font;
	return t2g_find_default_font();
}

int t2g_get_description_font_size(t2g_t *t2g)
{
	if (t2g->description_font_size)
		return t2g->description_font_size;
	if (t2g->root->description_font_size)
		return t2g->root->description_font_size;
	return 13;
}

char *t2g_get_time_font(t2g_t *t2g)
{
	if (t2g->time_font)
		return t2g->time_font;
	if (t2g->root->time_font)
		return t2g->root->time_font;
	return t2g_find_default_font();
}

int t2g_get_time_font_size(t2g_t *t2g)
{
	if (t2g->time_font_size)
		return t2g->time_font_size;
	if (t2g->root->time_font_size)
		return t2g->root->time_font_size;
	return 11;
}
