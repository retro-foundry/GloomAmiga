$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$djgppBin = Join-Path $repoRoot 'build\dos-toolchain\djgpp-mingw\djgpp\bin'
$stubEdit = Join-Path $repoRoot 'build\dos-toolchain\djgpp-mingw\djgpp\i586-pc-msdosdjgpp\bin\stubedit.exe'
$sdlSource = Join-Path $repoRoot 'build\dos-sdl\SDL'
$sdlBuild = Join-Path $repoRoot 'build\dos-sdl\SDL-build'
$sdlInclude = Join-Path $repoRoot 'build\dos-sdl\SDL\include'
$sdlLib = Join-Path $repoRoot 'build\dos-sdl\SDL-build\libSDL3.a'
$xmpSource = Join-Path $repoRoot 'build\_deps\sdl2_mixer-src\external\libxmp'
$xmpBuild = Join-Path $repoRoot 'build\dos-libxmp'
$xmpObj = Join-Path $xmpBuild 'obj'
$xmpInclude = Join-Path $xmpSource 'include'
$xmpLib = Join-Path $xmpBuild 'libxmp_dos.a'
$outDir = Join-Path $repoRoot 'build\dos'
$outExe = Join-Path $outDir 'GLOOMPC.EXE'
$cmakeLists = Get-Content -LiteralPath (Join-Path $repoRoot 'CMakeLists.txt') -Raw
$versionMatch = [regex]::Match($cmakeLists, 'project\s*\(\s*GloomPC\s+VERSION\s+(\d+)\.(\d+)\.(\d+)')
if (-not $versionMatch.Success) {
  throw "Unable to read GloomPC version from CMakeLists.txt"
}
$versionMajor = $versionMatch.Groups[1].Value
$versionMinor = $versionMatch.Groups[2].Value
$versionPatch = $versionMatch.Groups[3].Value

if (-not (Test-Path -LiteralPath (Join-Path $djgppBin 'i586-pc-msdosdjgpp-gcc.exe'))) {
  throw "Missing DJGPP compiler under $djgppBin"
}

$sdlDosEvents = Join-Path $sdlSource 'src\video\dos\SDL_dosevents.c'
if (Test-Path -LiteralPath $sdlDosEvents) {
  $eventsSource = (Get-Content -LiteralPath $sdlDosEvents -Raw) -replace "`r`n", "`n"
  $patchedSource = $eventsSource

  if ($patchedSource -notlike '*SDL_DOS_SKIP_EVENT_YIELD*') {
    $yieldPatchPattern = '(?s)\s*/\* Give cooperative threads a chance to run\..*?\*/\r?\n\s*DOS_Yield\(\);'
    if (-not [regex]::IsMatch($patchedSource, $yieldPatchPattern)) {
      throw "Unable to patch SDL DOS event yield in $sdlDosEvents"
    }
    $patchedSource = [regex]::Replace($patchedSource, $yieldPatchPattern,
      "`n    /* Gloom DOS pumps events once per rendered frame; yielding here turns`n       input polling into a visible frame-time spike on 386-class profiles. */`n    if (!SDL_GetHintBoolean(`"SDL_DOS_SKIP_EVENT_YIELD`", false)) {`n        DOS_Yield();`n    }", 1)
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
  $audioSource = (Get-Content -LiteralPath $sdlDosAudio -Raw) -replace "`r`n", "`n"
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
    $audioPatchPattern = '(?s)(    \} else \{\n\s*// SB 2\.0 \(DSP 2\.x\) and SB 1\.x: 8-bit mono unsigned\.\n\s*device->spec\.format = SDL_AUDIO_U8;\n\s*device->spec\.channels = 1;\n\s*\}\n)(\s*// Accept whatever frequency SDL3''s audio layer passes in\.)'
    if (-not [regex]::IsMatch($patchedAudio, $audioPatchPattern)) {
      throw "Unable to patch SDL DOS Sound Blaster manual frequency in $sdlDosAudio"
    }
    $patchedAudio = [regex]::Replace($patchedAudio, $audioPatchPattern, @'
$1
    if (SDL_GetHintBoolean("SDL_DOS_MANUAL_AUDIO_PUMP", false)) {
        const char *freq_hint = SDL_GetHint("SDL_DOS_MANUAL_AUDIO_FREQ");
        const int freq = freq_hint ? SDL_atoi(freq_hint) : 22050;
        if (freq >= 5000 && freq <= 44100) {
            device->spec.freq = freq;
        }
    }

$2
'@, 1)
  }
  if ($patchedAudio -notlike '*manual_soundblaster_device = device*') {
    $audioPatchPattern = '(?m)^(?<indent>\s*)SDL_Log\("SoundBlaster opened!"\);\n(?<indent2>\s*)return true;'
    if (-not [regex]::IsMatch($patchedAudio, $audioPatchPattern)) {
      throw "Unable to patch SDL DOS Sound Blaster open hook in $sdlDosAudio"
    }
    $patchedAudio = [regex]::Replace($patchedAudio, $audioPatchPattern,
      '${indent}SDL_Log("SoundBlaster opened!");' + "`n" +
      '${indent}manual_soundblaster_device = device;' + "`n" +
      '${indent2}return true;', 1)
  }
  if ($patchedAudio -notlike '*SDL_DOSSoundBlasterGetQueuedAudioSize*') {
    $audioPatchPattern = '(?m)^.*DOSSOUNDBLASTER_CloseDevice\s*\(\s*SDL_AudioDevice\s*\*\s*device\s*\)'
    $audioPatchMatch = [regex]::Match($patchedAudio, $audioPatchPattern)
    if (-not $audioPatchMatch.Success) {
      throw "Unable to patch SDL DOS Sound Blaster manual queue hooks in $sdlDosAudio"
    }
    $audioPatchInsert = @'

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

'@
    $patchedAudio = $patchedAudio.Insert($audioPatchMatch.Index, $audioPatchInsert)
  }
  if ($patchedAudio -notlike '*manual_soundblaster_device == device*') {
    $audioPatchPattern = '(?s)(static void DOSSOUNDBLASTER_CloseDevice\(SDL_AudioDevice \*device\)\s*\{\n\s*struct SDL_PrivateAudioData \*hidden = device->hidden;\n)'
    if (-not [regex]::IsMatch($patchedAudio, $audioPatchPattern)) {
      throw "Unable to patch SDL DOS Sound Blaster close hook in $sdlDosAudio"
    }
    $patchedAudio = [regex]::Replace($patchedAudio, $audioPatchPattern, @'
$1
    if (manual_soundblaster_device == device) {
        manual_soundblaster_device = NULL;
    }
'@, 1)
  }
  if ($patchedAudio -notlike '*ProvidesOwnCallbackThread = SDL_GetHintBoolean("SDL_DOS_MANUAL_AUDIO_PUMP"*') {
    $audioPatchPattern = '(?s)(\s*impl->PlayDevice = DOSSOUNDBLASTER_PlayDevice;\n\s*impl->CloseDevice = DOSSOUNDBLASTER_CloseDevice;\n)(\s*impl->OnlyHasDefaultPlaybackDevice = true;)'
    if (-not [regex]::IsMatch($patchedAudio, $audioPatchPattern)) {
      throw "Unable to patch SDL DOS Sound Blaster manual callback-thread hint in $sdlDosAudio"
    }
    $patchedAudio = [regex]::Replace($patchedAudio, $audioPatchPattern, @'
$1
    impl->ProvidesOwnCallbackThread = SDL_GetHintBoolean("SDL_DOS_MANUAL_AUDIO_PUMP", false);

$2
'@, 1)
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
if (-not (Test-Path -LiteralPath (Join-Path $xmpSource 'cmake\libxmp-sources.cmake'))) {
  throw "Missing libxmp source under $xmpSource. Run the normal CMake configure once so SDL_mixer/libxmp is fetched."
}

New-Item -ItemType Directory -Force -Path $xmpObj | Out-Null
$xmpSourceList = Get-Content -LiteralPath (Join-Path $xmpSource 'cmake\libxmp-sources.cmake') -Raw
$xmpSourceMatch = [regex]::Match(
  $xmpSourceList,
  'set\(LIBXMP_SRC_LIST\s+(?<body>.*?)\)\s*\r?\n\s*set\(LIBXMP_SRC_LIST_PROWIZARD',
  [Text.RegularExpressions.RegexOptions]::Singleline)
if (-not $xmpSourceMatch.Success) {
  throw "Unable to parse libxmp source list from $xmpSource"
}
$xmpSources = [regex]::Matches($xmpSourceMatch.Groups['body'].Value, 'src/[A-Za-z0-9_./-]+\.c') |
  ForEach-Object { $_.Value } |
  Where-Object { $_ -ne 'src/win32.c' -and $_ -ne 'src/mkstemp.c' }
$xmpObjects = @()
foreach ($xmpSourceRel in $xmpSources) {
  $xmpSourceFile = Join-Path $xmpSource ($xmpSourceRel -replace '/', '\')
  $xmpObjectName = (($xmpSourceRel -replace '[\\/]', '_') -replace '\.c$', '.o')
  $xmpObjectFile = Join-Path $xmpObj $xmpObjectName

  & (Join-Path $djgppBin 'i586-pc-msdosdjgpp-gcc.exe') `
    -O2 -fomit-frame-pointer -march=i386 -mtune=i486 -std=gnu99 `
    -DLIBXMP_STATIC -DLIBXMP_NO_DEPACKERS -DLIBXMP_NO_PROWIZARD -DHAVE_FNMATCH `
    "-I$xmpInclude" "-I$(Join-Path $xmpSource 'src')" "-I$(Join-Path $xmpSource 'src\loaders')" `
    -c $xmpSourceFile -o $xmpObjectFile
  if ($LASTEXITCODE -ne 0) {
    throw "Failed to compile DOS libxmp source $xmpSourceRel"
  }
  $xmpObjects += $xmpObjectFile
}
if (Test-Path -LiteralPath $xmpLib) {
  Remove-Item -LiteralPath $xmpLib -Force
}
& (Join-Path $djgppBin 'i586-pc-msdosdjgpp-ar.exe') rcs $xmpLib @xmpObjects
if ($LASTEXITCODE -ne 0) {
  throw "Failed to archive DOS libxmp library $xmpLib"
}

New-Item -ItemType Directory -Force -Path $outDir | Out-Null
Copy-Item -LiteralPath (Join-Path $repoRoot 'package\README.TXT') -Destination (Join-Path $outDir 'README.TXT') -Force
Copy-Item -LiteralPath (Join-Path $repoRoot 'package\GLOOM.INI') -Destination (Join-Path $outDir 'GLOOM.INI') -Force
$csdpmi = Join-Path $repoRoot 'build\dos-toolchain\csdpmi7b\bin\CWSDPMI.EXE'
if (Test-Path -LiteralPath $csdpmi) {
  Copy-Item -LiteralPath $csdpmi -Destination (Join-Path $outDir 'CWSDPMI.EXE') -Force
}
Set-Content -LiteralPath (Join-Path $outDir 'GLOOM.BAT') -Value @(
  '@ECHO OFF',
  'GLOOMPC.EXE %1 %2 %3 %4 %5 %6 %7 %8 %9'
) -Encoding ASCII
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
  'ggfx',
  'pics',
  'data\pics'
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
  @{ Source = 'combat.iff'; Dest = 'combat.iff' },
  @{ Source = 'title.iff'; Dest = 'title.iff' },
  @{ Source = 'blackmagic.iff'; Dest = 'BLACKMAG.IFF' },
  @{ Source = 'bullet1.bin'; Dest = 'bullet1.bin' },
  @{ Source = 'bullet2.bin'; Dest = 'bullet2.bin' },
  @{ Source = 'bullet3.bin'; Dest = 'bullet3.bin' },
  @{ Source = 'bullet4.bin'; Dest = 'bullet4.bin' },
  @{ Source = 'bullet5.bin'; Dest = 'bullet5.bin' },
  @{ Source = 'sparks1.bin'; Dest = 'sparks1.bin' },
  @{ Source = 'sparks2.bin'; Dest = 'sparks2.bin' },
  @{ Source = 'sparks3.bin'; Dest = 'sparks3.bin' },
  @{ Source = 'sparks4.bin'; Dest = 'sparks4.bin' },
  @{ Source = 'sparks5.bin'; Dest = 'sparks5.bin' },
  @{ Source = 'gridoffs4.bin'; Dest = 'gridoff4.bin' },
  @{ Source = 'spacehulk.iff'; Dest = 'SPACEHUL.IFF' },
  @{ Source = 'gothic.iff'; Dest = 'GOTHIC.IFF' },
  @{ Source = 'hell.iff'; Dest = 'HELL.IFF' },
  @{ Source = 'theend.iff'; Dest = 'THEEND.IFF' }
)

foreach ($runtimeRootFile in $runtimeRootFiles) {
  $sourceFile = Join-Path $repoRoot "amiga\$($runtimeRootFile.Source)"
  $destFile = Join-Path $outDir "amiga\$($runtimeRootFile.Dest)"
  if (-not (Test-Path -LiteralPath $sourceFile)) {
    throw "Missing required DOS runtime asset $sourceFile"
  }
  Copy-Item -LiteralPath $sourceFile -Destination $destFile -Force
}

$dosMiscDir = Join-Path $outDir 'amiga\misc'
New-Item -ItemType Directory -Force -Path $dosMiscDir | Out-Null
Copy-Item -LiteralPath (Join-Path $repoRoot 'amiga\misc\palette_8') -Destination (Join-Path $dosMiscDir 'PAL8.BIN') -Force
Copy-Item -LiteralPath (Join-Path $repoRoot 'amiga\misc\remap_8') -Destination (Join-Path $dosMiscDir 'RMAP8.BIN') -Force
Copy-Item -LiteralPath (Join-Path $repoRoot 'amiga\misc\smallfont2.bin') -Destination (Join-Path $dosMiscDir 'SMALLF2.BIN') -Force

Push-Location $repoRoot
try {
  & (Join-Path $djgppBin 'i586-pc-msdosdjgpp-gcc.exe') `
    -O3 -fomit-frame-pointer -march=i386 -mtune=i486 `
    -std=c17 -Wall -Wextra -Wpedantic -DGLOOM_DOS_SDL3 -DGLOOM_DOS_MENU_MUSIC `
    "-DGLOOM_BINARY_VERSION_MAJOR=$versionMajor" "-DGLOOM_BINARY_VERSION_MINOR=$versionMinor" `
    "-DGLOOM_BINARY_VERSION_PATCH=$versionPatch" `
    -Isrc "-I$sdlInclude" "-I$xmpInclude" `
    src\main.c src\map.c src\iff.c $sdlLib $xmpLib -lm `
    -o $outExe
} finally {
  Pop-Location
}

& $stubEdit $outExe minstack=8M
Write-Host "Built $outExe"
