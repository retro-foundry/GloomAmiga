param(
  [Parameter(Mandatory = $true)]
  [string] $RuntimeDir,

  [ValidateSet('windows', 'dos')]
  [string] $Platform = 'windows'
)

$ErrorActionPreference = 'Stop'

$runtimeRoot = Resolve-Path -LiteralPath $RuntimeDir
$runtimePath = $runtimeRoot.Path

function Require-Path {
  param([string] $RelativePath)

  $path = Join-Path $runtimePath $RelativePath
  if (-not (Test-Path -LiteralPath $path)) {
    throw "Missing runtime asset: $RelativePath in $runtimePath"
  }
}

Require-Path 'GLOOM.INI'
Require-Path 'amiga'
Require-Path 'amiga\maps\map1_1'
Require-Path 'amiga\data\maps\map1_2'
Require-Path 'amiga\data\txts\floor1'
Require-Path 'amiga\objs\player'
Require-Path 'amiga\misc\script'
Require-Path 'amiga\misc\smallfont2.bin'
Require-Path 'amiga\sfxs\med1'
Require-Path 'amiga\combat.iff'
Require-Path 'amiga\bullet1.bin'
Require-Path 'amiga\bullet2.bin'
Require-Path 'amiga\bullet3.bin'
Require-Path 'amiga\bullet4.bin'
Require-Path 'amiga\bullet5.bin'
Require-Path 'amiga\sparks1.bin'
Require-Path 'amiga\sparks2.bin'
Require-Path 'amiga\sparks3.bin'
Require-Path 'amiga\sparks4.bin'
Require-Path 'amiga\sparks5.bin'

if ($Platform -eq 'dos') {
  Require-Path 'GLOOMPC.EXE'
  Require-Path 'CWSDPMI.EXE'
  Require-Path 'GLOOM.BAT'
  Require-Path 'amiga\gridoff4.bin'
  Require-Path 'amiga\BLACKMAG.IFF'
  Require-Path 'amiga\SPACEHUL.IFF'
} else {
  Require-Path 'gloom_pc.exe'
  Require-Path 'amiga\gridoffs4.bin'
  Require-Path 'amiga\blackmagic.iff'
  Require-Path 'amiga\spacehulk.iff'
}

Write-Host "Runtime assets verified for $Platform at $runtimePath"
