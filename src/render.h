#ifndef _T2G_RENDER_H_
#define _T2G_RENDER_H_

#include <cairo/cairo.h>
#include "t2g.h"

#define FRAMES_PER_ITEM       12
#define CONNECTOR_H           85   /* px from timeline to label anchor */
#define DOT_RADIUS             7   /* event dot radius in px */
#define DEFAULT_ICON_SIZE     28   /* icon size when event.image_size is unset */
#define DEFAULT_LABEL_IMAGE_SIZE 60  /* label image size when event.label_image_size is unset */

/* World-space x position for an event (respects ev->x_pos override). */
double render_event_world_x(t2g_t *root, t2g_t *ev, int index);

/* Target camera_x so the event lands at 40% of screen width. */
double render_camera_target(t2g_t *root, t2g_t *ev, int index);

/* Render one animation frame; returns a new ARGB32 surface (caller destroys). */
cairo_surface_t *render_frame(t2g_t *root,
                               t2g_t *first_event,
                               int    committed_count,
                               int    current_index,
                               int    frame,
                               double camera_x_start,
                               double camera_x_end);

/* Render a static frame with all committed events and no animating event.
   Used for fast-transit and transition compositing. */
cairo_surface_t *render_transit_frame(t2g_t *root,
                                       t2g_t *first_event,
                                       int    committed_count,
                                       double camera_x);

/* Render a callout overlay frame: committed events (dimmed) + centred callout box.
 *   ev     - the event being previewed (label and time text are read from it)
 *   fade   - 0 = invisible, 1 = fully opaque
 *   exit_t - 0 during fade-in/hold; 0→1 during fade-out for exit effects
 *            (funnel / zoom / float — driven by callout.effect setting)
 */
cairo_surface_t *render_callout_frame(t2g_t *root,
                                       t2g_t *first_event,
                                       int    committed_count,
                                       double camera_x,
                                       t2g_t *ev,
                                       double fade,
                                       double exit_t);

/* Composite two frames for a between-event transition.
 *   from, to   - fully rendered source frames (not modified)
 *   t          - progress 0..1
 *   style      - "fade", "wipe", "dissolve"  (NULL/"none" → fade)
 *   block_size - pixel size of dissolve blocks (0 → default 8)
 */
cairo_surface_t *render_transition_frame(cairo_surface_t *from,
                                          cairo_surface_t *to,
                                          double t,
                                          const char *style,
                                          int block_size,
                                          int width, int height);

#endif /* _T2G_RENDER_H_ */
