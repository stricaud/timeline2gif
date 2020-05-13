#ifndef _T2G_H_
#define _T2G_H_

#include <stdint.h>

struct _t2g_t {
	int foo;
	struct _t2g_t *root;
	struct _t2g_t *next;
};
typedef struct _t2g_t t2g_t;

t2g_t *t2g_new(uint16_t width, uint16_t height);
void t2g_free(t2g_t *timeline);

#endif // _T2G_H_
