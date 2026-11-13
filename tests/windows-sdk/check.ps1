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

$profiles = Join-Path $BuildDir ("profile-check-" + [guid]::NewGuid().ToString('N'))
$null = New-Item -ItemType Directory -Path $profiles
$legacy = Join-Path $profiles 'overg.toml'
$first = Join-Path $profiles 'first player'
$second = Join-Path $profiles 'second player'
$null = New-Item -ItemType Directory -Path $first,$second
"# Existing player preferences`nuser_language = 4`n" | Set-Content -LiteralPath $legacy -Encoding utf8NoBOM
$original = (Get-FileHash -LiteralPath $legacy).Hash
(Get-Item -LiteralPath $legacy).IsReadOnly = $true
function Check-Profile([string] $PlayerPath, [string] $Language, [string[]] $MoreArgs = @()) {
    $actual = & $exe --profile-check $PlayerPath $legacy @MoreArgs
    if ((@($actual)[-1]).Trim() -ne $Language) { throw "Unexpected profile language: $actual" }
}
Check-Profile $first '4'
if ((Get-FileHash -LiteralPath (Join-Path $first 'overg.toml')).Hash -ne $original) { throw 'Migration changed the original TOML contents.' }
Check-Profile $first '3' @('3')
Check-Profile $second '4'
if ((Get-FileHash -LiteralPath $legacy).Hash -ne $original) { throw 'Saving one profile changed the original config.' }
Check-Profile $first '3'
Check-Profile $first '5' @('--user_language=5')
Check-Profile $first '3'
$concurrent = Join-Path $profiles 'concurrent player'
$workers = foreach ($i in 1..4) {
    $start = [System.Diagnostics.ProcessStartInfo]::new($exe)
    $start.UseShellExecute = $false
    $start.RedirectStandardOutput = $true
    foreach ($arg in '--profile-check',$concurrent,$legacy) { $start.ArgumentList.Add($arg) }
    [System.Diagnostics.Process]::Start($start)
}
foreach ($worker in $workers) {
    $worker.WaitForExit()
    $language = $worker.StandardOutput.ReadToEnd().Trim()
    if ($worker.ExitCode -ne 0 -or $language -ne '4') { throw 'Concurrent launches did not load the original preferences.' }
    $worker.Dispose()
}
if ((Get-FileHash -LiteralPath (Join-Path $concurrent 'overg.toml')).Hash -ne $original) { throw 'Concurrent migration produced an incomplete TOML.' }
(Get-Item -LiteralPath $legacy).IsReadOnly = $false
Remove-Item -LiteralPath $legacy
$fresh = Join-Path $profiles 'new player'
Check-Profile $fresh '1'
Check-Profile $fresh '5' @('5')
Check-Profile $fresh '5'
Write-Host 'Legacy settings, independent profiles, and command-line override passed.'
