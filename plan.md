# Gloom PC Port Plan (Updated)

## Scope
- Keep all legacy Amiga code and assets under amiga/ as immutable source material.
- Build a new PC runtime in C17 with SDL2, fetched from source via CMake FetchContent.
- Start with parity foundations: deterministic 50 Hz update, map parser, and bootable window loop.

## Completed
- Legacy repository content moved under amiga/.
- Map header endianness validated on amiga/maps/map1_1 (big-endian 32-bit offsets).
- Initial zone record layout validated from real bytes (32-byte zone records).
- Root CMake bootstrap created with SDL2 fetched from source via FetchContent (no SDL binaries).
- New C17 codebase scaffolded in src/ with map parser and fixed-step SDL app loop.
- Full configure + Debug build succeeds and produces build/Debug/gloom_pc.exe.
- SDL window now renders map-derived zone wireframe debug view from parsed map geometry.
- Sweep mode added (`--sweep`) to validate map parsing across directories.
- Sweep validated current parser coverage: 42 raw maps parsed, 42 CrM2 container maps detected and skipped.
- IFF parser module added for FORM/ILBM metadata (`BMHD`/`CMAP`/`BODY`) with CLI inspection mode (`--iff-info`).
- IFF parser validated on amiga/combat.iff (320x240, 7 planes, 128-color palette, compressed BODY).
- ILBM BODY decode implemented (ByteRun1 + planar-to-indexed conversion for modes masking=0/1/2).
- SDL IFF preview mode added (`--iff-preview`) and validated on amiga/combat.iff.
- Main map debug window now includes an integrated decoded texture panel (map + texture visible together).
- First software wall debug pass added to main window (camera orbit + projected zone walls + texture sampling).
- Software wall panel upgraded with horizon/floor depth shading and first billboard sprite/debug-object pass.
- Wall pass now clips segments against side frustum planes to reduce projection artifacts.
- Sprite pass now depth-sorts billboards and applies simple frame stepping for visible animation.
- CrM2 map container decompression implemented from the original Crunch-Mania LZH decruncher semantics.
- Sweep now validates both raw and compressed map directories: 84 maps parsed, 0 skipped, 0 failed.
- Map parser now exposes the real 32x32 wall/trigger grid cells from the map header grid block.
- Wall debug pass now uses the original `gridoffs4.bin` search order plus grid/ppnt wall candidates instead of drawing every zone segment.
- Main viewport now starts from the event-1 player spawn and renders full-screen wall columns from real map grid candidates using the zones' real texture slot/scale data.
- Main runtime now treats missing named texture screens or zone references to unloaded texture screens as fatal startup errors.
- Main viewport now derives the map's `tile_` tag from the original script and renders perspective floor/roof flats from the real `txts/floor*` and `txts/roof*` tile assets.
- Event parsing now preserves real open-door and texture-change commands, validates command texture references, and the runtime triggers events from the map trigger grid using the player collision radius read from the real `objs/player` asset.
- Door events now animate the target wall geometry and `zo_open` word in the 50 Hz update path, including the Amiga wall-column open-threshold scale and fixed-point endpoint slide behavior.
- Wall texture loading now keeps the original per-column transparency byte and skips transparent texels for see-through wall strips instead of treating every 65-byte texture column as opaque.
- Wall rendering now gathers multiple ray hits per column and draws them front-to-back per pixel, so transparent wall texels reveal farther real walls/flats instead of terminating the ray at the nearest strip.
- Object spawns now bind through the original object type table to real external `objs/*` animation assets, decode their packed Amiga palettes and column-major frames, and render supported map objects with their source handles, scales, and rotation frames instead of the old `rot*.iff` debug smoke pass.
- Embedded weapon pickup object types now resolve to the original `bullet1.bin` through `bullet5.bin` animation payloads, so first-map weapon spawns render from the real binary include data instead of being skipped.
- Renderer draw distance now uses the original Amiga `maxz` cutoff (`16 << darkshft` = 2048 view-Z units) for floors, walls, and objects, with the reversed `initdarktable` depth lookup and 4-bit per-channel palette subtraction so the far band fades to black before culling.
- Floor/roof tile sampling now matches the Amiga flat renderer's column-major address order `((world_x & 127) << 7) + (world_z & 127)` and uses the same fixed `focshft=6` projection scale as walls, fixing flat scroll/parallax against wall movement.
- Event parsing and runtime execution now preserve teleport and rotpoly commands; teleport events use the original delayed pixel-out timing before moving the player, and rotating/morphing polygon events mutate live zone geometry each 50 Hz tick using the Amiga `dorots`/`exec_rotpolys` data model.
- Player wall collision now follows the Amiga `checknewslow`/`adjustpos` path: movement scans the wall grid plus active rotpolys, uses the real player radius from `objs/player`, pushes along the nearest wall normal twice, and reverts/stalls when unresolved.
- Player control now uses the Amiga `rotplayer`/`moveplayer` constants for rotation acceleration, movement speed, and movement bob, while adding modern relative mouse look and WASD strafe/forward input on the same 50 Hz player state.
- Widescreen rendering now adapts the viewport width to the renderer shape, scales projection focal from the Amiga large-window 106x80 chixel/`focshft=6` reference so FOV matches the original instead of the full 320-pixel display, and keeps `--classic` for regression checks.

## In Progress
- Deterministic parity harness and runtime object/combat behavior after the renderer/event vertical slice.

## Milestones
1. Bootstrap and Compile
- Root CMakeLists.txt builds executable and fetches SDL2 source.
- App starts window and runs deterministic fixed-step loop.
- Status: complete.

2. Data Ingestion Baseline
- Parse map header, grid block, zones, point table blob, animations, texture name table, and event scripts.
- Validate parser against all files in amiga/maps and amiga/data/maps.
- Status: raw and CrM2-compressed map sweep complete.

3. Vertical Slice Foundation
- Load map1_1 and render debug visualization with player movement/collision scaffold.
- Preserve 50 Hz simulation behavior.
- Status: first visible debug build running, improving rendering fidelity now.

## Immediate Next Tasks
1. Introduce deterministic input replay harness for parity testing.
2. Wire projectile/spark runtime objects to the existing embedded bullet/spark payloads when combat logic comes online.
3. Implement lock-style teleport event behavior once player lock/control state is represented.
