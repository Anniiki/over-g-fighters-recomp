#Requires -Version 7.4
[CmdletBinding()]
param(
    [Parameter(Mandatory)] [string]$SdkRoot,
    [Parameter(Mandatory)] [string]$LlvmBin,
    [Parameter(Mandatory)] [string]$BuildDir
)

$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $true
$sdk = (Get-Item -LiteralPath $SdkRoot).FullName
$clang = (Get-Item -LiteralPath (Join-Path $LlvmBin 'clang++.exe')).FullName
$version = & (Join-Path $sdk 'bin/rexglue.exe') --version
if ("$version".Trim() -ne '0.10.1') { throw "Expected ReXGlue 0.10.1, found $version." }
$compiler = & $clang --version
if ($compiler[0] -notmatch '^clang version 20\.1\.8\b') { throw "Unexpected compiler: $($compiler[0])" }

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'The Visual Studio C++ build tools are required.' }
Import-Module (Join-Path $vs 'Common7/Tools/Microsoft.VisualStudio.DevShell.dll')
Enter-VsDevShell -VsInstallPath $vs -Arch amd64 -HostArch amd64 -SkipAutomaticLocation

cmake -G Ninja -S $PSScriptRoot -B $BuildDir "-DCMAKE_CXX_COMPILER=$clang" "-Drexglue_DIR=$sdk/lib/cmake/rexglue" -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build $BuildDir

$env:PATH = (Join-Path $sdk 'bin') + ';' + $env:PATH
$exe = Join-Path $BuildDir 'sdk_check.exe'
foreach ($language in 1, 3, 4, 5) {
    $reported = & $exe "--user_language=$language"
    if ("$reported".Trim() -ne "$language") { throw "Expected language $language, received '$reported'." }
    Write-Host "Language $language passed."
}
