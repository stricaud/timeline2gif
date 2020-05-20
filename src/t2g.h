#ifndef _T2G_H_
#define _T2G_H_

#include <stdio.h>
#include <stdint.h>


#define DEFAULT_FONT T2G_DATA_DIR"/fonts/NotoMono-Regular.ttf"


struct _t2gcolor_t {
	unsigned char a;
	unsigned char r;
	unsigned char g;
	unsigned char b;
};
typedef struct _t2gcolor_t t2gcolor_t;

struct _t2g_t {
	t2gcolor_t timeline_color;
	t2gcolor_t mark_color;
	char *time_text;
	char *label_text;
	
	int width;
	int height;

	int speed_nextitem;
	int speed_frames;
	int timeline_pos_y;

	char *time_font;
	int time_font_size;
	char *description_font;
	int description_font_size;
	
	struct _t2g_t *root;
	struct _t2g_t *next;
};
typedef struct _t2g_t t2g_t;

/* Check parser.y */
void t2g_parser_init(void);
void t2g_parser_terminate(void);
void t2grestart(FILE *);

t2g_t *t2g_new();
int t2g_append(t2g_t *t2g_root, t2g_t *next);
void t2g_free(t2g_t *timeline);

char *t2g_get_description_font(t2g_t *t2g);
int t2g_get_description_font_size(t2g_t *t2g);
char *t2g_get_time_font(t2g_t *t2g);
int t2g_get_time_font_size(t2g_t *t2g);

#endif // _T2G_H_
