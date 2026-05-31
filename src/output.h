#ifndef _T2G_OUTPUT_H_
#define _T2G_OUTPUT_H_

#include <cairo/cairo.h>
#include "t2g.h"

typedef enum {
	OUTPUT_GIF  = 0,
	OUTPUT_WEBP = 1,
	OUTPUT_APNG = 2
} output_format_t;

typedef struct output_ctx_t output_ctx_t;

/* Determine format from t2g settings or filename extension. */
output_format_t output_detect_format(t2g_t *root, const char *filename);

output_ctx_t *output_create(const char *filename, output_format_t fmt,
                             int width, int height);

/* Add one frame.  delay_ms is the frame duration in milliseconds. */
void output_add_frame(output_ctx_t *ctx, cairo_surface_t *surface,
                      int delay_ms);

/* Finalise and write the file.  Returns 0 on success. */
int output_finish(output_ctx_t *ctx);

void output_destroy(output_ctx_t *ctx);

/* High-level entry point called from main. */
int write_output(t2g_t *t2g, const char *filename);

#endif /* _T2G_OUTPUT_H_ */
