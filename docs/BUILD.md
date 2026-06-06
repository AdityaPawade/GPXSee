# BUILD — Building GPXSee (vcpkg + CMake + Qt5)

This fork builds with **CMake + vcpkg + MSVC**, reusing the **Qt 5.15.x** and
**OpenSSL 3.x** already present in the local vcpkg. OpenSSL is therefore a managed
dependency and is deployed automatically — **no manual DLL copying**.

> The original `gpxsee.pro` (qmake) build still works; this is an additional,
> dependency-managed path. Qt6 is intentionally **not** used.

## Prerequisites

| Tool | Purpose |
|---|---|
| Visual Studio 2022 (MSVC 14.4x) | `cl.exe` for the x64 build |
| vcpkg | provides `qt5-base` (5.15.x) + modules and `openssl` (3.x) |
| CMake ≥ 3.21 | configure/build |
| Ninja | optional |

The vcpkg toolchain path and triplet are pinned in **`CMakePresets.json`**
(`vcpkg-msvc`, triplet `x64-windows`, `VCPKG_MANIFEST_MODE=OFF`). Adjust the
`CMAKE_TOOLCHAIN_FILE` path there to your vcpkg location.

## Dependencies (`vcpkg.json`)

`qt5-base, qt5-svg, qt5-imageformats, qt5-multimedia, qt5-serialport,
qt5-location, openssl`. These resolve to versions already installed in the local
vcpkg, so `qt5-base` is **not** rebuilt.

### One-time: install any module the local vcpkg doesn't already have

`qt5-serialport` and `qt5-location` build in minutes against the existing
`qt5-base`. If the local vcpkg pins a `pkgconf` whose msys2 mirror version has
been rotated out (download 404), pass a pkgconf already on disk through the
build environment:

```bash
export VCPKG_ROOT="<your vcpkg>"
export PKG_CONFIG="$VCPKG_ROOT/downloads/tools/msys2/<hash>/mingw64/bin/pkgconf.exe"
export VCPKG_KEEP_ENV_VARS="PKG_CONFIG"
"$VCPKG_ROOT/vcpkg.exe" install qt5-serialport:x64-windows qt5-location:x64-windows
```

## Configure + build

```powershell
cmake --preset vcpkg-msvc
cmake --build build-cmake --config Release
```

Output: `build-cmake\Release\GPXSee.exe`.

## Deployment — what lands next to the exe (automatic)

* **vcpkg applocal** (post-link) copies every runtime DLL — `Qt5*.dll`,
  `libssl-3-x64.dll`, `libcrypto-3-x64.dll`, `zlib1.dll`, … — plus the Qt plugins
  (`plugins/platforms`, `sqldrivers`, `imageformats`, `styles`, `position`) and a
  `qt.conf`.
* A **POST_BUILD** step copies **`viz.cfg`** next to the exe.

So `build-cmake\Release\` is a runnable, self-contained folder. `cmake --install`
(prefix `deploy/`) produces the same layout for distribution.

## Run

```powershell
build-cmake\Release\GPXSee.exe track1.gpx track2.gpx
```

Then **Map → OpenStreetMap** once (online map source lives in
`%APPDATA%\GPXSee\maps\`; tiles need internet).

## Two design notes

1. **Why classic mode, not pure manifest-install.** A pure manifest install can
   rebuild *all of Qt from source* when the binary-cache ABI hashes no longer
   match the current MSVC toolset. To reuse what's installed, the CMake build
   consumes the already-installed vcpkg tree (`VCPKG_MANIFEST_MODE=OFF`);
   `vcpkg.json` remains the declared source of truth.
2. **One stock-source change.** `src/common/util.cpp` included Qt's bundled
   `<QtZlib/zlib.h>` on Windows; vcpkg's Qt links the **standalone** zlib instead,
   so the include is now `<zlib.h>` and `ZLIB::ZLIB` is linked. Functionally
   identical.

## Troubleshooting

| Symptom | Cause / fix |
|---|---|
| Blank map, log says `TLS initialization failed` | OpenSSL DLLs not beside the exe — rebuild so vcpkg applocal redeploys |
| `Cannot open include file: 'QtZlib/zlib.h'` | Stale `util.cpp` — must include `<zlib.h>` (note 2) |
| vcpkg build dies on `pkgconf … 404` | Set `PKG_CONFIG` + `VCPKG_KEEP_ENV_VARS=PKG_CONFIG` (above) |
| `qt5-base` wants to rebuild (long) | You're in manifest mode — ensure `VCPKG_MANIFEST_MODE=OFF` |
