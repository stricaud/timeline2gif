#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gd.h>
#include <gdfontl.h>

#include "colors.h"
#include "t2g.h"

#define KEYFRAMES 10

#define BRECT_LOWER_LEFT_X 0
#define BRECT_LOWER_LEFT_Y 1
#define BRECT_LOWER_RIGHT_X 2
#define BRECT_LOWER_RIGHT_Y 3
#define BRECT_UPPER_RIGHT_X 4
#define BRECT_UPPER_RIGHT_Y 5
#define BRECT_UPPER_LEFT_X 6
#define BRECT_UPPER_LEFT_Y 7


//#define DEFAULT_FONT "/usr/share/fonts/gnu-free/FreeMono.ttf"
#define DEFAULT_FONT "/usr/share/wine/fonts/times.ttf"

static gdImagePtr _new_image_with_background(t2g_t *t2g, gdImagePtr im, char r, char g, char b)
{
	im = gdImageCreateTrueColor(t2g->width, t2g->height);
	if (!im) {
		fprintf(stderr, "Error: Could not create TrueColor image\n");
		return NULL;
	}
	gdImageAlphaBlending(im, gdEffectReplace);
	gdImageFilledRectangle(im, 0, 0, t2g->width, t2g->height, gdTrueColorAlpha(255,255,255,0));
	return im;
}

static gdImagePtr _write_in_every_frame(t2g_t *t2g, gdImagePtr im)
{

	_new_image_with_background(t2g, im, 255, 255, 255);
	
	gdImageLine(im, 0, t2g->height/2, t2g->width, t2g->height/2, gdTrueColorAlpha(0,0,0,0)); // Timeline line
	/* gdImageColorTransparent (im, gdTrueColorAlpha(0,0,0,0)); */
	gdImageSetAntiAliased (im, gdTrueColorAlpha(0,0,0,0));
	return im;
}

static int _write_single_object(FILE *out, t2g_t *t2g, gdImagePtr im)
{
	gdImagePtr prev =NULL;
	int black, white;
	int delay = 6;
	int ypos = 0;
	int xpos = 0;
	int brect[8];
	int rect_p = 6; // Proportion of the rectangle/image
	
	char framepos;
	for (framepos = 0; framepos < KEYFRAMES; framepos++) {
		im = _write_in_every_frame(t2g, im);

		switch(framepos) {
		case 0:

			gdImageSetThickness(im, 4);
			gdImageFilledRectangle(im, t2g->width/2-t2g->width/rect_p, t2g->height/2-t2g->height/rect_p, t2g->width/2+t2g->width/rect_p, t2g->height/2+t2g->width/rect_p, gdTrueColorAlpha(255,255,255,0));
			gdImageRectangle(im, t2g->width/2-t2g->width/rect_p, t2g->height/2-t2g->height/rect_p, t2g->width/2+t2g->width/rect_p, t2g->height/2+t2g->width/rect_p, gdTrueColorAlpha(0,0,0,0));
			gdImageSetThickness(im, 1);
			
			gdImageStringFT(NULL, brect, 0,
					DEFAULT_FONT, 16,
					0,
					0, 0,
					t2g->label_text);
			int text_width = brect[BRECT_LOWER_RIGHT_X] - brect[BRECT_LOWER_LEFT_X];
			int text_height = brect[BRECT_LOWER_LEFT_Y] - brect[BRECT_UPPER_LEFT_Y];
			int text_center_pos_x = text_width / 2;
			int text_center_pos_y = text_height / 2;
			gdImageStringFT(im, brect, gdTrueColorAlpha(0,0,0,0),
					DEFAULT_FONT, 16,
					0,
					t2g->width/2-text_center_pos_x, t2g->height/2-text_center_pos_y,
					t2g->label_text);
			break;
		case 1:
			break;
		case 2:
			break;
		case 3:
			break;
		case 4:
			break;
		case 5:
			break;
		case 6:
			break;
		case 7:
			break;
		case 8:
			break;
		case 9:
			break;
			
		default:
			fprintf(stderr, "Uh? Should nevery see this!\n");
		}
		
		
		gdImageGifAnimAdd(im, out, 1, 0, 0, delay, 0, NULL);
	}
		
	
	
	
/* 	ypos = 0; */
/* 	for(i = 0; i < 14; i++) { */
/* 		int r,g,b; */
/* 		im = gdImageCreate(imwidth, imheight); */
		
/* 		/\* r = rand() % 255; *\/ */
/* 		/\* g = rand() % 255; *\/ */
/* 		/\* b = rand() % 255; *\/ */

/* 		gdImageColorAllocate(im, 255, 255, 255);  /\* allocate white as side effect *\/ */
/* //		black = gdImageColorAllocate(im, 0, 0, 0); */
/* 		black = gdImageColorAllocate(im, 0, 0, 0); */
/* 		gdImageLine(im, 0, 0+(imheight/2), imwidth, 0+(imheight/2), black); // Timeline line */

/* 		#if 0 */
/* 		if (i < 10) { */
/* 			ypos = i*((imheight/2)/10); */
/* 		} */
		
/* 		gdImageLine(im, 40, ypos-20, 40, ypos+20, black); */

/* 		if (i > 9) { */
/* 			gdImageFilledEllipse(im, 40, (imheight/2), 10, 10, gdImageColorAllocate(im, 0, 98, 174)); */
/* 		} */
/* 		if (i >= 10) { */
/* 			gdImageString(im, gdFontGetLarge(), 40, ypos - 30, t2g->label_text, black); */
/* 			delay += 10; */
/* 		} */
/* 		/\* if (i>10) { *\/ */
/* 		/\* 	gdImageString(im, gdFontGetLarge(), 40, ypos - 30, t2g->label_text, black); *\/ */
/* 		/\* } *\/ */
/* /\* gdImageLine(im, 0, 0+i, imwidth, 0+i, black); *\/ */

/* 		/\* black = gdImageColorAllocate(im,  r, g, b); *\/ */
/* 		/\* printf("(%i, %i, %i)\n",r, g, b); *\/ */
/* 		/\* gdImageFilledRectangle(im, rand() % 100, rand() % 100, rand() % 100, rand() % 100, black); *\/ */
/* 		gdImageGifAnimAdd(im, out, 1, 0, 0, delay, 1, prev); */

/* 		if(prev) { */
/* 			gdImageDestroy(prev); */
/* 		} */
/* 		prev = im; */
/* 		#endif */
/* 	}	 */
}

int write_gif(t2g_t *t2g, char *outgif)
{
	FILE *out;
	int retval;
	t2g_t *iter;

	gdImagePtr im;

	int black, white;
	int delay = 6;
	int ypos = 0;
	int xpos = 0;

	out = fopen(outgif, "wb");
	if (!out) {
		fprintf(stderr, "can't create file %s", outgif);
		return 1;
	} 	

	im = _new_image_with_background(t2g, im, 255, 255, 255);
	
	gdImageGifAnimBegin(im, out, 1, -1);
	
	iter = t2g->root;

	if(iter) {
		while (iter->next) {
			iter = iter->next;
			_write_single_object(out, iter, im);
		}
	}      

	gdImageGifAnimEnd(out);

	gdImageDestroy(im);
	fclose(out);
	
	return 0;
}
