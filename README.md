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

Optional upscaled presentation art can be enabled on Windows native builds:

```powershell
.\build\Debug\gloom_pc.exe --hd-art D:\art-HD-v1
```

The same path can be set in `GLOOM.INI` with `hd_art_path=D:\art-HD-v1`; the command-line option overrides the INI value.
Once HD art is loaded, press `F10` during gameplay or use the pause menu's `HD TEXTURES` item to switch it on or off without reloading the level.

The original Amiga assets remain required and authoritative; HD art only replaces sampled presentation pixels for
walls, flats, sprites, weapon art, menu/screen artwork, and fonts.
HD art packs may be partial: any wall, flat, sprite, weapon, menu image, or font replacement that is not present
falls back to the original SD asset at runtime.

`GLOOM.INI` controls the software framebuffer size with `resolution=WIDTHxHEIGHT`, the OpenGL/WebGL logical
render target with `hardware_resolution=WIDTHxHEIGHT` (default 1920x1080), `renderer=auto|opengl|software`, plus
optional viewport, HD art, control-source, and keyboard settings. `--renderer` overrides the INI renderer choice, and
`--resolution`/`--window-size`/`--boot-resolution` override either configured renderer resolution.
On desktop/web, the OpenGL/WebGL backend owns source-backed menu, script, pause, HUD, weapon, SD floor/ceiling
`flat` texture draws, and SD wall-column draws from original 16-band wall atlases with transparent-column alpha.
It also draws HD floor/ceiling and wall presentation pixels from the same source-backed layout, draws represented
`drawblood` one-pixel splots on the GPU, and applies the original red-palette injury feedback and pixelate transition
as GPU post-processes. SD `drawshapes` billboards/effects use the GPU path on the validated low comparison target;
large hardware targets and HD sprite presentation still use the Amiga-derived software overlay pending the remaining
GL sprite port.
Use `--frame-dump out.bmp --renderer software|opengl` for one-frame gameplay captures from the selected renderer.
Use `python scripts/compare_renderer_frames.py --build-dir build --config Debug` on a machine with an OpenGL-capable
SDL video driver for a fixed `map1_1` software/OpenGL frame comparison.
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
