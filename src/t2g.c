#include <stdio.h>
#include <stdlib.h>

#include "t2g.h"

t2g_t *t2g_new(void)
{
	t2g_t *t2g;

	t2g = (t2g_t *)malloc(sizeof(t2g_t));
	if (!t2g) {
		fprintf(stderr, "Cannot initialize t2g!\n");
		return NULL;
	}
	t2g->width = 800;
	t2g->height = 500;
	t2g->root = t2g;
	t2g->next = NULL;
	t2g->timeline_color.a = 255;
	t2g->timeline_color.r = 0;
	t2g->timeline_color.g = 0;
	t2g->timeline_color.b = 0;
	t2g->mark_color.a = 255;
	t2g->mark_color.r = 0;
	t2g->mark_color.g = 0;
	t2g->mark_color.b = 0;
	t2g->speed_nextitem = 10;
	t2g->speed_frames = 6;
	
	t2g->time_text = NULL;
	t2g->label_text = NULL;
	
	return t2g;
}

int t2g_append(t2g_t *t2g_root, t2g_t *t2g_next) {
	t2g_t *iter;
	iter = t2g_root->root;
	if (iter) {
		while (iter->next) {
			iter = iter->next;
		}
	}
	iter->next = t2g_next;

	return 0;
}

void t2g_free(t2g_t *t2g)
{
	t2g_t *iter;
	t2g_t *this;
	
	iter = t2g->root;
	if (iter) {
		while(iter->next) {
			this = iter;
			iter = iter->next;
			free(this);
		}
	}
	free(t2g);
}

