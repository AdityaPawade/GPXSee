# GPXSee — Telemetry-Overlay Fork

A customised build of **GPXSee** that renders **rich per-point telemetry carried
in GPX tracks** and lays it over the map, in addition to everything stock GPXSee
does. Useful for vehicle/aircraft logs whose `<trkpt>`s carry extra fields beyond
position and elevation.

## What this fork adds

GPX allows arbitrary data inside `<trkpt><extensions>…</extensions>`. This fork
parses a set of telemetry fields from there and presents them three ways:

| Feature | What it shows | Source files |
|---|---|---|
| **Live Stats dock** | A table of every telemetry value at the current timeline-slider instant — heading/pitch/roll, airspeed, vertical speed, roll/yaw rate, radar mode + scan, contact bearing/range, nav bearing/range, fuel, gear, weight-on-wheels, auto-slats, weapon flag | `src/GUI/livestatswidget.*` |
| **Radar FOV overlay** | A scan-sector wedge from the vehicle's nose, coloured by radar mode and sized by scan width | `src/GUI/radaroverlayitem.*` |
| **Target overlay** | A crosshair at a contact position, projected from the vehicle using the contact bearing + range fields | `src/GUI/radaroverlayitem.*` |

Recognised extension tags (all optional, per `<trkpt>`):

```
roll pitch yaw airspeed vspeed roll_rate yaw_rate fuel_pct
nav_bearing nav_range radar_mode radar_scan_width
contact_bearing contact_range weapon gear wow auto_slats
```

Everything is driven by the parsed values, and **all interpretation constants
live in `viz.cfg`** (see **CONFIG.md**) — nothing is hard-coded in the binary.

## Documentation map

| Doc | Purpose |
|---|---|
| **OVERVIEW.md** (this file) | What the fork does |
| **BUILD.md** | How to build (vcpkg + CMake + Qt5) and run |
| **GUIDE.md** | Developer code-map: how an extension tag reaches the screen; how to add a field |
| **CONFIG.md** | Every `viz.cfg` key, its default and meaning |

## Quick start

```powershell
cmake --preset vcpkg-msvc
cmake --build build-cmake --config Release

build-cmake\Release\GPXSee.exe track1.gpx track2.gpx
```

Then **Map → OpenStreetMap** (once), scrub the timeline, and the Live Stats dock
+ radar/target overlays update at each instant. The basemap needs internet for
tiles.

## Relationship to upstream GPXSee

The upstream qmake project (`gpxsee.pro`) is untouched and still builds. This fork
adds a parallel **CMake + vcpkg** build (`CMakeLists.txt`, `vcpkg.json`,
`CMakePresets.json`), the telemetry-overlay sources, and `viz.cfg`. The only
change to a stock source file is one `#include` in `common/util.cpp` (see
BUILD.md).
