param([string]$DevkitPro, [int]$Jobs = 2)
$ErrorActionPreference = 'Stop'
$project = Split-Path $PSScriptRoot -Parent
if (-not $DevkitPro) {
    if ($env:DEVKITPRO -and (Test-Path -LiteralPath $env:DEVKITPRO)) { $DevkitPro = $env:DEVKITPRO }
    else { $DevkitPro = Join-Path (Split-Path $project -Parent) 'devkitPro' }
}
$DevkitPro = (Resolve-Path -LiteralPath $DevkitPro).Path
$bash = Join-Path $DevkitPro 'msys2/usr/bin/bash.exe'
if (-not (Test-Path -LiteralPath $bash)) { throw 'Run make from a devkitPro shell, or supply a Windows devkitPro root containing msys2.' }
function Convert-BashPath([string]$Path) {
    $value = & $bash --noprofile --norc -c 'cygpath -u "$1"' build-path $Path
    if ($LASTEXITCODE -ne 0) { throw 'Cannot convert build path' }
    return $value
}
$sdkPath = Convert-BashPath $DevkitPro
$projectPath = Convert-BashPath $project
$script = 'export DEVKITPRO="$1"; export DEVKITA64="$DEVKITPRO/devkitA64"; export PATH="$DEVKITPRO/portlibs/switch/bin:$DEVKITPRO/tools/bin:$DEVKITA64/bin:/usr/bin:$PATH"; aarch64-none-elf-pkg-config --exists freetype2 || exit 2; exec make -C "$2" -j"$3"'
$ErrorActionPreference = 'Continue'
& $bash --noprofile --norc -c $script toolbox-build $sdkPath $projectPath $Jobs
$buildExit = $LASTEXITCODE
$ErrorActionPreference = 'Stop'
if ($buildExit -ne 0) { throw "NRO build failed ($buildExit)" }
Write-Output (Join-Path $project 'gta5save.nro')
