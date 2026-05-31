# timeline2gif — file format reference

timeline2gif reads a plain-text `.tig` file that declares global settings and
a list of timeline events, then produces an animated file in GIF, WebP, or
APNG format.

---

## Usage

```
timeline2gif  input.tig  output.gif
timeline2gif  input.tig  output.webp
timeline2gif  input.tig  output.apng
```

The output format is chosen automatically from the file extension.  
You can also override it explicitly inside the `.tig` file — see
[`output.format`](#outputformat) below.

---

## File structure

A `.tig` file is a sequence of:

1. **Comments** — lines starting with `#`
2. **Settings** — `group.key value`
3. **Events** — pairs of quoted strings `"time" "label"`

Settings can appear in any order before, between, or after events.  
Unknown settings are silently ignored, so you can add your own comments with
`# …` freely.

```
# This is a comment

image.width  900
theme.accent argb(255,255,100,0)

"Jan 2020" "Project kickoff"
"Mar 2020" "First release"
```

---

## Colors — the `argb()` format

Every color value uses the form:

```
argb(alpha, red, green, blue)
```

Each component is an integer **0–255**.

| Component | Meaning |
|-----------|---------|
| `alpha`   | Opacity: **255 = fully opaque**, 0 = fully transparent |
| `red`     | Red channel |
| `green`   | Green channel |
| `blue`    | Blue channel |

**Examples**

```
argb(255, 0, 0, 0)          # opaque black
argb(255, 255, 255, 255)    # opaque white
argb(255, 0, 180, 216)      # sky blue (default accent)
argb(128, 255, 0, 0)        # semi-transparent red
```

> Note: the `argb()` literal must have **no spaces** inside the parentheses.  
> Use `argb(255,0,180,216)` — not `argb(255, 0, 180, 216)`.

---

## Settings reference

### Canvas

| Setting | Type | Default | Description |
|---------|------|---------|-------------|
| `image.width` | integer | `800` | Output image width in pixels |
| `image.height` | integer | `500` | Output image height in pixels |

```
image.width  1200
image.height 400
```

---

### Timeline line

| Setting | Type | Default | Description |
|---------|------|---------|-------------|
| `timeline.position` | integer | `height / 2` | Y coordinate of the horizontal timeline line |
| `timeline.color` | argb | same as `theme.accent` | Color of the timeline line (also sets `theme.accent`) |

```
timeline.position 200       # line sits in the top third
timeline.color argb(255,0,180,216)
```

---

### Theme

All four theme settings accept an `argb()` color value.

| Setting | Default | Description |
|---------|---------|-------------|
| `theme.background`  | `argb(255,22,33,62)`   | Background gradient — top color |
| `theme.background2` | `argb(255,10,25,47)`   | Background gradient — bottom color |
| `theme.accent`      | `argb(255,0,180,216)`  | Dots, connector lines, timeline glow |
| `theme.text`        | `argb(255,224,224,224)`| Label and time text color |

The background is always rendered as a vertical gradient from `theme.background`
(top) to `theme.background2` (bottom).  Set both to the same color for a flat
background.

```
# Light theme example
theme.background  argb(255,245,245,250)
theme.background2 argb(255,230,232,240)
theme.accent      argb(255,41,98,255)
theme.text        argb(255,30,30,40)
```

---

### Fonts

Fonts are specified as **Pango font family names** — the same names you would
use in CSS or a desktop text application.  The font is looked up from the
system font catalog at render time; no font files are embedded.

| Setting | Type | Default | Description |
|---------|------|---------|-------------|
| `description.font` | string | system sans-serif | Font family for event labels |
| `description.font_size` | integer | `13` | Label font size in points |
| `time.font` | string | system sans-serif | Font family for time strings |
| `time.font_size` | integer | `11` | Time font size in points |

Font family names that work on most systems:

| Platform | Reliable choices |
|----------|-----------------|
| macOS | `Arial`, `Helvetica Neue`, `Georgia`, `Menlo` |
| Linux | `Liberation Sans`, `DejaVu Sans`, `FreeSans` |
| Both | `Sans`, `Serif`, `Monospace` (generic Pango aliases) |

Font names that contain spaces must be **quoted**:

```
description.font      Arial
description.font_size 14

time.font      "Liberation Sans"
time.font_size 11
```

If neither `time.font` nor `description.font` is set, timeline2gif probes for
Arial, Liberation Sans, and DejaVu Sans in that order, then falls back to the
generic Pango family `Sans`.

---

### Animation speed

Speed values are in **centiseconds** (hundredths of a second).

| Setting | Type | Default | Description |
|---------|------|---------|-------------|
| `speed.frames`   | integer | `5`  | Delay between each animation frame (5 cs = 50 ms) |
| `speed.nextitem` | integer | `50` | Extra hold on the last frame before the next event starts (50 cs = 500 ms) |

Lower values play faster.  A value of `2` for `speed.frames` gives a snappy
animation; `10` gives a slow, deliberate one.

```
speed.frames   3    # quick animation
speed.nextitem 80   # long pause so viewers can read each event
```

---

### Layout

| Setting | Type | Default | Description |
|---------|------|---------|-------------|
| `item.spacing` | integer | `160` | Horizontal distance (px) between consecutive event positions in world space |

Larger values spread events further apart.  When camera scrolling is on, any
number of events can fit regardless of canvas width.

```
item.spacing 200
```

---

### Camera

| Setting | Values | Default | Description |
|---------|--------|---------|-------------|
| `camera.scroll` | `yes` / `no` | `yes` | Pan the viewport to reveal each event as it animates in |

When `yes`, the view smoothly tracks each new event using an ease-in-out curve.
When `no`, events are rendered at fixed screen positions starting from the left;
events that exceed the canvas width will be clipped.

```
camera.scroll yes   # default — recommended for long timelines
camera.scroll no    # static view — good for 3–4 events that all fit
```

**Fast transit for large time gaps**

When `event.x` is used to model real time proportions, gaps larger than
3 × `item.spacing` automatically trigger a fast camera sprint before the dot
animation starts.  The sprint duration scales with the gap size (2–10 extra
frames).  No extra configuration is needed — it activates automatically.

---

### Transitions

Between-event transitions blend the scene smoothly instead of cutting abruptly.

| Setting | Values | Default | Description |
|---------|--------|---------|-------------|
| `transition.style`  | `none` `fade` `wipe` `dissolve` | `none` | Style of transition played after each event's animation |
| `transition.frames` | integer | `8` | Number of frames the transition takes |

| Style | Effect |
|-------|--------|
| `none`    | No transition (hard cut to the next state) |
| `fade`    | Smooth alpha crossfade between old and new scene |
| `wipe`    | A rectangle sweeps left-to-right, revealing the new scene |
| `dissolve`| Random 20×20-pixel blocks of the new scene fill in over the old |

```
transition.style   dissolve
transition.frames  10
```

---

### Output format

| Setting | Values | Default | Description |
|---------|--------|---------|-------------|
| `output.format` | `gif` `webp` `apng` | auto | Override the output format regardless of the output filename extension |

When omitted, the format is detected from the output filename extension
(`.gif`, `.webp`, `.apng` / `.png`), defaulting to GIF if unrecognised.

```
output.format webp     # always produce WebP even if the filename ends in .gif
```

**Format comparison**

| Format | Color depth | Transparency | Typical size | Notes |
|--------|-------------|--------------|--------------|-------|
| GIF    | 256 colours per frame | 1-bit | large | universal, but colour-limited; gradients are dithered |
| WebP   | full colour (24-bit) | full alpha | ~10× smaller than GIF | best quality/size ratio; supported in all modern browsers |
| APNG   | full colour (24-bit) | full alpha | medium | PNG-compatible; lossless; falls back to static PNG in very old viewers |

---

## Events

Each event is a pair of **quoted strings** on a single line:

```
"time string" "label string"
```

The **time string** is displayed near the timeline line in a smaller font.  
The **label string** is displayed at the end of the connector line in a larger font.

Events appear in the order they are listed in the file.  Consecutive events
**alternate** above and below the timeline line automatically so that labels
never overlap:

```
event 0 → label above the timeline
event 1 → label below the timeline
event 2 → label above the timeline
…
```

Strings can span multiple lines if you include a newline inside the quotes:

```
"Mar 2020" "FIRST postponed,
CanSec went online"
```

Both single quotes (`'`) and double quotes (`"`) are accepted.  Backslash
escapes work inside strings:

```
"Q1 2020" "Revenue up 12\%"
```

---

## Per-event customization

Any of the following `event.*` settings can appear **immediately before** an
event's `"time" "label"` line.  They apply only to that one event, then reset
to defaults for the next one.

### Colors

| Setting | Type | Description |
|---------|------|-------------|
| `event.dot_color`      | argb | Color of the dot (SVG icons use their own fill colors) |
| `event.text_color`     | argb | Color of both the label and time text for this event |
| `event.line_color`     | argb | Color of the vertical connector line |
| `event.timeline_color` | argb | Color of the **main timeline line segment** leading into this event (from the previous event up to this one); resets to default `theme.accent` for subsequent events |
| `event.background`     | argb | **New background gradient top color** from this event onward — persists for all subsequent events until another `event.background` overrides it |
| `event.background2`    | argb | Background gradient bottom color (use with `event.background` for a gradient; if omitted, both stops are the same color) |

`event.timeline_color` lets you highlight a stretch of the timeline — for example, painting it red during an active incident and green once resolved:

```
# Amber milestone dot with matching connector
event.dot_color  argb(255,255,185,0)
event.line_color argb(255,200,140,0)
"Q2 2020" "Milestone reached"

# Red incident — dot, text, connector, and the line segment all turn red
event.dot_color      argb(255,220,60,60)
event.text_color     argb(255,255,140,120)
event.line_color     argb(255,200,50,50)
event.timeline_color argb(255,200,60,60)
"Q3 2020" "Production outage"

# Recovery — green segment from the incident to now
event.dot_color      argb(255,60,180,100)
event.timeline_color argb(255,40,160,80)
"Q4 2020" "Recovery complete"
# Timeline line returns to theme.accent for subsequent events automatically
```

---

### Custom x position

| Setting | Type | Description |
|---------|------|-------------|
| `event.x` | integer (px) | Explicit world-space x position, overriding sequential placement |

By default events are placed at `(index + 1) × item.spacing` pixels from the
origin.  Set `event.x` to break that sequence — for instance to model a real
time gap.

**Modelling a timeline with real proportions:**

Set `item.spacing` to the pixel width of your base time unit (e.g. 1 month),
then use `event.x` to place each event at `months_from_start × item.spacing`:

```
item.spacing 20      # 1 unit = 1 month, so 20 px/month

"Jan 2020" "Start"          # default x = 1 × 20 = 20

event.x 60                  # month 3
"Mar 2020" "Quick follow-up"

event.x 240                 # month 12 = 1 year later
"Jan 2021" "Annual review"

event.x 480                 # month 24 = 2 years later
"Jan 2022" "Expansion"
```

The camera scrolls smoothly to each event regardless of gap size — it
accelerates over large gaps and decelerates as it arrives, making the time
distance perceptible to viewers.

---

### Image instead of dot

| Setting | Type | Default | Description |
|---------|------|---------|-------------|
| `event.image`      | quoted path | — | Path to a PNG or SVG file to draw instead of the standard dot |
| `event.image_size` | integer (px) | `28` | Diameter of the icon in pixels |

The image is centered on the timeline position, scaled uniformly to
`event.image_size` pixels, and plays the same elastic appear animation as a
normal dot.

Both **PNG** and **SVG** files are supported.  SVG files are rendered at full
vector quality at any size.  File paths that contain `/` or spaces must be
**quoted**.

```
# SVG icon
event.image      "samples/icons/star.svg"
event.image_size 28
"Jan 2022" "Award received"

# PNG icon
event.image      "icons/logo.png"
event.image_size 32
"Mar 2022" "Partnership announced"
```

> Relative image paths are resolved relative to the **`.tig` file's own
> directory**, so they work correctly regardless of which directory you run
> `timeline2gif` from.

---

## Complete example

```
# ── Canvas ──────────────────────────────────────────────────
image.width  900
image.height 420

# ── Theme ───────────────────────────────────────────────────
theme.background  argb(255,15,15,25)
theme.background2 argb(255,8,10,20)
theme.accent      argb(255,90,200,250)
theme.text        argb(255,220,220,230)

# ── Timeline line ────────────────────────────────────────────
timeline.position 210

# ── Fonts ────────────────────────────────────────────────────
description.font_size 14
time.font_size        10

# ── Animation ────────────────────────────────────────────────
speed.frames   4
speed.nextitem 70
item.spacing   200
camera.scroll  yes

# ── Output ───────────────────────────────────────────────────
output.format webp

# ── Events ───────────────────────────────────────────────────
"Jan 2020" "Project kickoff"
"Mar 2020" "Alpha release"
"Jun 2020" "Public beta"
"Sep 2020" "v1.0 shipped"
```

Run it:

```
timeline2gif  my-timeline.tig  my-timeline.webp
```

---

## Quick reference card

```
# Comments
# Any line starting with # is ignored

# Canvas
image.width   <px>
image.height  <px>

# Timeline
timeline.position  <y-px>
timeline.color     argb(A,R,G,B)

# Theme
theme.background   argb(A,R,G,B)   # gradient top
theme.background2  argb(A,R,G,B)   # gradient bottom
theme.accent       argb(A,R,G,B)   # dots, line, connectors
theme.text         argb(A,R,G,B)   # all text

# Fonts (Pango family names)
description.font       FamilyName
description.font_size  <pt>
time.font              FamilyName
time.font_size         <pt>

# Speed (centiseconds = 1/100 s)
speed.frames    <cs>
speed.nextitem  <cs>

# Layout
item.spacing  <px>

# Camera
camera.scroll  yes | no

# Transitions (between events)
transition.style   none | fade | wipe | dissolve
transition.frames  <count>   # default 8

# Output
output.format  gif | webp | apng

# Events (plain)
"time label" "description label"

# Events with per-event overrides (place before the event line; reset after)
event.dot_color       argb(A,R,G,B)      # dot / icon accent color
event.text_color      argb(A,R,G,B)      # label + time text color
event.line_color      argb(A,R,G,B)      # connector line color
event.timeline_color  argb(A,R,G,B)      # main timeline segment into this event
event.background      argb(A,R,G,B)      # new background (top gradient) from here on
event.background2     argb(A,R,G,B)      # new background bottom gradient (optional)
event.x           <world-px>         # explicit x position (0 = sequential)
event.image       "path/to/file.svg" # PNG or SVG instead of dot
event.image_size  <px>               # icon diameter (default 28)
"time" "label"
```
