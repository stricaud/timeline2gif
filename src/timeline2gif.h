#ifndef _TIMELINE2GIF_H_
#define _TIMELINE2GIF_H_

#include <t2g.h>

#include "output.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Parse `input_tig` and write the animation to `output_path`.
 * Returns 0 on success, non-zero on failure. */
int t2g_generate(const char *input_tig, const char *output_path);

/* Parse `input_tig` and render all frames as full-color PNG bytes in memory.
 * Caller must call t2g_frame_list_free(out) when done. */
int t2g_generate_frames(const char *input_tig, t2g_frame_list_t *out);

#ifdef __cplusplus
}
#endif

#endif // _TIMELINE2GIF_H_
