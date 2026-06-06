# CONFIG — `viz.cfg` reference

Every **display semantic** used by the telemetry overlay (radar-mode
values→names, FOV geometry, weapon threshold, overlay colours, discrete-state
labels) is read from `viz.cfg`. **Nothing is hard-coded** in the binary — if the
file is absent the built-in defaults below apply, and any single key you set
overrides just that one.

## Location & precedence

Loaded by `src/GUI/vizconfig.cpp` at first use, later files overriding earlier:

1. built-in defaults
2. `<GPXSee.exe directory>\viz.cfg`  — shipped, copied here by the build
3. `%APPDATA%\GPXSee\viz.cfg`        — per-user override (hand-edit this)

Restart GPXSee after editing.

## Syntax

```ini
; or #         comment
[section]      section header
key = value    setting
```
Colours are `R,G,B` or `R,G,B,A` (each 0–255). A radar-mode line is
`value = name | R,G,B` (the colour part is optional).

## Keys

### `[radar]`
| Key | Default | Meaning |
|---|---|---|
| `fov_range_m` | `55560` | Length of the FOV wedge on the map, metres (~30 NM) |
| `scan_default_deg` | `60` | Sector width drawn when the track's scan width is absent |
| `on_min_code` | `0` | Radar is "on" when mode value > this |
| `default_color` | `120,120,120` | Wedge colour for modes not in `[radar_modes]` |
| `active_color` | `0,150,0` | Live-Stats "Radar mode" text colour when on |

### `[radar_modes]`  — `value = name | R,G,B`
| Value | Default name | Default colour |
|---|---|---|
| `0` | `OFF` | grey |
| `64` | `RWS (search)` | `40,120,255` |
| `68` | `TWS (track)` | `255,170,0` |
| `72` | `STT (lock)` | `230,30,30` |
| `76` | `Combat` | `200,0,120` |

The value is read verbatim from each track's `<radar_mode>` extension. Add a
line to support another value; unknown values show as `0x..` in grey.

### `[weapon]`
| Key | Default | Meaning |
|---|---|---|
| `engaged_threshold` | `1` | Live-Stats shows the engaged label when `<weapon>` ≥ this |
| `engaged_color` | `200,0,0` | colour of the engaged label |
| `engaged_label` | `ENGAGED` | text when engaged |
| `idle_label` | `-` | text when not engaged |

### `[contact]`
| Key | Default | Meaning |
|---|---|---|
| `target_color` | `230,30,30` | contact crosshair colour |
| `target_radius_px` | `7` | crosshair radius, pixels |

### `[labels]` — discrete-state interpretations in Live Stats
| Key | Default | | Key | Default |
|---|---|---|---|---|
| `gear_down` | `DOWN` | | `gear_up` | `up` |
| `wow_ground` | `ON GROUND` | | `wow_air` | `airborne` |
| `slats_out` | `OUT` | | `slats_in` | `in` |

## Example overrides

```ini
[radar]
fov_range_m = 92600          ; ~50 NM
[radar_modes]
72 = STT (LOCK) | 255,0,0
[weapon]
engaged_label = FIRING
```

## What is *not* in this file (deliberately)

Pure implementation constants that are not display semantics stay in code: the
WGS84 radius used for the great-circle projection, the Qt scene draw-order
`Z_VALUE`, and the wedge polygon step count.
