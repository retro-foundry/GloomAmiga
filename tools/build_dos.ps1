$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$djgppBin = Join-Path $repoRoot 'build\dos-toolchain\djgpp-mingw\djgpp\bin'
$stubEdit = Join-Path $repoRoot 'build\dos-toolchain\djgpp-mingw\djgpp\i586-pc-msdosdjgpp\bin\stubedit.exe'
$sdlSource = Join-Path $repoRoot 'build\dos-sdl\SDL'
$sdlBuild = Join-Path $repoRoot 'build\dos-sdl\SDL-build'
$sdlInclude = Join-Path $repoRoot 'build\dos-sdl\SDL\include'
$sdlLib = Join-Path $repoRoot 'build\dos-sdl\SDL-build\libSDL3.a'
$outDir = Join-Path $repoRoot 'build\dos'
$outExe = Join-Path $outDir 'GLOOMPC.EXE'

if (-not (Test-Path -LiteralPath (Join-Path $djgppBin 'i586-pc-msdosdjgpp-gcc.exe'))) {
  throw "Missing DJGPP compiler under $djgppBin"
}

$sdlDosEvents = Join-Path $sdlSource 'src\video\dos\SDL_dosevents.c'
if (Test-Path -LiteralPath $sdlDosEvents) {
  $eventsSource = Get-Content -LiteralPath $sdlDosEvents -Raw
  $patchedSource = $eventsSource

  if ($patchedSource -notlike '*SDL_DOS_SKIP_EVENT_YIELD*') {
    $yieldPatchSource = "    /* Give cooperative threads a chance to run.  Audio mixing now runs`n       in its own cooperative thread (via SDL's normal audio thread),`n       so it will execute during this yield along with loading threads`n       and anything else that is runnable. */`n    DOS_Yield();"
    if (-not $patchedSource.Contains($yieldPatchSource)) {
      throw "Unable to patch SDL DOS event yield in $sdlDosEvents"
    }
    $patchedSource = $patchedSource.Replace($yieldPatchSource,
      "    /* Gloom DOS pumps events once per rendered frame; yielding here turns`n       input polling into a visible frame-time spike on 386-class profiles. */`n    if (!SDL_GetHintBoolean(`"SDL_DOS_SKIP_EVENT_YIELD`", false)) {`n        DOS_Yield();`n    }")
  }
  if ($patchedSource -notlike '*SDL_DOS_SKIP_MOUSE_PUMP*') {
    $mousePatchSource = '    if (mouse->internal) { // if non-NULL, there''s a mouse detected on the system.'
    if (-not $patchedSource.Contains($mousePatchSource)) {
      throw "Unable to patch SDL DOS mouse pump in $sdlDosEvents"
    }
    $patchedSource = $patchedSource.Replace($mousePatchSource,
      '    if (mouse->internal && !SDL_GetHintBoolean("SDL_DOS_SKIP_MOUSE_PUMP", false)) { // if non-NULL, there''s a mouse detected on the system.')
  }
  if ($patchedSource -ne $eventsSource) {
    Set-Content -LiteralPath $sdlDosEvents -Value $patchedSource -NoNewline
  }
}

$sdlDosAudio = Join-Path $sdlSource 'src\audio\dos\SDL_dosaudio_sb.c'
if (Test-Path -LiteralPath $sdlDosAudio) {
  $audioSource = Get-Content -LiteralPath $sdlDosAudio -Raw
  $patchedAudio = $audioSource

  if ($patchedAudio -notlike '*#define RING_BUFFER_CHUNKS 8*') {
    $audioPatchSource = '#define RING_BUFFER_CHUNKS 4'
    if (-not $patchedAudio.Contains($audioPatchSource)) {
      throw "Unable to patch SDL DOS Sound Blaster ring depth in $sdlDosAudio"
    }
    $patchedAudio = $patchedAudio.Replace(
      '// 4 chunks is ~45 ms at 44100 Hz, enough headroom for 22 fps frame times.',
      '// 8 chunks is ~186 ms at 22050 Hz with 512-frame chunks, enough headroom for 8 fps frame pacing.')
    $patchedAudio = $patchedAudio.Replace($audioPatchSource, '#define RING_BUFFER_CHUNKS 8')
  }
  if ($patchedAudio -notlike '*manual_soundblaster_device*') {
    $audioPatchSource = 'static Uint8 soundblaster_silence_value = 0;'
    if (-not $patchedAudio.Contains($audioPatchSource)) {
      throw "Unable to patch SDL DOS Sound Blaster manual device pointer in $sdlDosAudio"
    }
    $patchedAudio = $patchedAudio.Replace($audioPatchSource,
      "static Uint8 soundblaster_silence_value = 0;`nstatic SDL_AudioDevice *manual_soundblaster_device = NULL;")
  }
  if ($patchedAudio -notlike '*SDL_DOS_MANUAL_AUDIO_FREQ*') {
    $audioPatchSource = @'
    } else {
        // SB 2.0 (DSP 2.x) and SB 1.x: 8-bit mono unsigned.
        device->spec.format = SDL_AUDIO_U8;
        device->spec.channels = 1;
    }

    // Accept whatever frequency SDL3's audio layer passes in. For SB16 (DSP >= 4)
'@
    if (-not $patchedAudio.Contains($audioPatchSource)) {
      throw "Unable to patch SDL DOS Sound Blaster manual frequency in $sdlDosAudio"
    }
    $patchedAudio = $patchedAudio.Replace($audioPatchSource, @'
    } else {
        // SB 2.0 (DSP 2.x) and SB 1.x: 8-bit mono unsigned.
        device->spec.format = SDL_AUDIO_U8;
        device->spec.channels = 1;
    }
    if (SDL_GetHintBoolean("SDL_DOS_MANUAL_AUDIO_PUMP", false)) {
        const char *freq_hint = SDL_GetHint("SDL_DOS_MANUAL_AUDIO_FREQ");
        const int freq = freq_hint ? SDL_atoi(freq_hint) : 22050;
        if (freq >= 5000 && freq <= 44100) {
            device->spec.freq = freq;
        }
    }

    // Accept whatever frequency SDL3's audio layer passes in. For SB16 (DSP >= 4)
'@)
  }
  if ($patchedAudio -notlike '*manual_soundblaster_device = device*') {
    $audioPatchSource = "    SDL_Log(`"SoundBlaster opened!`");`n    return true;"
    if (-not $patchedAudio.Contains($audioPatchSource)) {
      throw "Unable to patch SDL DOS Sound Blaster open hook in $sdlDosAudio"
    }
    $patchedAudio = $patchedAudio.Replace($audioPatchSource,
      "    SDL_Log(`"SoundBlaster opened!`");`n    manual_soundblaster_device = device;`n    return true;")
  }
  if ($patchedAudio -notlike '*SDL_DOSSoundBlasterGetQueuedAudioSize*') {
    $audioPatchSource = @'
    return true;
}

static void DOSSOUNDBLASTER_CloseDevice(SDL_AudioDevice *device)
'@
    if (-not $patchedAudio.Contains($audioPatchSource)) {
      throw "Unable to patch SDL DOS Sound Blaster manual queue hooks in $sdlDosAudio"
    }
    $patchedAudio = $patchedAudio.Replace($audioPatchSource, @'
    return true;
}

int SDL_DOSSoundBlasterGetQueuedAudioSize(SDL_AudioDeviceID devid)
{
    SDL_AudioDevice *device = manual_soundblaster_device;
    struct SDL_PrivateAudioData *hidden = device ? device->hidden : NULL;

    (void)devid;
    if (!hidden) {
        return -1;
    }
    return hidden->ring_write - isr_ring_read;
}

int SDL_DOSSoundBlasterGetAudioBufferSize(SDL_AudioDeviceID devid)
{
    SDL_AudioDevice *device = manual_soundblaster_device;
    struct SDL_PrivateAudioData *hidden = device ? device->hidden : NULL;

    (void)devid;
    return hidden ? hidden->ring_size : 0;
}

int SDL_DOSSoundBlasterGetAudioChunkSize(SDL_AudioDeviceID devid)
{
    SDL_AudioDevice *device = manual_soundblaster_device;
    struct SDL_PrivateAudioData *hidden = device ? device->hidden : NULL;

    (void)devid;
    return hidden ? hidden->chunk_size : 0;
}

bool SDL_DOSSoundBlasterQueueAudio(SDL_AudioDeviceID devid, const void *buffer, int len)
{
    SDL_AudioDevice *device = manual_soundblaster_device;
    struct SDL_PrivateAudioData *hidden = device ? device->hidden : NULL;
    int used = 0;

    (void)devid;
    if (!hidden || !buffer || len <= 0) {
        return false;
    }
    used = hidden->ring_write - isr_ring_read;
    if (len > hidden->ring_size - used) {
        return false;
    }
    return DOSSOUNDBLASTER_PlayDevice(device, (const Uint8 *)buffer, len);
}

void SDL_DOSSoundBlasterClearQueuedAudio(SDL_AudioDeviceID devid)
{
    SDL_AudioDevice *device = manual_soundblaster_device;
    struct SDL_PrivateAudioData *hidden = device ? device->hidden : NULL;

    (void)devid;
    if (!hidden) {
        return;
    }
    DOS_DisableInterrupts();
    SDL_memset(hidden->ring_buffer, soundblaster_silence_value, hidden->ring_size);
    hidden->ring_write = isr_ring_read;
    isr_ring_write = hidden->ring_write;
    DOS_EnableInterrupts();
}

static void DOSSOUNDBLASTER_CloseDevice(SDL_AudioDevice *device)
'@)
  }
  if ($patchedAudio -notlike '*manual_soundblaster_device == device*') {
    $audioPatchSource = @'
static void DOSSOUNDBLASTER_CloseDevice(SDL_AudioDevice *device)
{
    struct SDL_PrivateAudioData *hidden = device->hidden;
'@
    if (-not $patchedAudio.Contains($audioPatchSource)) {
      throw "Unable to patch SDL DOS Sound Blaster close hook in $sdlDosAudio"
    }
    $patchedAudio = $patchedAudio.Replace($audioPatchSource, @'
static void DOSSOUNDBLASTER_CloseDevice(SDL_AudioDevice *device)
{
    struct SDL_PrivateAudioData *hidden = device->hidden;
    if (manual_soundblaster_device == device) {
        manual_soundblaster_device = NULL;
    }
'@)
  }
  if ($patchedAudio -notlike '*ProvidesOwnCallbackThread = SDL_GetHintBoolean("SDL_DOS_MANUAL_AUDIO_PUMP"*') {
    $audioPatchSource = @'
    impl->PlayDevice = DOSSOUNDBLASTER_PlayDevice;
    impl->CloseDevice = DOSSOUNDBLASTER_CloseDevice;

    impl->OnlyHasDefaultPlaybackDevice = true;
'@
    if (-not $patchedAudio.Contains($audioPatchSource)) {
      throw "Unable to patch SDL DOS Sound Blaster manual callback-thread hint in $sdlDosAudio"
    }
    $patchedAudio = $patchedAudio.Replace($audioPatchSource, @'
    impl->PlayDevice = DOSSOUNDBLASTER_PlayDevice;
    impl->CloseDevice = DOSSOUNDBLASTER_CloseDevice;
    impl->ProvidesOwnCallbackThread = SDL_GetHintBoolean("SDL_DOS_MANUAL_AUDIO_PUMP", false);

    impl->OnlyHasDefaultPlaybackDevice = true;
'@)
  }
  if ($patchedAudio -ne $audioSource) {
    Set-Content -LiteralPath $sdlDosAudio -Value $patchedAudio -NoNewline
  }
}

if (Test-Path -LiteralPath $sdlBuild) {
  & cmake --build $sdlBuild --config Release
}
if (-not (Test-Path -LiteralPath $sdlLib)) {
  throw "Missing DOS SDL3 library $sdlLib"
}

New-Item -ItemType Directory -Force -Path $outDir | Out-Null
$env:Path = "$djgppBin;$env:Path"

$runtimeDirs = @(
  'maps',
  'data\maps',
  'txts',
  'data\txts',
  'objs',
  'data\objs',
  'prog\objs',
  'misc',
  'sfxs',
  'ggfx'
)

foreach ($runtimeDir in $runtimeDirs) {
  $sourceDir = Join-Path $repoRoot "amiga\$runtimeDir"
  $destDir = Join-Path $outDir "amiga\$runtimeDir"
  if (Test-Path -LiteralPath $sourceDir) {
    New-Item -ItemType Directory -Force -Path $destDir | Out-Null
    Get-ChildItem -LiteralPath $sourceDir -Force | Copy-Item -Destination $destDir -Recurse -Force
  }
}

$runtimeRootFiles = @(
  @{ Source = 'blackmagic.iff'; Dest = 'BLACKMAG.IFF' }
)

foreach ($runtimeRootFile in $runtimeRootFiles) {
  $sourceFile = Join-Path $repoRoot "amiga\$($runtimeRootFile.Source)"
  $destFile = Join-Path $outDir "amiga\$($runtimeRootFile.Dest)"
  if (Test-Path -LiteralPath $sourceFile) {
    Copy-Item -LiteralPath $sourceFile -Destination $destFile -Force
  }
}

$dosMiscDir = Join-Path $outDir 'amiga\misc'
New-Item -ItemType Directory -Force -Path $dosMiscDir | Out-Null
Copy-Item -LiteralPath (Join-Path $repoRoot 'amiga\misc\palette_8') -Destination (Join-Path $dosMiscDir 'PAL8.BIN') -Force
Copy-Item -LiteralPath (Join-Path $repoRoot 'amiga\misc\remap_8') -Destination (Join-Path $dosMiscDir 'RMAP8.BIN') -Force

Push-Location $repoRoot
try {
  & (Join-Path $djgppBin 'i586-pc-msdosdjgpp-gcc.exe') `
    -O3 -fomit-frame-pointer -march=i386 -mtune=i486 `
    -std=c17 -Wall -Wextra -Wpedantic -DGLOOM_DOS_SDL3 `
    -DGLOOM_BINARY_VERSION_MAJOR=1 -DGLOOM_BINARY_VERSION_MINOR=0 -DGLOOM_BINARY_VERSION_PATCH=0 `
    -Isrc "-I$sdlInclude" `
    src\main.c src\map.c src\iff.c $sdlLib -lm `
    -o $outExe
} finally {
  Pop-Location
}

& $stubEdit $outExe minstack=8M
Write-Host "Built $outExe"
