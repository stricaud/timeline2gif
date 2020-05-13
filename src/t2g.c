#include <stdio.h>
#include <stdlib.h>

#include "t2g.h"

t2g_t *t2g_new(uint16_t width, uint16_t height)
{
	t2g_t *t2g;

	t2g = (t2g_t *)malloc(sizeof(t2g_t));
	if (!t2g) {
		fprintf(stderr, "Cannot initialize t2g!\n");
		return NULL;
	}
	t2g->root = t2g;
	t2g->next = NULL;

	return t2g;
}

int t2g_append(t2g_t *t2g) {
	t2g_t *iter;
	iter = t2g->root;
	if (iter) {
		while (iter->next) {
			iter = iter->next;
		}
	}
	iter->next = (t2g_t *)malloc(sizeof(t2g_t));
	if (!iter->next) {
		fprintf(stderr, "Cannot initialize the item to append!\n");
		return -1;
	}
	iter->next->next = NULL;
	iter->next->foo = 1234;

	return 0;
}


void t2g_free(t2g_t *t2g)
{
	free(t2g);
}

