#ifndef _T2G_EFFECTS_H_
#define _T2G_EFFECTS_H_

#include <gd.h>
#include <gdfontl.h>

#include "t2g.h"

#define DEFAULT_FONT "/usr/share/wine/fonts/times.ttf"

#define BRECT_LOWER_LEFT_X 0
#define BRECT_LOWER_LEFT_Y 1
#define BRECT_LOWER_RIGHT_X 2
#define BRECT_LOWER_RIGHT_Y 3
#define BRECT_UPPER_RIGHT_X 4
#define BRECT_UPPER_RIGHT_Y 5
#define BRECT_UPPER_LEFT_X 6
#define BRECT_UPPER_LEFT_Y 7

gdImagePtr effects_center_text(t2g_t *t2g, gdImagePtr im, int x, int y, char *text);
gdImagePtr effects_central_rect_shrinks_to_xy(t2g_t *t2g, gdImagePtr im, int frame, int dest_x, int dest_y);

#endif	// _T2G_EFFECTS_H_
