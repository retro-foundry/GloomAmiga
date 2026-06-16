# Gloom PC Port

This project ports Gloom to the PC runtime. The original Amiga source and assets under `amiga/` remain the gameplay authority.

## Native Build

```bash
cmake -S . -B build
cmake --build build
```

Windows Debug/Release builds can also be built with:

```powershell
cmake --build build --config Debug
cmake --build build --config Release
```

Optional upscaled presentation art can be enabled for the OpenGL/WebGL renderer on Windows native builds:

```powershell
.\build\Debug\gloom_pc.exe --hd-art D:\art-HD-v1
```

The same path can be set in `GLOOM.INI` with `hd_art_path=D:\art-HD-v1`; the command-line option overrides the INI value.
When the active renderer is OpenGL/WebGL, press `F10` during gameplay or use the pause menu's `HD TEXTURES` item to switch it on or off without reloading the level. The software renderer always presents the original source assets even if an HD art root is configured.

The original Amiga assets remain required and authoritative; HD art only replaces sampled presentation pixels for
OpenGL/WebGL walls, flats, sprites, weapon art, menu/screen artwork, and fonts.
HD art packs may be partial: any wall, flat, sprite, weapon, menu image, or font replacement that is not present
falls back to the original SD asset at runtime.
HD wall replacement PNGs may use different dimensions per `texture_##.png` slot, as long as each replacement is at
least the original 64x64 wall texture size.
HD sprite, billboard, weapon, and chunk replacement PNGs may also use different dimensions per frame; each replacement
must be at least as large as the original logical frame, and OpenGL/WebGL samples it through the original frame
coordinates so presentation resolution does not change gameplay handles, scale, or frame choice.

`GLOOM.INI` controls the software framebuffer size with `resolution=WIDTHxHEIGHT`, the OpenGL/WebGL logical
render target with `hardware_resolution=WIDTHxHEIGHT` (default 1920x1080), `renderer=auto|opengl|software`, plus
optional viewport, HD art, control-source, and keyboard settings. `--renderer` overrides the INI renderer choice, and
`--resolution`/`--window-size`/`--boot-resolution` override either configured renderer resolution.
On desktop/web, the OpenGL/WebGL backend owns source-backed menu, script, pause, HUD, weapon, SD floor/ceiling
`flat` texture draws, and SD wall-column draws from original 16-band wall atlases with transparent-column alpha.
It also draws HD floor/ceiling and wall presentation pixels from the same source-backed layout, blending PNG alpha
over the original source texel without adding non-Amiga wall holes, draws SD and HD `drawshapes` billboards/effects
as GPU column quads using the existing source-backed sprite layout and source-derived drawshape mask, draws represented `drawblood`
one-pixel splots on the GPU, and applies the original red-palette injury feedback and pixelate transition as GPU
post-processes.
Use `--frame-dump out.bmp --renderer software|opengl` for one-frame gameplay captures from the selected renderer.
Diagnostic frame dumps can also use `--frame-dump-ticks`, `--force-first-door`,
`--frame-dump-look-at-first-door`, `--frame-dump-look-at-transparent-wall`, and
`--force-level-transition` to capture represented `doanims`/`dodoors`/transparent-column/scriptplay states.
Use `--screen-frame-dump start_menu|combat_menu|pause|script_intro|completion out.bmp --renderer software|opengl`
for deterministic menu/screen captures from the real runtime renderer.
Use `python scripts/compare_renderer_frames.py --build-dir build --config Debug` on a machine with an OpenGL-capable
SDL video driver for software/OpenGL frame comparisons across base gameplay, red flash, pixelate, blood, combat,
two-player split, animated walls, doors, transparent walls, level transitions, menus, pause, script-intro, and
completion scenarios.
Windows reads `GLOOM.INI` from the game/executable directory and writes the loaded INI path plus final
framebuffer/window size to `GLOOM.LOG` next to `gloom_pc.exe`.

The runtime consumes the source-derived layout emitted by `tools/extract_amiga_art.py`: menu and script pictures can
come from `art/form/<asset-key>/frame_0001.png` or `art/trimmed_pictures/<asset-key>.png`, while HD font glyphs can
come from `art/blit`, `art/blit2`, or `art/objects` glyph/frame folders. Use
`.\build\Debug\gloom_pc.exe --hd-art-selftest D:\art-HD-v1` to verify that a local HD art root contains the menu
artwork and font replacements expected by the runtime.

## Releases

GitHub Actions builds tagged Windows and DOS releases and publishes downloadable archives with checksums and provenance. See [RELEASING.md](RELEASING.md) for maintainer steps and [VERIFY_RELEASE.md](VERIFY_RELEASE.md) for player verification commands.

## Web Build (Emscripten)

Web builds use Emscripten's SDL2 port and preload the `amiga/` runtime assets into the virtual filesystem.

```bash
emcmake cmake -S . -B build-web
cmake --build build-web
```

On this Windows setup, the PowerShell `emcmake` wrapper may require `py.exe`; the batch wrapper works:

```powershell
cmd /c C:\Users\paula\emsdk\upstream\emscripten\emcmake.bat cmake -S . -B build-web
cmake --build build-web
```

The resulting runnable page is `build-web/gloom_pc.html`. Serve it from a local web server, for example:

```bash
cd build-web
python -m http.server 8000
```

## WebRTC Multiplayer

The web shell supports a 2-player WebRTC mode for Gloom. The host tab runs the game and streams the canvas; the guest opens the join link, watches the stream, and sends keyboard/gamepad controls as player two. Remote input always feeds player two and does not follow the host's local control assignment menu.

A signaling server is required. This repo carries the Cloudflare Workers/Durable Objects signaling service in `webrtc/gloom-signaling/`. See [webrtc/gloom-signaling/README.md](webrtc/gloom-signaling/README.md) for deploy steps and how to set `SIGNAL_BASE` in [package/template.html](package/template.html).
