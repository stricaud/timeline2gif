#ifndef _TIMELINE2GIF_H_
#define _TIMELINE2GIF_H_

#include <stdint.h>

struct _t2g_t {
	int foo;
};
typedef struct _t2g_t t2g_t;

t2g_t *t2g_new(uint16_t width, uint16_t height);
void t2g_free(t2g_t *timeline);

#endif // _TIMELINE2GIF_H_
