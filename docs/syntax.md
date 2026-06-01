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
[`output.format`](#output-format) below.

---

## File structure

A `.tig` file is a sequence of:

1. **Comments** — lines starting with `#`
2. **Settings** — `group.key value`
3. **Events** — pairs of quoted strings `"time" "label"`

Settings can appear in any order before, between, or after events.  
Unknown settings are silently ignored.

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
argb(255,0,0,0)          # opaque black
argb(255,255,255,255)    # opaque white
argb(255,0,180,216)      # sky blue (default accent)
argb(128,255,0,0)        # semi-transparent red
```

> Note: the `argb()` literal must have **no spaces** inside the parentheses.

---

## Settings reference

### Canvas

| Setting | Type | Default | Description |
|---------|------|---------|-------------|
| `image.width` | integer | `800` | Output image width in pixels |
| `image.height` | integer | `500` | Output image height in pixels |

---

### Timeline line

| Setting | Type | Default | Description |
|---------|------|---------|-------------|
| `timeline.position` | integer | `height / 2` | Y coordinate of the horizontal timeline line |
| `timeline.color` | argb | same as `theme.accent` | Color of the timeline line |

---

### Theme

| Setting | Default | Description |
|---------|---------|-------------|
| `theme.background`  | `argb(255,22,33,62)`    | Background gradient — top color |
| `theme.background2` | `argb(255,10,25,47)`    | Background gradient — bottom color |
| `theme.accent`      | `argb(255,0,180,216)`   | Dots, connector lines, timeline glow |
| `theme.text`        | `argb(255,224,224,224)` | Label and time text color |

Set both `theme.background` and `theme.background2` to the same color for a flat background.

---

### Fonts

Fonts are specified as **Pango font family names**.

| Setting | Type | Default | Description |
|---------|------|---------|-------------|
| `description.font` | string | system sans-serif | Font family for event labels |
| `description.font_size` | integer | `13` | Label font size in points |
| `time.font` | string | system sans-serif | Font family for time strings |
| `time.font_size` | integer | `11` | Time font size in points |

Font names with spaces must be quoted: `time.font "Liberation Sans"`

---

### Animation speed

Speed values are in **centiseconds** (1 cs = 1/100 s).

| Setting | Type | Default | Description |
|---------|------|---------|-------------|
| `speed.frames`     | integer | `5`  | Delay between animation frames |
| `speed.nextitem`   | integer | `50` | Hold on the last frame of each event before moving to the next |
| `speed.loop_pause` | integer | `0`  | Extra hold on the very last frame before the animation loops (0 = none) |

`speed.loop_pause` gives viewers time to read the completed timeline before it restarts.
A value of `1000` (= 10 seconds) is good for dense timelines.

```
speed.frames     4
speed.nextitem   60
speed.loop_pause 800    # 8-second pause at the end
```

---

### Layout

| Setting | Type | Default | Description |
|---------|------|---------|-------------|
| `item.spacing` | integer | `160` | World-space pixels between consecutive event positions |

---

### Time-based auto-positioning

By default events are spaced evenly at `item.spacing` intervals. Set
`time.format` to instead derive each event's x position automatically
from its time string — no `event.x` needed.

| Setting | Values | Default | Description |
|---------|--------|---------|-------------|
| `time.format` | `year` `number` | — (sequential) | How to parse the time string into a numeric value |
| `time.origin` | integer | auto | Reference time value that maps to x = `item.spacing`; defaults to the first event's parsed value |

| Format | What it parses |
|--------|---------------|
| `year`   | First 4-digit number in the range 1000–9999 (e.g. `"Jan 1990"` → `1990`) |
| `number` | First numeric token in the string (e.g. `"Phase 3"` → `3`) |

```
time.format  year
item.spacing 48          # 48 px per year

"1990" "Project launch"
"1993" "First milestone"    # placed 3 × 48 = 144 px after the first event
"2001" "Major release"      # 11 × 48 = 528 px after first event (large gap → fast transit)
```

`event.x` always takes precedence over auto-positioning if explicitly set.

**Fast transit for large gaps**

When two consecutive events are more than `3 × item.spacing` apart (whether
via auto-positioning or explicit `event.x`), the camera performs a brief sprint
to cross the gap before the next event animates in. This makes large time jumps
perceptible to the viewer without slowing down the rest of the animation.

---

### Camera

| Setting | Values | Default | Description |
|---------|--------|---------|-------------|
| `camera.scroll` | `yes` / `no` | `yes` | Pan the viewport to each event as it animates in |

---

### Transitions

Between-event transitions blend the scene instead of cutting abruptly.

| Setting | Values | Default | Description |
|---------|--------|---------|-------------|
| `transition.style`      | `none` `fade` `wipe` `dissolve` `pixelize` | `none` | Style played between events |
| `transition.frames`     | integer | `8` | Number of frames the transition takes |
| `transition.block_size` | integer (px) | `8` | Block size for `dissolve`/`pixelize` — smaller = finer grain |

| Style | Effect |
|-------|--------|
| `none`     | Hard cut |
| `fade`     | Alpha crossfade |
| `wipe`     | Rectangle sweeps left-to-right |
| `dissolve` | Random pixel blocks of the new scene fill in (alias: `pixelize`) |
| `pixelize` | Same as `dissolve` |

For `dissolve` / `pixelize`, the camera is stationary during the reveal so only
the new element pixelizes in — previously committed elements remain crisp.

---

### Callout spotlight

A callout displays a centered shape with the event's label *before* the event
joins the timeline. The shape fades in, pauses, then exits with an animated
effect.

| Setting | Values | Default | Description |
|---------|--------|---------|-------------|
| `callout.shape`  | `rectangle` `rounded` `cloud` | — (off) | Shape style; setting this enables callouts |
| `callout.effect` | `none` `funnel` `zoom` `float` | `none` | Exit animation when the callout fades out |
| `callout.pause`  | integer (cs) | `200` | How long to hold the callout (centiseconds) |
| `callout.color`  | argb | `theme.background` | Box fill color |
| `callout.border` | argb | `theme.accent` | Border / glow color |

| Shape | Appearance |
|-------|-----------|
| `rectangle` | Sharp-cornered box |
| `rounded`   | Softly curved box with 20 px corner radius |
| `cloud`     | Bumpy cloud silhouette with rounded bottom |

| Exit effect | What happens during fade-out |
|-------------|------------------------------|
| `none`      | Plain alpha fade, no movement |
| `funnel`    | Shape contracts toward the event dot on the timeline |
| `zoom`      | Shape scales to zero at its centre |
| `float`     | Shape drifts upward while fading |

Per-event override: `event.callout_effect` overrides `callout.effect` for that
one event, so a single file can showcase multiple exit styles.

```
callout.shape   rounded
callout.effect  funnel     # default exit for all events
callout.pause   150        # 1.5 seconds
callout.color   argb(255,10,14,35)
callout.border  argb(255,0,210,190)

# This event uses a different exit animation
event.callout_effect zoom
"1998" "Google founded"

# Back to the default (funnel)
"2004" "Facebook launched"
```

---

### Progress bar

An optional progress bar at the bottom of the canvas shows how far through
the timeline the animation has progressed.

| Setting | Values | Default | Description |
|---------|--------|---------|-------------|
| `progress.show`       | `yes` / `no` | `no` | Enable the progress bar |
| `progress.color`      | argb | `theme.accent` | Fill / foreground color |
| `progress.background` | argb | fill color at 12 % alpha | Track (unfilled) color |
| `progress.height`     | integer (px) | `4` | Bar thickness |

The bar advances smoothly as each event animates in, using the same easing
curve as the rest of the animation. A glowing leading-edge cap slides across
the canvas.

```
progress.show   yes
progress.color  argb(255,0,210,190)
progress.height 5
```

---

### Output format

| Setting | Values | Default | Description |
|---------|--------|---------|-------------|
| `output.format` | `gif` `webp` `apng` | auto from extension | Override the output format |

| Format | Color depth | Size | Notes |
|--------|-------------|------|-------|
| GIF    | 256 colours per frame | large | Universal compatibility |
| WebP   | Full colour (24-bit) | ~10× smaller than GIF | Best quality/size ratio; recommended |
| APNG   | Full colour (24-bit) | medium | Lossless; falls back to static PNG in old viewers |

---

## Events

Each event is a pair of **quoted strings**:

```
"time string" "label string"
```

Events alternate above and below the timeline automatically so labels
don't overlap. Both single and double quotes are accepted.

---

## Per-event settings

Place any of these **immediately before** an event line. They apply to that
event only, then reset.

### Colors

| Setting | Type | Description |
|---------|------|-------------|
| `event.dot_color`      | argb | Dot / icon accent color |
| `event.text_color`     | argb | Label and time text color |
| `event.line_color`     | argb | Connector line color |
| `event.timeline_color` | argb | Color of the main timeline segment leading into this event |
| `event.background`     | argb | New background top gradient from this event onward (persists) |
| `event.background2`    | argb | New background bottom gradient |

### Position

| Setting | Type | Description |
|---------|------|-------------|
| `event.x` | integer (px) | Explicit world-space x position; always overrides auto-positioning |

### Icon

| Setting | Type | Default | Description |
|---------|------|---------|-------------|
| `event.image`      | quoted path | — | PNG or SVG to draw instead of the standard dot |
| `event.image_size` | integer (px) | `28` | Icon diameter |

Relative paths are resolved relative to the `.tig` file's own directory.

### Callout exit effect

| Setting | Values | Description |
|---------|--------|-------------|
| `event.callout_effect` | `none` `funnel` `zoom` `float` | Override `callout.effect` for this event only |

---

## Quick reference card

```
# Canvas
image.width   <px>
image.height  <px>

# Timeline
timeline.position  <y-px>
timeline.color     argb(A,R,G,B)

# Theme
theme.background   argb(A,R,G,B)
theme.background2  argb(A,R,G,B)
theme.accent       argb(A,R,G,B)
theme.text         argb(A,R,G,B)

# Fonts
description.font       FamilyName
description.font_size  <pt>
time.font              FamilyName
time.font_size         <pt>

# Speed (centiseconds = 1/100 s)
speed.frames      <cs>     # per-frame delay
speed.nextitem    <cs>     # hold after each event
speed.loop_pause  <cs>     # hold on the final frame before looping

# Layout
item.spacing  <px>

# Time-based auto-positioning (no event.x needed)
time.format   year | number
time.origin   <value>      # reference time → first event; default auto

# Camera
camera.scroll  yes | no

# Transitions
transition.style      none | fade | wipe | dissolve | pixelize
transition.frames     <count>
transition.block_size <px>

# Callout spotlight
callout.shape   rectangle | rounded | cloud
callout.effect  none | funnel | zoom | float
callout.pause   <cs>
callout.color   argb(A,R,G,B)
callout.border  argb(A,R,G,B)

# Progress bar
progress.show        yes | no
progress.color       argb(A,R,G,B)
progress.background  argb(A,R,G,B)
progress.height      <px>

# Output
output.format  gif | webp | apng

# Events
"time" "label"

# Per-event overrides (place immediately before the event line)
event.dot_color         argb(A,R,G,B)
event.text_color        argb(A,R,G,B)
event.line_color        argb(A,R,G,B)
event.timeline_color    argb(A,R,G,B)
event.background        argb(A,R,G,B)
event.background2       argb(A,R,G,B)
event.x                 <world-px>
event.image             "path/to/file.svg"
event.image_size        <px>
event.callout_effect    none | funnel | zoom | float
"time" "label"
```
