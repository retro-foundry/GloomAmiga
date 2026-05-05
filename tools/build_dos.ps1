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
