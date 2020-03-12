#include <stdio.h>
#include <stdlib.h>
#include <gd.h>
#include <gdfontl.h>

#include <parser.h>

#define IMHEIGHT 500
#define IMWIDTH 800

int main(void)
{
	int i;
	FILE * out;

	gdImagePtr im;
	gdImagePtr prev =NULL;
	int black, white;
	int delay = 6;
	int ypos;
	
	im = gdImageCreate(IMWIDTH, IMHEIGHT);
	if (!im) {
		fprintf(stderr, "can't create image");
		return 1;
	}

	out = fopen("anim.gif", "wb");
	if (!out) {
		fprintf(stderr, "can't create file %s", "anim.gif");
		return 1;
	}

	white = gdImageColorAllocate(im, 255, 255, 255); /* allocate white as side effect */
	gdImageGifAnimBegin(im, out, 1, -1);

	ypos = 0;
	for(i = 0; i < 14; i++) {
		int r,g,b;
		im = gdImageCreate(IMWIDTH, IMHEIGHT);
		
		/* r = rand() % 255; */
		/* g = rand() % 255; */
		/* b = rand() % 255; */

		gdImageColorAllocate(im, 255, 255, 255);  /* allocate white as side effect */
//		black = gdImageColorAllocate(im, 0, 0, 0);
		black = gdImageColorAllocate(im, 0, 0, 0);
		gdImageLine(im, 0, 0+(IMHEIGHT/2), IMWIDTH, 0+(IMHEIGHT/2), black); // Timeline line

		if (i < 10) {
			ypos = i*((IMHEIGHT/2)/10);
		}
		
		gdImageLine(im, 40, ypos-20, 40, ypos+20, black);

		if (i > 9) {
			gdImageFilledEllipse(im, 40, (IMHEIGHT/2), 10, 10, gdImageColorAllocate(im, 0, 98, 174));
		}
		if (i == 10) {
			gdImageString(im, gdFontGetLarge(), 40, ypos - 30, "This", black);
			delay += 10;
		}
		if (i == 11) {
			gdImageString(im, gdFontGetLarge(), 40, ypos - 30, "This is", black);
		}
		if (i == 12) {
			gdImageString(im, gdFontGetLarge(), 40, ypos - 30, "This is my", black);
		}
		if (i == 13) {
			gdImageString(im, gdFontGetLarge(), 40, ypos - 30, "This is my\n story", black);
		}
/* gdImageLine(im, 0, 0+i, IMWIDTH, 0+i, black); */

		/* black = gdImageColorAllocate(im,  r, g, b); */
		/* printf("(%i, %i, %i)\n",r, g, b); */
		/* gdImageFilledRectangle(im, rand() % 100, rand() % 100, rand() % 100, rand() % 100, black); */
		gdImageGifAnimAdd(im, out, 1, 0, 0, delay, 1, prev);

		if(prev) {
			gdImageDestroy(prev);
		}
		prev = im;
	}

	gdImageGifAnimEnd(out);
	fclose(out);

	return 0;
}
