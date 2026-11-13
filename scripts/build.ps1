#Requires -Version 5.1
[CmdletBinding()]
param(
    [string]$SdkRoot,
    [string]$LlvmBin,
    [string]$BuildDir
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
if (!$SdkRoot) { $SdkRoot = Join-Path $repoRoot '../private/tools/rexglue/win-amd64' }
if (!$LlvmBin) { $LlvmBin = Join-Path $repoRoot '../private/tools/llvm/bin' }
if (!$BuildDir) { $BuildDir = Join-Path $repoRoot 'out/build/win-amd64-relwithdebinfo' }
$gameFile = Get-Item -LiteralPath (Join-Path $repoRoot '../private/game/default.xex')
$expectedHash = '51cc8b46170e7e965a9a4655727c98e7c9e83abc73a70aae8cc156d26edef453'
if ($gameFile.Length -ne 7454720) {
    throw "Unsupported default.xex size: $($gameFile.Length) bytes (expected 7454720)."
}
$sha256 = [Security.Cryptography.SHA256]::Create()
$gameStream = $gameFile.OpenRead()
try {
    $actualHash = [BitConverter]::ToString($sha256.ComputeHash($gameStream)).Replace('-', '')
} finally {
    $gameStream.Dispose()
    $sha256.Dispose()
}
if ($actualHash -ne $expectedHash) {
    throw "Unsupported default.xex SHA-256 (expected $expectedHash)."
}

$sdkPath = (Resolve-Path -LiteralPath $SdkRoot).Path
$rexglue = Join-Path $sdkPath 'bin/rexglue.exe'
$sdkVersion = & $rexglue --version
if ($LASTEXITCODE -ne 0 -or "$sdkVersion".Trim() -ne '0.10.1') {
    throw "Expected ReXGlue 0.10.1 at $rexglue (reported '$sdkVersion')."
}
$clang = (Get-Item -LiteralPath (Join-Path $LlvmBin 'clang++.exe')).FullName
$buildPath = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($BuildDir)

if ($env:VSCMD_ARG_TGT_ARCH -ne 'x64' -or $env:VSCMD_ARG_HOST_ARCH -ne 'x64') {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    $vsRoot = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if ($LASTEXITCODE -ne 0 -or !$vsRoot) {
        throw 'Install Visual Studio with the Desktop development with C++ workload.'
    }
    Import-Module (Join-Path $vsRoot 'Common7/Tools/Microsoft.VisualStudio.DevShell.dll')
    Enter-VsDevShell -VsInstallPath $vsRoot -Arch amd64 -HostArch amd64 -SkipAutomaticLocation
}
$cmake = (Get-Command cmake.exe -CommandType Application).Source
$ninja = (Get-Command ninja.exe -CommandType Application).Source

Push-Location -LiteralPath $repoRoot
try {
    & $rexglue codegen (Join-Path $repoRoot 'overg_manifest.toml')
    if ($LASTEXITCODE -ne 0) { throw "ReXGlue code generation failed ($LASTEXITCODE)." }

    $configureArgs = @(
        '--preset', 'win-amd64-relwithdebinfo', '-B', $buildPath
        "-DCMAKE_CXX_COMPILER:FILEPATH=$clang"
        "-DCMAKE_MAKE_PROGRAM:FILEPATH=$ninja"
        "-DCMAKE_PREFIX_PATH:PATH=$sdkPath"
        "-Drexglue_DIR:PATH=$sdkPath/lib/cmake/rexglue"
    )
    & $cmake @configureArgs
    if ($LASTEXITCODE -ne 0) { throw "CMake configuration failed ($LASTEXITCODE)." }

    & $cmake --build $buildPath --target overg
    if ($LASTEXITCODE -ne 0) { throw "CMake build failed ($LASTEXITCODE)." }
} finally {
    Pop-Location
}
