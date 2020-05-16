#include <gd.h>
#include <gdfontl.h>

#include "t2g.h"
#include "effects.h"

gdImagePtr effects_center_text(t2g_t *t2g, gdImagePtr im, int x, int y, char *text)
{
	int brect[8];
	
	gdImageStringFT(NULL, brect, 0,
			DEFAULT_FONT, 16,
			0,
			0, 0,
			text);
	int text_width = brect[BRECT_LOWER_RIGHT_X] - brect[BRECT_LOWER_LEFT_X];
	int text_height = brect[BRECT_LOWER_LEFT_Y] - brect[BRECT_UPPER_LEFT_Y];
	int text_center_pos_x = text_width / 2;
	int text_center_pos_y = text_height / 2;
	
	gdImageStringFT(im, brect, gdTrueColorAlpha(0,0,0,0),
			DEFAULT_FONT, 16,
			0,
			x-text_center_pos_x, y-text_center_pos_y,
			text);

	return im;
}

gdImagePtr effects_central_rect_shrinks_to_xy(t2g_t *t2g, gdImagePtr im, int frame, int dest_x, int dest_y)
{
	int rect_p = 6; // Proportion of the rectangle/image
		
	gdImageSetThickness(im, 4);
	gdImageFilledRectangle(im, t2g->width/2-t2g->width/rect_p, t2g->height/2-t2g->height/rect_p, t2g->width/2+t2g->width/rect_p, t2g->height/2+t2g->width/rect_p, gdTrueColorAlpha(255,255,255,0));
	gdImageRectangle(im, t2g->width/2-t2g->width/rect_p, t2g->height/2-t2g->height/rect_p, t2g->width/2+t2g->width/rect_p, t2g->height/2+t2g->width/rect_p, gdTrueColorAlpha(0,0,0,0));
	gdImageSetThickness(im, 1);

	return im;
}
