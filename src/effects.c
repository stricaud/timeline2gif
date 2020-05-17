#include <gd.h>
#include <gdfontl.h>

#include "t2g.h"
#include "effects.h"

gdImagePtr effects_center_text(t2g_t *t2g, gdImagePtr im, int x, int y, int angle, char *text)
{
	int brect[8];
	
	gdImageStringFT(NULL, brect, 0,
			DEFAULT_FONT, 16,
			0,
			0, angle,
			text);
	int text_width = brect[BRECT_LOWER_RIGHT_X] - brect[BRECT_LOWER_LEFT_X];
	int text_height = brect[BRECT_LOWER_LEFT_Y] - brect[BRECT_UPPER_LEFT_Y];
	int text_center_pos_x = text_width / 2;
	int text_center_pos_y = text_height / 2;
	
	gdImageStringFT(im, brect, gdTrueColorAlpha(0,0,0,0),
			DEFAULT_FONT, 16,
			angle,
			x-text_center_pos_x, y-text_center_pos_y,
			text);

	return im;
}

gdImagePtr effects_text(t2g_t *t2g, gdImagePtr im, int x, int y, int angle, char *text)
{
	int brect[8];
	
	gdImageStringFT(im, brect, gdTrueColorAlpha(0,0,0,0),
			DEFAULT_FONT, 16,
			angle,
			x, y,
			text);

	return im;
}


gdImagePtr effects_central_rect_shrinks_to_xy(t2g_t *t2g, gdImagePtr im, int frame, int dest_x, int dest_y)
{
	int rect_p = 6; // Proportion of the rectangle/image

	int x1, y1, x2, y2;

	x1 = t2g->width/2-t2g->width/rect_p;
	y1 = t2g->height/2-t2g->height/rect_p;
	x2 = t2g->width/2+t2g->width/rect_p;
	y2 = t2g->height/2+t2g->height/rect_p;
	
	switch(frame) {
	case 0:
		gdImageSetThickness(im, 4);
		gdImageFilledRectangle(im, x1, y1, x2, y2, gdTrueColorAlpha(255,255,255,0));
		gdImageRectangle(im, x1, y1, x2, y2, gdTrueColorAlpha(0,0,0,0));
		gdImageSetThickness(im, 1);
		break;
	case 1:
		/* gdImageSetThickness(im, 3); */
		/* gdImageRectangle(im, */
		/* 		 dest_x > x1 ? (dest_x - x1)/2 : (dest_x + x1)/2, */
		/* 		 dest_y > y1 ? (dest_y - y1)/2 : (dest_y + y1)/2, */
		/* 		 dest_x > x2 ? (dest_x - x2)/2 : (dest_x + x2)/2, */
		/* 		 dest_y > y2 ? (dest_y - y2)/2 : (dest_y + y2)/2, */
		/* 		 gdTrueColorAlpha(0,0,0,0)); */
		/* gdImageSetThickness(im, 1); */
		break;
	case 2:
		break;
	}

	return im;
}

gdImagePtr effects_linedown(t2g_t *t2g, gdImagePtr im, int frame, int x)
{
	int median = (t2g->height - t2g->timeline_pos_y) / 2;
	int linesize = 10;
	int lineposition_y = ((t2g->height - t2g->timeline_pos_y) / 2) / FRAMES_PER_ITEM ;
	

	if (frame > 2) {
		gdImageSetThickness(im, 2);
		gdImageLine(im, x, t2g->timeline_pos_y + 10, x, (t2g->timeline_pos_y + 10) - (linesize * frame),gdTrueColorAlpha(0, 98, 174, 0));
		gdImageSetThickness(im, 1);
	}
	if (frame == FRAMES_PER_ITEM-1) {
		gdImageFilledEllipse(im, x, (t2g->timeline_pos_y + 10) - (linesize * frame), 10, 10, gdTrueColorAlpha(0, 98, 174, 0));
		im = effects_text(t2g, im, x, (t2g->timeline_pos_y + 10) - (linesize * frame) - 20, 45, t2g->label_text);
		im = effects_center_text(t2g, im, x, t2g->timeline_pos_y + 40, 0, t2g->time_text);
	}
			
	return im;
}

gdImagePtr effects_pin(t2g_t *t2g, gdImagePtr im, int frame, int x)
{
	/* gdImageFilledEllipse(im, 40, (IMHEIGHT/2), 10, 10, gdImageColorAllocate(im, 0, 98, 174)); */
	return im;
}
