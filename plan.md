# Gloom PC Port Plan (Updated)

## Scope
- Keep all legacy Amiga code and assets under amiga/ as immutable source material.
- Build a new PC runtime in C17 with SDL2, fetched from source via CMake FetchContent.
- Start with parity foundations: deterministic fixed-step update, map parser, and bootable window loop, preserving Amiga
  25 Hz gameplay-tick motion rates where the PC runtime uses a different logic clock.
- Gameplay work is a port, not a redesign: do not add speculative mechanics or convenience AI. Every enemy/combat behavior must be traceable to `amiga/gloom2.s` or another original asset/source file; unported behavior should remain absent or be marked TODO instead of filled in.

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
- Door events now animate the target wall geometry and `zo_open` word in the fixed update path, including the Amiga wall-column open-threshold scale and fixed-point endpoint slide behavior.
- Wall texture loading now keeps the original per-column transparency byte and skips transparent texels for see-through wall strips instead of treating every 65-byte texture column as opaque.
- Wall rendering now gathers multiple ray hits per column and draws them front-to-back per pixel, so transparent wall texels reveal farther real walls/flats instead of terminating the ray at the nearest strip.
- Object spawns now bind through the original object type table to real external `objs/*` animation assets, decode their packed Amiga palettes and column-major frames, and render supported map objects with their source handles, scales, and rotation frames instead of the old `rot*.iff` debug smoke pass.
- Embedded weapon pickup object types now resolve to the original `bullet1.bin` through `bullet5.bin` animation payloads, so first-map weapon spawns render from the real binary include data instead of being skipped.
- Renderer draw distance now uses the original Amiga `maxz` cutoff (`16 << darkshft` = 2048 view-Z units) for floors, walls, and objects, with the reversed `initdarktable` depth lookup and 4-bit per-channel palette subtraction so the far band fades to black before culling.
- Floor/roof tile sampling now matches the Amiga flat renderer's column-major address order `((world_x & 127) << 7) + (world_z & 127)` and uses the same fixed `focshft=6` projection scale as walls, fixing flat scroll/parallax against wall movement.
- Event parsing and runtime execution now preserve teleport and rotpoly commands; teleport events use the original delayed pixel-out timing before moving the player, and rotating/morphing polygon events mutate live zone geometry using the Amiga `dorots`/`exec_rotpolys` data model.
- Player wall collision now follows the Amiga `checknewslow`/`adjustpos` path: movement scans the wall grid plus active rotpolys, uses the real player radius from `objs/player`, pushes along the nearest wall normal twice, and reverts/stalls when unresolved.
- Player control now uses the Amiga `rotplayer`/`moveplayer` constants for rotation acceleration, movement speed, and movement bob, while adding modern relative mouse look and WASD strafe/forward input on the same player state.
- Widescreen rendering now adapts the viewport width to the renderer shape, scales projection focal from the Amiga large-window 106x80 chixel/`focshft=6` reference so FOV matches the original instead of the full 320-pixel display, and keeps `--classic` for regression checks.
- The SDL runtime now runs gameplay on a fixed 60 Hz step with Amiga 25 Hz gameplay-tick motion/timer rates scaled for parity, renders uncapped, interpolates camera position/bob and live door/rotpoly zone geometry between logic steps, and applies mouse orientation every render frame.
- Player firing now follows the Amiga `checkfire` one-shot/reload cadence, spawns live runtime projectiles from the original `wtable` weapon speeds/damage values, and draws bullets and wall-hit sparks from the real `bullet*.bin`/`sparks*.bin` payloads.
- The Gloom Deluxe ADFs have been extracted under `build/adf_extract`; Disk 1 contains the missing real on-screen weapon payload `misc/gun.bin`, which is now copied into `amiga/misc/gun.bin`, decoded to `amiga/art/objects/misc__gun.bin`, and rendered as the player gun instead of inventing or scaling projectile art.
- The on-screen gun now uses the real `misc/gun.bin` frames for idle/recoil and muzzle flash, draws at integer logical-pixel coordinates, renders the muzzle flash behind the gun body, applies recoil while firing, and bobs in the slight smile-shaped left/right footstep cycle.
- Projectile spawning now defaults to a centered barrel-origin start point 2.5 projectile radii in front of the player, while preserving the legacy player-position path through `--player-projectiles`/`--legacy-projectiles` so gameplay changes remain testable.
- Barrel-origin projectiles carry their original player-origin coordinates too, ready for the future enemy-hit compatibility rule where a very close center-view enemy should still count if it would have been hit by the old start position.
- Runtime enemy combat state now mirrors the Amiga object table hitpoints, damage, movement speed, `ob_base`/`ob_range`, `ob_frame`, `ob_framespeed`, and `ob_hurtpause` fields for represented monsters; player projectiles sweep-test against live enemies before wall hits, and killed enemies are removed from sprite rendering.
- Runtime enemies now keep live position/facing state, use the generic Amiga `monsterlogic` movement/fire timing for object type 10, use `hurtobject`/`pauselogic2` hurt frame/pause timing from `ob_hurtpause`, and expose player health in the window title while death flow/HUD health polish remain future work.
- Enemy-owned projectiles now exist for the generic `monsterlogic`/`fire1` path using the Amiga bullet1 parameters; enemy bullets damage the player instead of monsters, and player HP now enters a dead state that freezes control and marks the window title as `DEAD`. Special monster firing routines are now represented separately from their original routines instead of sharing invented generic projectile behavior.
- Enemy movement cleanup removed non-Amiga line-of-sight/range gates, artificial pack separation, and placeholder deathhead charge behavior so future monster work stays tied to direct `gloom2.s` routine ports rather than invented substitutes.
- Generic enemy movement now follows the `monsterlogic`/`monsterfix` shape more directly: monsters move along their own `ob_rot`, tick an `ob_delay`-style timer initialized from `ob_base`/`ob_range`, turn to the player only for `fire1`, pause for 7 ticks after firing, and use the Amiga wall-hit turn sequence (+/-90, 180, oldrot+180) instead of direct chase steering.
- Generic enemy movement/collision now follows `checkvecs` more closely: blocked proposed movement reports a wall hit instead of using player-style wall push/slide, `fire1` applies its original `(rndw & 31)-16` aim noise, and `pauselogic` may restore `ob_oldrot` after firing using the original player-facing angle test.
- Baldy-family melee behavior now ports the shared `baldylogic`/`bl2`/`baldycharge`/`baldypunch` shape used by baldy, lizard, and troll: `ob_delay` gates charge, charge uses `ob_movspeed` and `ob_framespeed` multiplied by four, wall/angle loss returns through `monsterfix` plus `rnddelay`, and collision-gated punch frames apply `ob_damage` on the original `ob_punchrate` cadence.
- Deathhead cruise/charge movement now ports `deathheadlogic`, `deathcharge`, `deathbounce`, and `deathanim`: `ob_delay` is preserved as signed rotation speed, wall hits reverse and reseed through `rnddelay`, near-facing deathheads enter charge, charge exits on wall hit or wide angle, and the frame oscillator stays in the original `$8000..$28000` range.
- Special projectile monster logic now ports `terralogic`/`terralogic2`, `ghoullogic`, `phantomlogic`, and `demonlogic`/`demonpause`: terra fires five bullet4 shots on `ob_firerate`, ghoul bobs/fires bullet2 and ignores walls, phantom fires bullet3 then enters the original 7-tick pause, and demon fires the five-weapon `wtable` burst with the original 3/4 damage scaling.
- Dragon movement/firing now ports `dragonlogic`, `dragonanim`, `dragonfire`, `getobrot`, and the represented part of `homeinlogic`: dragon rotation uses `ob_rotspeed`, fire timing emits bullet5 homing projectiles during the negative `ob_delay` burst window, and homing bullets accelerate toward the player with the original per-axis speed cap.
- Dragon death now ports the represented `blowdragon`/`dragondead` shape: 64 `bloodymess2` splots, four `blowchunx` passes from `dragon2`, render/collision removal, and the original 127-tick death wait before loudly reporting that the `finished=3` end-game flow is still not ported.
- Deathhead death dispatch now preserves `blowdeath`: if the unported soul-suck owner state is not active it follows through to `blowobject`, matching the original non-sucking path instead of pretending the deathhead has a generic table death.
- Deathhead non-lethal hit behavior now ports `hurtdeath`, `deathsuck`, and `addsoul` using the original global `sucking`/`sucker` shape: one active deathhead suck owner, 64-tick suck duration, `deathbounce`/`deathanim` during the state, rotation toward the player each tick, four soul-colored blood-list particles per tick, and return through `rnddelay` when the suck ends.
- Terra death dispatch now keeps `blowterra` as its own `objinfo` death routine and routes through the original `blowquick` body after the still-unported `robodiesfx` audio call, instead of collapsing the table entry to generic `blowquick`.
- Focused combat parity selftest mode (`--combat-selftest`) now checks the original `wtable` weapon hitpoints/damage/speed values, representative `objinfo` enemy health/damage/death routine entries, `demonpause` 3/4 `wtable` damage scaling, the `hurtdeath`/`deathsuck`/`addsoul` state transition plus soul particle count, and projectile cadence/parameters for `terralogic2`, `phantomlogic`, `demonpause`, and `dragonfire`.
- Enemy hurt handling now follows `hurtobject`/`pauselogic2` for the table-driven `ob_hurtpause` wait and frame 4 hurt pose, then clears back to frame 0 when the hurt pause completes.
- Enemy hit reaction rendering now matches `drawshape_8`: the renderer no longer masks `ob_frame` to frames 0-3, so `hurtobject` frame 4 is visible while walking animation still wraps in the movement logic.
- Enemy non-lethal hit dispatch now follows the represented `objinfo` hit routines instead of treating every monster as generic `hurtobject`: dragon hits use `rts`, ghoul hits use `hurtghoul` blood-only behavior, generic grunts/lizard/troll/terra still use `hurtobject`, and deathhead hits use the ported `hurtdeath`/`deathsuck` path.
- Enemy blood now uses a `maxblood`-sized runtime list ported from `bloodymess`, `bloodymess2`, `bloodspeed`, `bloodspeed2`, `bloodspeed3`, `moveblood`, and `drawblood`: non-lethal hits emit 24 blood splots with `bloodspeed`, generic deaths emit 32 splots with `bloodspeed3`, gravity is `#$8000` per Amiga tick, and pixels are shaded with `blcols & ob_blood`.
- Generic enemy deaths now load the original `ob_chunks` payloads from `amiga/objs/*2` and port `blowchunx` plus the default `mode=0` `chunklogic2` path: each chunk frame is spawned from `bloodspeed3`, starts at y=-64 with y velocity minus `#$40000`, receives `#$8000` gravity per Amiga tick, renders through `drawshape_1sc`, and is killed when it reaches the floor.
- The original violence model switch is represented with `--meaty` for `mode=0` chunk vanish behavior and `--messy` for `mode=1` persistent `maxgore` plus wall-stop behavior. The PC runtime now defaults to an explicitly user-requested `MEATY+MESSY` mode (`--meaty-messy`/`--gore`) that keeps the freer MEATY chunk motion while also persisting chunks as MESSY ground gore.
- Persistent gore size now follows the original render split: flying chunks use `drawshape_1sc` with the dying object's `ob_scale`, while floor gore uses `drawshape_q` at fixed `$200` scale.

## In Progress
- Broader runtime object/combat behavior after the renderer/event vertical slice, with projectile-to-enemy damage, generic `monsterlogic`/`fire1` enemy projectiles, generic `monsterlogic` movement, baldy/lizard/troll charge-and-punch melee, deathhead cruise/charge movement, special projectile monster routines, dragon movement/fire logic, and minimal player death state now in place.
- `splat` sound, `blowterra` sound behavior, soul-suck audio/secondary rendering polish, and the `dragondead` `finished=3` end-game transition remain TODO until their original audio/runtime dependencies are ported directly.
- Deterministic parity harness for broader fixed-step player, door, projectile, and event behavior against Amiga-derived expectations.

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
- Preserve Amiga-authored simulation rates while allowing the PC runtime to render independently.
- Status: first visible debug build running, improving rendering fidelity now.

## Immediate Next Tasks
1. Introduce deterministic input replay harness for parity testing.
2. Finish remaining combat/death parity directly from `gloom2.s`, especially `blowterra` sound behavior, `splat` sound, `dragondead` finished-state handling, and focused parity tests for projectile/object edge cases.
3. Continue audio-backed death feedback once the original sound path is represented.
4. Expand focused combat parity tests for close barrel-origin hits, enemy removal, wall-hit priority, dragon death chunks, and special-monster projectile cadence.
5. Implement lock-style teleport event behavior once player lock/control state is represented.
