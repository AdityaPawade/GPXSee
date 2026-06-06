# GUIDE — Developer code map

How a value travels from a GPX `<extension>` tag to the screen, and how to add a
new one.

## Data flow

```
 GPX  <trkpt><extensions><yaw>…</yaw><radar_mode>…</radar_mode>…
   │
   │  (1) PARSE
   ▼
 src/data/gpxparser.cpp :: trkptExtensions()
   reads each tag → fills the TrackPoint's Telemetry struct
   │
   ▼
 src/data/telemetry.h :: struct Telemetry
   roll/pitch/yaw, airspeed, vspeed, rollRate/yawRate, fuelPct,
   navBearing/navRange, radarMode/radarScan, contactBearing/contactRange,
   weapon (qreal, NAN = absent) + gear/wow/autoSlats (int, -1 = absent)
   │
   │  (2) CARRY through the data model
   ▼
 src/data/trackpoint.h   TrackPoint::telemetry()
 src/data/track.cpp      Track::path()  → PathPoint(coords, distance, telemetry)
 src/data/path.h         PathPoint::telemetry()
   │
   │  (3) SAMPLE at the slider instant
   ▼
 src/GUI/pathitem.cpp :: setMarkerPosition()
   maps slider → distance → nearest PathPoint
   emits  markerTelemetry(name, coordinates, telemetry)
   │
   ├───────────────► src/GUI/mapview.cpp
   │                    re-emits MapView::markerTelemetry
   │                    updateRadarOverlay()  → RadarOverlayItem::setData()
   │                                            (FOV wedge + boresight + target)
   │
   └───────────────► src/GUI/gui.cpp connects MapView::markerTelemetry
                        → LiveStatsWidget::updateTelemetry()  (the dock table)
   │
   ▼  (4) INTERPRET — all display semantics from config, nothing hard-coded
 src/GUI/vizconfig.{h,cpp}  ← reads viz.cfg  (see CONFIG.md)
```

## File map

| File | Role | Origin |
|---|---|---|
| `src/data/telemetry.h` | `Telemetry` value struct (the parsed fields) | **new** |
| `src/data/gpxparser.cpp` | `trkptExtensions()` parses the custom tags | modified |
| `src/data/trackpoint.h` | holds a `Telemetry` per point | modified |
| `src/data/path.h` | `PathPoint` carries telemetry to the GUI | modified |
| `src/data/track.cpp` | `Track::path()` copies telemetry into `PathPoint` | modified |
| `src/GUI/pathitem.{h,cpp}` | emits `markerTelemetry` at the slider point | modified |
| `src/GUI/mapview.{h,cpp}` | owns `RadarOverlayItem`, relays telemetry | modified |
| `src/GUI/gui.{h,cpp}` | creates the Live Stats dock, wires the signal | modified |
| `src/GUI/livestatswidget.{h,cpp}` | the parameter table dock | **new** |
| `src/GUI/radaroverlayitem.{h,cpp}` | radar FOV wedge + target crosshair | **new** |
| `src/GUI/vizconfig.{h,cpp}` | loads `viz.cfg`; all interpretation constants | **new** |
| `viz.cfg` | the config file (shipped next to the exe) | **new** |

New `.cpp` files are picked up automatically by the `file(GLOB_RECURSE …
CONFIGURE_DEPENDS)` in `CMakeLists.txt` — no edit needed to add sources.

## Config rule (important)

**No display semantic may be hard-coded.** Radar-mode names/values, the FOV
range, scan default, weapon-engaged threshold, overlay colours and the
gear/WoW/slats labels all come from `VizConfig::instance()` (backed by
`viz.cfg`). Only pure implementation constants stay in code (the WGS84 radius
used for the great-circle projection, the scene `Z_VALUE`, the polygon step
count). See **CONFIG.md**.

## Recipe — add a new telemetry field

1. **Producer side**: whatever generates your GPX must emit the new tag inside
   `<trkpt><extensions>`.
2. **telemetry.h**: add a member (`qreal x = NAN;` or `int x = -1;`).
3. **gpxparser.cpp** `trkptExtensions()`: parse the new tag into the member.
4. **livestatswidget.{h,cpp}**: add a `RowXxx` enum value, a `ROW_LABELS`
   entry, and a `set(RXxx, …)` line in `updateTelemetry()`.
5. If it needs interpretation (values→names, thresholds, colours), add keys to
   **vizconfig.{h,cpp}** + **viz.cfg** — do **not** hard-code them.
6. Rebuild (`cmake --build build-cmake --config Release`).

The data-model plumbing (trackpoint→path→pathitem) forwards the whole
`Telemetry` struct, so new fields ride along for free.
