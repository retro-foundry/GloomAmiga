$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$toolchainRoot = Join-Path $repoRoot 'build\dos-toolchain'
$djgppZip = Join-Path $toolchainRoot 'djgpp-mingw-gcc1220-standalone.zip'
$csdpmiZip = Join-Path $toolchainRoot 'csdpmi7b.zip'
$djgppDir = Join-Path $toolchainRoot 'djgpp-mingw'
$csdpmiDir = Join-Path $toolchainRoot 'csdpmi7b'
$sdlRoot = Join-Path $repoRoot 'build\dos-sdl'
$sdlSource = Join-Path $sdlRoot 'SDL'
$sdlBuild = Join-Path $sdlRoot 'SDL-build'
$djgppBin = Join-Path $djgppDir 'djgpp\bin'
$sdlCommit = 'c699512adcc139ec1a355ff97cb2e5dbab3c9ac2'

function Assert-NativeCommandSucceeded {
  param([string] $Description)

  if ($LASTEXITCODE -ne 0) {
    throw "$Description failed with exit code $LASTEXITCODE"
  }
}

New-Item -ItemType Directory -Force -Path $toolchainRoot | Out-Null

if (-not (Test-Path -LiteralPath (Join-Path $djgppBin 'i586-pc-msdosdjgpp-gcc.exe'))) {
  if (-not (Test-Path -LiteralPath $djgppZip)) {
    Invoke-WebRequest `
      -Uri 'https://github.com/andrewwutw/build-djgpp/releases/download/v3.4/djgpp-mingw-gcc1220-standalone.zip' `
      -OutFile $djgppZip
  }
  if (Test-Path -LiteralPath $djgppDir) {
    Remove-Item -LiteralPath $djgppDir -Recurse -Force
  }
  New-Item -ItemType Directory -Force -Path $djgppDir | Out-Null
  Expand-Archive -LiteralPath $djgppZip -DestinationPath $djgppDir -Force
}

if (-not (Test-Path -LiteralPath (Join-Path $csdpmiDir 'bin\CWSDPMI.EXE'))) {
  if (-not (Test-Path -LiteralPath $csdpmiZip)) {
    Invoke-WebRequest `
      -Uri 'https://ftp.gwdg.de/pub/msdos/gcc/djgpp/v2misc/csdpmi7b.zip' `
      -OutFile $csdpmiZip
  }
  if (Test-Path -LiteralPath $csdpmiDir) {
    Remove-Item -LiteralPath $csdpmiDir -Recurse -Force
  }
  New-Item -ItemType Directory -Force -Path $csdpmiDir | Out-Null
  Expand-Archive -LiteralPath $csdpmiZip -DestinationPath $csdpmiDir -Force
}

if (-not (Test-Path -LiteralPath (Join-Path $repoRoot 'build\_deps\sdl2_mixer-src\external\libxmp\cmake\libxmp-sources.cmake'))) {
  cmake -S $repoRoot -B (Join-Path $repoRoot 'build') -A x64
  Assert-NativeCommandSucceeded "Native CMake dependency configure"
}

New-Item -ItemType Directory -Force -Path $sdlRoot | Out-Null
if (-not (Test-Path -LiteralPath (Join-Path $sdlSource '.git'))) {
  if (Test-Path -LiteralPath $sdlSource) {
    Remove-Item -LiteralPath $sdlSource -Recurse -Force
  }
  git clone https://github.com/libsdl-org/SDL.git $sdlSource
  Assert-NativeCommandSucceeded "SDL clone"
}

git -C $sdlSource fetch --depth 1 origin $sdlCommit
Assert-NativeCommandSucceeded "SDL fetch"
git -C $sdlSource checkout --force $sdlCommit
Assert-NativeCommandSucceeded "SDL checkout"

$env:Path = "$djgppBin;$env:Path"
cmake -S $sdlSource -B $sdlBuild -G Ninja `
  -DCMAKE_TOOLCHAIN_FILE="$sdlSource\build-scripts\i586-pc-msdosdjgpp.cmake" `
  -DCMAKE_BUILD_TYPE=Release `
  -DSDL_SHARED=OFF `
  -DSDL_STATIC=ON `
  -DSDL_TESTS=OFF `
  -DSDL_EXAMPLES=OFF `
  -DCMAKE_DISABLE_PRECOMPILE_HEADERS=ON
Assert-NativeCommandSucceeded "SDL DOS configure"

cmake --build $sdlBuild --config Release --parallel
Assert-NativeCommandSucceeded "SDL DOS build"
