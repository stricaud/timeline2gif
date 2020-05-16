#include <stdio.h>
#include <stdlib.h>
#include <gd.h>
#include <gdfontl.h>

#include "colors.h"
#include "t2g.h"

#define KEYFRAMES 10

static void _allocate_colors(gdImagePtr im)
{
	t2g_white = gdImageColorAllocate(im, 255, 255, 255);
	t2g_black = gdImageColorAllocate(im, 0, 0, 0);
	t2g_blue = gdImageColorAllocate(im, 0, 98, 174);
}

static gdImagePtr _new_image_with_background(t2g_t *t2g, gdImagePtr im, char r, char g, char b)
{
	im = gdImageCreate(t2g->width, t2g->height);
	if (!im) {
		return NULL;
	}
	gdImageColorAllocate(im, r, g, b);
	return im;
}

static gdImagePtr _write_in_every_frame(t2g_t *t2g, gdImagePtr im)
{
	int black;

	_new_image_with_background(t2g, im, 255, 255, 255);
	
	gdImageLine(im, 0, 0+(t2g->height/2), t2g->width, 0+(t2g->height/2), t2g_black); // Timeline line
	return im;
}

static int _write_single_object(FILE *out, t2g_t *t2g, gdImagePtr im)
{
	gdImagePtr prev =NULL;
	int black, white;
	int delay = 6;
	int ypos = 0;
	int xpos = 0;
	
	char framepos;
	for (framepos = 0; framepos < KEYFRAMES; framepos++) {
		im = _write_in_every_frame(t2g, im);

		
		
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
	_allocate_colors(im);
	
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
