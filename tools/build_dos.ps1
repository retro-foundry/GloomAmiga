$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$djgppBin = Join-Path $repoRoot 'build\dos-toolchain\djgpp-mingw\djgpp\bin'
$stubEdit = Join-Path $repoRoot 'build\dos-toolchain\djgpp-mingw\djgpp\i586-pc-msdosdjgpp\bin\stubedit.exe'
$sdlInclude = Join-Path $repoRoot 'build\dos-sdl\SDL\include'
$sdlLib = Join-Path $repoRoot 'build\dos-sdl\SDL-build\libSDL3.a'
$outDir = Join-Path $repoRoot 'build\dos'
$outExe = Join-Path $outDir 'GLOOMPC.EXE'

if (-not (Test-Path -LiteralPath (Join-Path $djgppBin 'i586-pc-msdosdjgpp-gcc.exe'))) {
  throw "Missing DJGPP compiler under $djgppBin"
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

$dosMiscDir = Join-Path $outDir 'amiga\misc'
New-Item -ItemType Directory -Force -Path $dosMiscDir | Out-Null
Copy-Item -LiteralPath (Join-Path $repoRoot 'amiga\misc\palette_8') -Destination (Join-Path $dosMiscDir 'PAL8.BIN') -Force
Copy-Item -LiteralPath (Join-Path $repoRoot 'amiga\misc\remap_8') -Destination (Join-Path $dosMiscDir 'RMAP8.BIN') -Force

Push-Location $repoRoot
try {
  & (Join-Path $djgppBin 'i586-pc-msdosdjgpp-gcc.exe') `
    -O3 -fomit-frame-pointer -march=i386 -mtune=i486 `
    -std=c17 -Wall -Wextra -Wpedantic -DGLOOM_DOS_SDL3 `
    -Isrc "-I$sdlInclude" `
    src\main.c src\map.c src\iff.c $sdlLib -lm `
    -o $outExe
} finally {
  Pop-Location
}

& $stubEdit $outExe minstack=8M
Write-Host "Built $outExe"
