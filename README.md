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

## Releases

GitHub Actions builds tagged desktop and DOS releases and publishes downloadable archives with checksums and provenance. See [RELEASING.md](RELEASING.md) for maintainer steps and [VERIFY_RELEASE.md](VERIFY_RELEASE.md) for player verification commands.

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

The web shell supports a 2-player WebRTC mode based on the Stunt Car Remake setup. The host tab runs the game and streams the canvas; the guest opens the join link, watches the stream, and sends keyboard/gamepad controls as player two. Remote input always feeds player two and does not follow the host's local control assignment menu.

A signaling server is required. This repo carries the Cloudflare Workers/Durable Objects signaling service in `webrtc/muttistuntcarsignal/`. See [webrtc/muttistuntcarsignal/README.md](webrtc/muttistuntcarsignal/README.md) for deploy steps and how to set `SIGNAL_BASE` in [package/template.html](package/template.html).
