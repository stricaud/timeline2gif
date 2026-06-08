#ifndef _T2G_OUTPUT_H_
#define _T2G_OUTPUT_H_

#include <stddef.h>
#include <cairo/cairo.h>
#include "t2g.h"

typedef enum {
	OUTPUT_GIF    = 0,
	OUTPUT_WEBP   = 1,
	OUTPUT_APNG   = 2,
	OUTPUT_FRAMES = 3  /* in-memory PNG frames, no file written */
} output_format_t;

/* ── In-memory frame list (full-color PNG bytes per frame) ─────────── */

typedef struct {
	uint8_t *data;      /* Cairo PNG bytes */
	size_t   size;
	int      delay_ms;
} t2g_frame_t;

typedef struct {
	t2g_frame_t *frames;
	int          count;
} t2g_frame_list_t;

void t2g_frame_list_free(t2g_frame_list_t *list);

/* ── File-based output ─────────────────────────────────────────────── */

typedef struct output_ctx_t output_ctx_t;

/* Determine format from t2g settings or filename extension. */
output_format_t output_detect_format(t2g_t *root, const char *filename);

output_ctx_t *output_create(const char *filename, output_format_t fmt,
                             int width, int height);

/* output_create variant for in-memory frames: pass NULL for filename. */
output_ctx_t *output_create_frames(t2g_frame_list_t *list,
                                    int width, int height);

/* Add one frame.  delay_ms is the frame duration in milliseconds. */
void output_add_frame(output_ctx_t *ctx, cairo_surface_t *surface,
                      int delay_ms);

/* Finalise and write the file.  Returns 0 on success. */
int output_finish(output_ctx_t *ctx);

void output_destroy(output_ctx_t *ctx);

/* High-level entry points. */
int write_output(t2g_t *t2g, const char *filename);
int write_frames(t2g_t *t2g, t2g_frame_list_t *out);

#endif /* _T2G_OUTPUT_H_ */
