#Requires -Version 5.1
<#
.SYNOPSIS
Builds a side-by-side D3D12 plugin for the Windows x64 ReXGlue 0.10.1 runtime.
.DESCRIPTION
The first invocation needs GitHub access and a new WorkDir. The same directory
can be reused after an interrupted submodule download or build. The script
checks the source revision, applies the source patch, and builds RelWithDebInfo.
Use a short local path for WorkDir, Git for Windows, Visual Studio 2022 C++
Build Tools with a Windows SDK, CMake 3.25 or newer, Ninja, and x64 Clang.
The target builds the GPU plugin and its SDK dependencies in WorkDir.
StageBesideExecutable optionally copies the alternate plugin beside a test EXE.
Select that plugin on the next launch with --gpu_plugin=overg-resolve.
.EXAMPLE
./scripts/build-resolve-plugin.ps1 -WorkDir C:/build/overg-resolve -LlvmBin C:/llvm/bin
.EXAMPLE
./scripts/build-resolve-plugin.ps1 -WorkDir C:/build/overg-resolve -LlvmBin C:/llvm/bin -StageBesideExecutable C:/qa/overg.exe
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$WorkDir,
    [string]$LlvmBin,
    [ValidateRange(1, 64)][int]$Jobs = 4,
    [string]$StageBesideExecutable
)

$ErrorActionPreference = 'Stop'
$revision = 'bd833a2ab69d305f99ddf57418fc38ffec8af93d'
$origin = 'https://github.com/rexglue/rexglue-sdk.git'
$patch = (Get-Item -LiteralPath (Join-Path $PSScriptRoot '../third_party/patches/rexglue-0.10.1-empty-resolve.patch')).FullName
$work = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($WorkDir)
$marker = Join-Path $work '.overg-resolve-build'
$sourceDir = Join-Path $work 'sdk'
$dllName = 'rexgpu-overg-resolverd.dll'

function Invoke-Checked([string]$Program, [string[]]$Arguments) {
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program exited with code $LASTEXITCODE." }
}

if ($StageBesideExecutable) {
    $hostExe = Get-Item -LiteralPath $StageBesideExecutable
    if ($hostExe.PSIsContainer -or $hostExe.Extension -ne '.exe') {
        throw 'StageBesideExecutable must name an existing Windows executable.'
    }
}
if ($env:VSCMD_ARG_TGT_ARCH -ne 'x64' -or $env:VSCMD_ARG_HOST_ARCH -ne 'x64') {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    $vsRoot = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if ($LASTEXITCODE -ne 0 -or !$vsRoot) { throw 'Install Visual Studio C++ Build Tools and the Windows SDK.' }
    Import-Module (Join-Path $vsRoot 'Common7/Tools/Microsoft.VisualStudio.DevShell.dll')
    Enter-VsDevShell -VsInstallPath $vsRoot -Arch amd64 -HostArch amd64 -SkipAutomaticLocation
}
$git = (Get-Command git.exe -CommandType Application).Source
$cmake = (Get-Command cmake.exe -CommandType Application).Source
$ninja = (Get-Command ninja.exe -CommandType Application).Source
if (!$LlvmBin) { $LlvmBin = Split-Path (Get-Command clang++.exe -CommandType Application).Source }
$clang = (Get-Item -LiteralPath (Join-Path $LlvmBin 'clang.exe')).FullName
$clangxx = (Get-Item -LiteralPath (Join-Path $LlvmBin 'clang++.exe')).FullName

if (Test-Path -LiteralPath $work) {
    if (!(Test-Path -LiteralPath $marker -PathType Leaf) -or
        (Get-Content -LiteralPath $marker -Raw).Trim() -ne $revision) {
        throw 'Choose a new WorkDir or a directory created by this script.'
    }
} else {
    New-Item -ItemType Directory -Path $work | Out-Null
    Set-Content -LiteralPath $marker -Value $revision -Encoding Ascii
}
if (!(Test-Path -LiteralPath $sourceDir)) {
    Invoke-Checked $git @('clone', '-c', 'core.longpaths=true', '--depth', '1', '--branch', 'v0.10.1', $origin, $sourceDir)
}
if ((Invoke-Checked $git @('-C', $sourceDir, 'rev-parse', 'HEAD')) -ne $revision -or
    (Invoke-Checked $git @('-C', $sourceDir, 'remote', 'get-url', 'origin')) -ne $origin) {
    throw 'The SDK source checkout does not match the pinned release and origin.'
}
$changedFiles = @(Invoke-Checked $git @('-C', $sourceDir, 'diff', '--name-only', 'HEAD', '--ignore-submodules=all'))
if (@($changedFiles | Where-Object { $_ -ne 'src/graphics/util/draw.cpp' }).Count) {
    throw 'The SDK source checkout has changes outside the patch.'
}
if (!$changedFiles.Count) {
    Invoke-Checked $git @('-C', $sourceDir, 'apply', '--check', $patch)
    Invoke-Checked $git @('-C', $sourceDir, 'apply', $patch)
}
$patchedBlob = Invoke-Checked $git @('-C', $sourceDir, 'hash-object', '--', 'src/graphics/util/draw.cpp')
if ($patchedBlob -ne '3b79a3442d3392b5ac30a9eb94bb95e7ad7f31e5') {
    throw 'The patched SDK source file differs from the expected content.'
}
Invoke-Checked $git @('-C', $sourceDir, 'submodule', 'update', '--init', '--recursive', '--depth', '1', '--jobs', "$Jobs")

Push-Location -LiteralPath $sourceDir
try {
    Invoke-Checked $cmake @('--preset', 'win-amd64', "-DCMAKE_C_COMPILER:FILEPATH=$clang",
        "-DCMAKE_CXX_COMPILER:FILEPATH=$clangxx", "-DCMAKE_MAKE_PROGRAM:FILEPATH=$ninja",
        '-DREXGLUE_USE_D3D12=ON', '-DREXGLUE_USE_VULKAN=OFF')
    Invoke-Checked $cmake @('--build', 'out/build/win-amd64', '--config', 'RelWithDebInfo',
        '--target', 'rexgpu-xenos', '--parallel', "$Jobs")
} finally {
    Pop-Location
}
$built = (Get-Item -LiteralPath (Join-Path $sourceDir 'out/win-amd64/RelWithDebInfo/rexgpu-xenosrd.dll')).FullName
$sha = (Get-FileHash -LiteralPath $built -Algorithm SHA256).Hash
$stageDir = if ($StageBesideExecutable) { $hostExe.DirectoryName } else { $work }
$staged = Join-Path $stageDir $dllName
if (Test-Path -LiteralPath $staged) {
    if ((Get-FileHash -LiteralPath $staged -Algorithm SHA256).Hash -ne $sha) {
        throw "A different plugin exists at $staged. The new build is at $built."
    }
} else {
    [System.IO.File]::Copy($built, $staged, $false)
}
[pscustomobject]@{ Plugin = $staged; SHA256 = $sha; Select = '--gpu_plugin=overg-resolve' }
