#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gd.h>
#include <gdfontl.h>

#include "colors.h"
#include "effects.h"
#include "t2g.h"

#define KEYFRAMES 10

static gdImagePtr permanent_im;

static void _save_to_permanent_image(t2g_t *t2g, gdImagePtr im)
{
	gdImageCopy(permanent_im, im, 0, 0, 0, 0, t2g->width-1, t2g->height-1);
}

static gdImagePtr _merge_with_permanent_image(t2g_t *t2g, gdImagePtr im)
{
	gdImageCopy(im, permanent_im, 0, 0, 0, 0, t2g->width-1, t2g->height-1);
	
	return im;
}

static void _destroy_permanent_image()
{
	gdImageDestroy(permanent_im);
}


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
	// We clear our image for each frame. This allows us to run effects and animation that don't persist.
	gdImageFilledRectangle(im, 0, 0, t2g->width, t2g->height, gdTrueColorAlpha(255,255,255,0));
	// However we want stuff to persist, here they come:
	im = _merge_with_permanent_image(t2g, im);
	
	gdImageLine(im, 0, t2g->timeline_pos_y, t2g->width, t2g->timeline_pos_y, gdTrueColorAlpha(0,0,0,0)); // Timeline line
	/* gdImageColorTransparent (im, gdTrueColorAlpha(0,0,0,0)); */
	gdImageSetAntiAliased (im, gdTrueColorAlpha(0,0,0,0));
	
	return im;
}

static int _write_single_object(FILE *out, t2g_t *t2g, gdImagePtr im, int count)
{
	gdImagePtr prev =NULL;
	int black, white;
	/* int delay = 6; */
	int ypos = 0;
	int xpos = 0;
	int brect[8];
	int effect_speed;

	
	char framepos;
	for (framepos = 0; framepos < KEYFRAMES; framepos++) {
		im = _write_in_every_frame(t2g, im);

		im = effects_linedown(t2g, im, framepos, count * 100);
		
		effect_speed = 0;
		
		switch(framepos) {
		case 0:
			im = effects_central_rect_shrinks_to_xy(t2g, im, framepos, 0, t2g->height/2);
			im = effects_center_text(t2g, im,
						 t2g_get_description_font(t2g),
						 t2g_get_description_font_size(t2g) + 5,
						 t2g->width/2, t2g->height/2, 0, t2g->label_text);
			effect_speed = 100;
			break;
		case 1:
			im = effects_central_rect_shrinks_to_xy(t2g, im, framepos, 0, t2g->height/2);
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
			_save_to_permanent_image(t2g, im);
			break;
			
		default:
			fprintf(stderr, "Uh? Should nevery see this!\n");
		}
		

		if (framepos < 9) {
			gdImageGifAnimAdd(im, out, 1, 0, 0, t2g->speed_frames + effect_speed, 0, NULL);
		} else {
			gdImageGifAnimAdd(im, out, 1, 0, 0, t2g->speed_nextitem, 0, NULL);			
		}
	}
	
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
	int count = 1;
	
	out = fopen(outgif, "wb");
	if (!out) {
		fprintf(stderr, "can't create file %s", outgif);
		return 1;
	} 	

	im = _new_image_with_background(t2g, im, 255, 255, 255);
	permanent_im = _new_image_with_background(t2g, permanent_im, 255, 255, 255);
	
	gdImageGifAnimBegin(im, out, 1, -1);
	
	iter = t2g->root;

	if(iter) {
		while (iter->next) {
			iter = iter->next;
			_write_single_object(out, iter, im, count);
			count++;
		}
	}      

	gdImageGifAnimEnd(out);

	gdImageDestroy(im);
	fclose(out);
	
	return 0;
}
