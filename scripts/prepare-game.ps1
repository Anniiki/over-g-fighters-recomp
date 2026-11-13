#Requires -Version 5.1
[CmdletBinding()]
param(
    [string]$IsoPath,
    [string]$GameRoot,
    [string]$XdvdfsPath
)

$ErrorActionPreference = 'Stop'
$isoSize = 7834892288L
$isoFingerprint = '47e6c331baee6b10bbaf0f2fd6172446dc5d35790bccb6af083505f4e0cf61d7'
$repoRoot = (Get-Item -LiteralPath (Join-Path $PSScriptRoot '..')).FullName
$manifest = Join-Path $repoRoot 'config/disc-files.sha256'
if (!$GameRoot) { $GameRoot = Join-Path $repoRoot '../private/game' }
$destination = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($GameRoot)
$destination = [System.IO.Path]::GetFullPath($destination)
if ($destination.TrimEnd('\', '/') -eq [System.IO.Path]::GetPathRoot($destination).TrimEnd('\', '/')) {
    throw 'Choose a game directory below the volume root.'
}
$destination = $destination.TrimEnd('\', '/')
if ($destination -eq $repoRoot -or
    $destination.StartsWith($repoRoot + '\', [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Choose a game directory outside the checkout.'
}

function Confirm-GameFiles([string]$Directory) {
    foreach ($entry in [System.IO.File]::ReadLines($manifest)) {
        $hash, $size, $name = $entry -split ' ', 3
        $path = Join-Path $Directory $name
        if (![System.IO.File]::Exists($path)) {
            throw "Incomplete extraction. Missing disc file: $name"
        }
        $stream = [System.IO.File]::OpenRead($path)
        try {
            if ($stream.Length -ne [long]$size) {
                throw "Incorrect size for '$name'. Expected $size bytes, found $($stream.Length)."
            }
            $actual = (Get-FileHash -InputStream $stream -Algorithm SHA256).Hash
            if ($actual -ne $hash) {
                throw "Incorrect SHA-256 for '$name'. Expected $hash, found $actual."
            }
        } finally {
            $stream.Dispose()
        }
    }
    Write-Host "Verified all disc files in '$Directory'."
}

$isoStream = $null
$staging = $null
try {
    if ($IsoPath) {
        $isoFile = Get-Item -LiteralPath $IsoPath
        if ($isoFile -isnot [System.IO.FileInfo]) { throw 'The ISO path must point to a file.' }
        $isoStream = [System.IO.File]::OpenRead($isoFile.FullName)
        if ($isoStream.Length -ne $isoSize) {
            throw "Unsupported disc size. Expected $isoSize bytes, found $($isoStream.Length)."
        }
        $actualIsoHash = (Get-FileHash -InputStream $isoStream -Algorithm SHA256).Hash
        if ($actualIsoHash -ne $isoFingerprint) {
            throw "Unsupported disc SHA-256. Expected $isoFingerprint, found $actualIsoHash."
        }
        Write-Host "Verified source ISO SHA-256: $actualIsoHash"
    }

    if ([System.IO.Directory]::Exists($destination)) {
        Confirm-GameFiles $destination
        return
    }
    if (Test-Path -LiteralPath $destination) { throw "Destination is a file: $destination" }
    if (!$isoStream) { throw "No extraction found at '$destination'. Supply -IsoPath to create it." }

    if ($XdvdfsPath) {
        $tool = (Get-Item -LiteralPath $XdvdfsPath).FullName
    } else {
        $command = Get-Command xdvdfs.exe -CommandType Application -ErrorAction SilentlyContinue |
            Select-Object -First 1
        if (!$command) { throw 'Install XDVDFS 0.8.3 or pass the executable with -XdvdfsPath.' }
        $tool = $command.Source
    }
    $version = & $tool --version
    if ($LASTEXITCODE -ne 0 -or "$version".Trim() -ne 'xdvdfs-cli 0.8.3') {
        throw "Expected XDVDFS 0.8.3 at '$tool', found '$version'."
    }

    $parent = [System.IO.Path]::GetDirectoryName($destination)
    [void][System.IO.Directory]::CreateDirectory($parent)
    $stageName = '.overg-unpack-' + [guid]::NewGuid().ToString('N')
    $stagePath = Join-Path $parent $stageName
    # New-Item fails if another process already owns this name.
    $staging = (New-Item -ItemType Directory -Path $stagePath).FullName
    & $tool unpack $isoFile.FullName $staging
    if ($LASTEXITCODE -ne 0) { throw "XDVDFS unpack failed with exit code $LASTEXITCODE." }
    Confirm-GameFiles $staging
    try {
        [System.IO.Directory]::Move($staging, $destination)
        $staging = $null
        Write-Host "Prepared '$destination'."
    } catch [System.IO.IOException] {
        if (![System.IO.Directory]::Exists($destination)) { throw }
        # Another preparer may have published its verified extraction first.
        Confirm-GameFiles $destination
    }
} finally {
    if ($isoStream) { $isoStream.Dispose() }
    if ($staging -and [System.IO.Directory]::Exists($staging)) {
        $owned = Get-Item -LiteralPath $staging -Force
        if ($owned.Parent.FullName -ne $parent -or $owned.Name -ne $stageName -or
            ($owned.Attributes -band [System.IO.FileAttributes]::ReparsePoint)) {
            throw "Unexpected temporary directory: $staging"
        }
        Remove-Item -LiteralPath $owned.FullName -Recurse -Force
    }
}
