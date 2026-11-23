[CmdletBinding()]
param(
    [string]$Version,
    [string]$Prefix,
    [string]$Archive,
    [switch]$Yes,
    [switch]$Uninstall,
    [switch]$Update,
    [switch]$Check,
    [string]$Sdk
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$RepositoryUrl = "https://github.com/pingdart/PingCore"
$ExplicitVersion = [bool]$Version

function Fail([string]$Message) {
    throw "PingCore installer: $Message"
}

function Get-AbsolutePath([string]$Path) {
    if ([System.IO.Path]::IsPathRooted($Path)) {
        return [System.IO.Path]::GetFullPath($Path)
    }
    return [System.IO.Path]::GetFullPath((Join-Path (Get-Location).Path $Path))
}

function Test-PingCoreSdk([string]$Root) {
    return (Test-Path -LiteralPath (Join-Path $Root "bin/pingcore.exe") -PathType Leaf) -and
        (Test-Path -LiteralPath (Join-Path $Root "include/pingcore/pingcore.h") -PathType Leaf) -and
        (Test-Path -LiteralPath (Join-Path $Root "lib/cmake/PingCore/PingCoreConfig.cmake") -PathType Leaf) -and
        (Test-Path -LiteralPath (Join-Path $Root "share/pingcore/resources/pingcore/resources.bin") -PathType Leaf)
}

function Get-SdkVersion([string]$Root) {
    $PreviousHome = $env:PINGCORE_HOME
    try {
        $env:PINGCORE_HOME = $Root
        $Result = & (Join-Path $Root "bin/pingcore.exe") --version
        if ($LASTEXITCODE -ne 0 -or $Result -notmatch '^pingcore ([0-9]+\.[0-9]+\.[0-9]+)$') {
            Fail "SDK CLI cannot report its release version: $Root"
        }
        return $Matches[1]
    } finally {
        $env:PINGCORE_HOME = $PreviousHome
    }
}

function Lock-Sdk {
    if (Test-SdkLink $Prefix) {
        Fail "installation prefix must not be a link: $Prefix"
    }
    try {
        return [IO.File]::Open("$Prefix.pingcore-lock", [IO.FileMode]::CreateNew, [IO.FileAccess]::Write,
            [IO.FileShare]::None)
    } catch {
        Fail "another installer may be using this SDK; inspect $Prefix.pingcore-lock before retrying"
    }
}

function Confirm-Action([string]$Description) {
    Write-Host $Description
    if ($Yes) {
        return
    }
    $Answer = Read-Host "Continue? [y/N]"
    if ($Answer -notmatch "^(?i:y|yes)$") {
        Fail "cancelled"
    }
}

function Assert-SafePrefix([string]$Path) {
    $Root = [System.IO.Path]::GetPathRoot($Path)
    if ([string]::IsNullOrWhiteSpace($Path) -or
        $Path.Equals($Root, [System.StringComparison]::OrdinalIgnoreCase) -or
        $Path.Equals($env:USERPROFILE, [System.StringComparison]::OrdinalIgnoreCase)) {
        Fail "unsafe installation prefix: $Path"
    }
}

function Set-UserEnvironment([string]$SdkRoot) {
    $OldHome = [Environment]::GetEnvironmentVariable("PINGCORE_HOME", "User")
    $OldPath = [Environment]::GetEnvironmentVariable("Path", "User")
    $Entries = [System.Collections.Generic.List[string]]::new()
    if ($OldPath) {
        foreach ($Entry in $OldPath.Split(";", [System.StringSplitOptions]::RemoveEmptyEntries)) {
            if ($OldHome -and $Entry.Equals((Join-Path $OldHome "bin"), [System.StringComparison]::OrdinalIgnoreCase)) {
                continue
            }
            if (-not $Entry.Equals((Join-Path $SdkRoot "bin"), [System.StringComparison]::OrdinalIgnoreCase)) {
                $Entries.Add($Entry)
            }
        }
    }
    $Entries.Add((Join-Path $SdkRoot "bin"))
    try {
        [Environment]::SetEnvironmentVariable("PINGCORE_HOME", $SdkRoot, "User")
        [Environment]::SetEnvironmentVariable("Path", ($Entries -join ";"), "User")
    } catch {
        [Environment]::SetEnvironmentVariable("PINGCORE_HOME", $OldHome, "User")
        [Environment]::SetEnvironmentVariable("Path", $OldPath, "User")
        throw
    }
}

function Remove-UserEnvironment([string]$SdkRoot) {
    $CurrentHome = [Environment]::GetEnvironmentVariable("PINGCORE_HOME", "User")
    if (-not $CurrentHome -or
        -not $CurrentHome.Equals($SdkRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        return
    }
    $CurrentPath = [Environment]::GetEnvironmentVariable("Path", "User")
    $Entries = [System.Collections.Generic.List[string]]::new()
    if ($CurrentPath) {
        foreach ($Entry in $CurrentPath.Split(";", [System.StringSplitOptions]::RemoveEmptyEntries)) {
            if (-not $Entry.Equals((Join-Path $SdkRoot "bin"), [System.StringComparison]::OrdinalIgnoreCase)) {
                $Entries.Add($Entry)
            }
        }
    }
    [Environment]::SetEnvironmentVariable("PINGCORE_HOME", $null, "User")
    [Environment]::SetEnvironmentVariable("Path", ($Entries -join ";"), "User")
}

function Test-SdkLink([string]$Path) {
    $Item = Get-Item -LiteralPath $Path -Force -ErrorAction SilentlyContinue
    return $Item -and ($Item.Attributes -band [IO.FileAttributes]::ReparsePoint)
}

function Assert-SdkSelection([string]$Root, [string]$ExpectedVersion) {
    if (-not (Test-PingCoreSdk $Root) -or
        -not (Test-Path -LiteralPath (Join-Path $Root "share/pingcore/tools") -PathType Container) -or
        -not (Test-Path -LiteralPath (Join-Path $Root "share/pingcore/skills/pingcore-app-development") -PathType Container)) {
        Fail "SDK directory is incomplete: $Root"
    }
    if ((Get-SdkVersion $Root) -ne $ExpectedVersion) {
        Fail "SDK version does not match $($ExpectedVersion): $Root"
    }
    $PreviousHome = $env:PINGCORE_HOME
    try {
        $env:PINGCORE_HOME = $Root
        & (Join-Path $Root "bin/pingcore.exe") sdk --help 2>&1 | Out-Null
        if ($LASTEXITCODE -ne 0) {
            Fail "PingCore $ExpectedVersion does not support SDK switching"
        }
    } finally {
        $env:PINGCORE_HOME = $PreviousHome
    }
}

function Write-SdkSelection {
    Write-Host "Selected PingCore SDK $Version at $Prefix"
    Write-Host "Restart the terminal to use the updated user environment."
}

function Select-Sdk {
    Assert-SdkSelection $Prefix $Version
    Set-UserEnvironment $Prefix
}

if ($Sdk -and $Sdk -notin @("list", "use", "update")) {
    Fail "invalid SDK command '$Sdk'; expected list, use, or update"
}
if ($Check -and -not $Update -and $Sdk -ne "update") {
    Fail "-Check requires an update"
}
if ($Update -and (-not $Prefix -or $Uninstall)) {
    Fail "-Update requires -Prefix and cannot be combined with -Uninstall"
}
if ($Sdk -and (-not $Prefix -or $Update -or $Uninstall -or $Archive)) {
    Fail "SDK management requires the current SDK prefix"
}
if ($Uninstall -and ($Version -or $Archive)) {
    Fail "-Uninstall cannot be combined with -Version or -Archive"
}
if ($Version -and $Archive) {
    Fail "-Version and -Archive cannot be combined"
}
if (-not $env:LOCALAPPDATA) {
    Fail "LOCALAPPDATA is not configured"
}
if (-not $Prefix) {
    $Prefix = Join-Path $env:LOCALAPPDATA "PingCore"
}
$Prefix = Get-AbsolutePath $Prefix
Assert-SafePrefix $Prefix

if ($Sdk) {
    if (-not (Test-PingCoreSdk $Prefix)) {
        Fail "SDK management requires an installed SDK: $Prefix"
    }
    $CurrentPrefix = $Prefix
    $CurrentVersion = Get-SdkVersion $CurrentPrefix
    $OriginalPrefix = $CurrentPrefix
    $VersionsDirectory = Split-Path -Parent $CurrentPrefix
    if ($VersionsDirectory.EndsWith("-versions", [StringComparison]::OrdinalIgnoreCase) -and
        (Split-Path -Leaf $CurrentPrefix) -eq $CurrentVersion) {
        $OriginalPrefix = $VersionsDirectory.Substring(0, $VersionsDirectory.Length - "-versions".Length)
    } else {
        $VersionsDirectory = "$OriginalPrefix-versions"
    }
    if (Test-SdkLink $VersionsDirectory) {
        Fail "SDK versions directory must not be a link: $VersionsDirectory"
    }
    if ($Sdk -eq "list") {
        $DefaultPrefix = [Environment]::GetEnvironmentVariable("PINGCORE_HOME", "User")
        Write-Host "Installed PingCore SDKs (* default):"
        if (Test-PingCoreSdk $OriginalPrefix) {
            $Marker = if ($OriginalPrefix -eq $DefaultPrefix) { "*" } else { " " }
            Write-Host ("{0} {1,-10} {2}" -f $Marker, (Get-SdkVersion $OriginalPrefix), $OriginalPrefix)
        }
        if (Test-Path -LiteralPath $VersionsDirectory -PathType Container) {
            Get-ChildItem -LiteralPath $VersionsDirectory -Directory | Where-Object {
                $_.Name -match '^\d+\.\d+\.\d+$' -and
                -not ($_.Attributes -band [IO.FileAttributes]::ReparsePoint) -and (Test-PingCoreSdk $_.FullName)
            } | ForEach-Object {
                $Marker = if ($_.FullName -eq $DefaultPrefix) { "*" } else { " " }
                Write-Host ("{0} {1,-10} {2}" -f $Marker, $_.Name, $_.FullName)
            }
        }
        exit 0
    }
    if ($Sdk -eq "use" -and $Version -cnotmatch '^v?[0-9]+\.[0-9]+\.[0-9]+$') {
        Fail "expected a major.minor.patch release version"
    }
    if ($Sdk -eq "update" -and $Version) {
        Fail "SDK update selects the latest stable release; use sdk use to select a version"
    }
}

if ($Uninstall) {
    if (-not (Test-PingCoreSdk $Prefix)) {
        Fail "refusing to remove a directory that is not a PingCore SDK: $Prefix"
    }
    Confirm-Action "Uninstall PingCore SDK`n  SDK: $Prefix"
    $SdkLock = Lock-Sdk
    try {
        Remove-UserEnvironment $Prefix
        Remove-Item -LiteralPath $Prefix -Recurse -Force
    } finally {
        $SdkLock.Dispose()
        Remove-Item -LiteralPath "$Prefix.pingcore-lock"
    }
    Write-Host "PingCore SDK removed from $Prefix"
    exit 0
}

$Architecture = $(if ($env:PROCESSOR_ARCHITEW6432) { $env:PROCESSOR_ARCHITEW6432 } else { $env:PROCESSOR_ARCHITECTURE }).ToLowerInvariant()

if ($Architecture -notin @("amd64", "x64", "x86_64")) {
    Fail "Windows SDK archives currently support x86_64 hosts only"
}

$HostArchitecture = "x86_64"

if ($Update) {
    if (-not (Test-PingCoreSdk $Prefix)) {
        Fail "-Update requires an installed PingCore SDK: $Prefix"
    }
    if (Test-Path -LiteralPath (Join-Path $Prefix "CMakeLists.txt")) {
        Fail "refusing to update a source checkout"
    }
    $CurrentVersion = Get-SdkVersion $Prefix
}

$ReleaseTag = $null
if (-not $Archive) {
    if (-not $Version) {
        try {
            $Response = (Invoke-WebRequest -Uri "$RepositoryUrl/releases/latest" -Method Head -UseBasicParsing).BaseResponse
            $ReleaseUri = if ($Response -is [System.Net.HttpWebResponse]) {
                $Response.ResponseUri
            } else {
                $Response.RequestMessage.RequestUri
            }
        } catch {
            Fail "failed to resolve the latest PingCore release: $_"
        }

        $TagPrefix = [regex]::Escape("$RepositoryUrl/releases/tag/")
        if (-not $ReleaseUri -or $ReleaseUri.AbsoluteUri -cnotmatch "^$TagPrefix(v[0-9A-Za-z._-]+)$") {
            Fail "latest PingCore release did not redirect to a valid tag: $ReleaseUri"
        }
        $ReleaseTag = $Matches[1]
        $Version = $ReleaseTag.Substring(1)
    } else {
        $Version = $Version.TrimStart("v")
        $ReleaseTag = "v$Version"
    }
    if ($Version -notmatch "^[0-9A-Za-z._-]+$") {
        Fail "invalid version: $Version"
    }

    $ArchiveName = "pingcore-sdk-$Version-windows-$HostArchitecture.zip"
    $ArchiveSource = "$RepositoryUrl/releases/download/$ReleaseTag/$ArchiveName"
    $ArchiveDisplay = $ArchiveSource
} else {
    $Archive = Get-AbsolutePath $Archive
    if (-not (Test-Path -LiteralPath $Archive -PathType Leaf)) {
        Fail "archive does not exist: $Archive"
    }
    $ArchiveName = Split-Path -Leaf $Archive
    $ArchiveDisplay = $Archive
}

if ($ArchiveName -notmatch "^pingcore-sdk-.+-windows-x86_64\.zip$") {
    Fail "archive does not match this host: $ArchiveName"
}
if ((Test-Path -LiteralPath $Prefix) -and -not (Test-PingCoreSdk $Prefix)) {
    Fail "installation prefix exists but is not a PingCore SDK: $Prefix"
}

if ($Update) {
    if ($ArchiveName -notmatch '^pingcore-sdk-([0-9]+\.[0-9]+\.[0-9]+)-windows-x86_64\.zip$') {
        Fail "expected a major.minor.patch release version"
    }
    $Version = $Matches[1]
    Write-Host "Update PingCore SDK`n  SDK: $Prefix`n  Current: $CurrentVersion`n  Target: $Version`n  Package: windows-x86_64"
    if ([version]$CurrentVersion -eq [version]$Version) {
        Write-Host "PingCore SDK is already at the requested version."
        exit 0
    }
    if (-not $ExplicitVersion -and -not $Archive -and [version]$CurrentVersion -gt [version]$Version) {
        Write-Host "The installed SDK is newer; use --version to explicitly downgrade."
        exit 0
    }
    if ($Check) {
        Write-Host "PingCore SDK update available."
        exit 0
    }
    Confirm-Action "The entire SDK will be replaced, including local modifications. Stop SDK builds and tools first."
} elseif ($Sdk) {
    if ($Version -notmatch '^[0-9]+\.[0-9]+\.[0-9]+$') {
        Fail "expected a major.minor.patch release version"
    }
    if ($Sdk -eq "update") {
        Write-Host "Current PingCore SDK: $CurrentVersion"
        Write-Host "Latest stable SDK: $Version"
        if ([version]$CurrentVersion -gt [version]$Version) {
            Write-Host "The installed SDK is newer than the latest stable release."
            exit 0
        }
        if ([version]$CurrentVersion -eq [version]$Version) {
            Write-Host "PingCore SDK is up to date."
            exit 0
        }
        if ($Check) {
            Write-Host "PingCore SDK update available."
            exit 0
        }
    }
    if ([version]$Version -lt [version]"0.4.0") {
        Fail "SDK switching starts with PingCore 0.4.0"
    }

    $Prefix = Join-Path $VersionsDirectory $Version
    if ($Version -eq $CurrentVersion) {
        $Prefix = $CurrentPrefix
    } elseif (-not (Test-Path -LiteralPath $Prefix) -and (Test-PingCoreSdk $OriginalPrefix) -and
        (Get-SdkVersion $OriginalPrefix) -eq $Version) {
        $Prefix = $OriginalPrefix
    }
    if (Test-SdkLink $Prefix) {
        Fail "SDK directory must not be a link: $Prefix"
    }
    if (Test-Path -LiteralPath $Prefix) {
        if ($Sdk -eq "update") {
            Confirm-Action "Switch to the installed PingCore SDK $Version at $Prefix"
        }
        $SdkLock = Lock-Sdk
        try {
            Select-Sdk
        } finally {
            $SdkLock.Dispose()
            Remove-Item -LiteralPath "$Prefix.pingcore-lock"
        }
        Write-SdkSelection
        exit 0
    }
    Confirm-Action "Install and select PingCore SDK $Version at $Prefix"
} else {
    Confirm-Action "Install PingCore SDK`n  Archive: $ArchiveDisplay`n  SDK: $Prefix"
}

$TemporaryDirectory = Join-Path ([System.IO.Path]::GetTempPath()) ("pingcore-sdk-" + [guid]::NewGuid())
$StagingDirectory = $null
$BackupDirectory = $null
$SdkLock = $null
New-Item -ItemType Directory -Path $TemporaryDirectory | Out-Null
try {
    if ($Update) {
        $SdkLock = Lock-Sdk
        if ((Get-SdkVersion $Prefix) -ne $CurrentVersion) {
            Fail "SDK changed during the update check; retry"
        }
    }
    if (-not $Archive) {
        $Archive = Join-Path $TemporaryDirectory $ArchiveName
        Invoke-WebRequest -Uri $ArchiveSource -OutFile $Archive -UseBasicParsing
        Invoke-WebRequest -Uri "$ArchiveSource.sha256" -OutFile "$Archive.sha256" -UseBasicParsing
    }

    $ChecksumPath = "$Archive.sha256"
    if (-not (Test-Path -LiteralPath $ChecksumPath -PathType Leaf)) {
        Fail "archive checksum does not exist: $ChecksumPath"
    }
    $ChecksumParts = ((Get-Content -LiteralPath $ChecksumPath -TotalCount 1).Trim() -split "\s+")
    if ($ChecksumParts.Count -lt 2 -or $ChecksumParts[1] -ne $ArchiveName) {
        Fail "archive checksum is malformed or names a different file: $ChecksumPath"
    }
    $ActualChecksum = (Get-FileHash -LiteralPath $Archive -Algorithm SHA256).Hash
    if (-not $ActualChecksum.Equals($ChecksumParts[0], [System.StringComparison]::OrdinalIgnoreCase)) {
        Fail "archive checksum does not match: $ArchiveName"
    }

    $ExtractDirectory = Join-Path $TemporaryDirectory "extract"
    $ArchiveRoot = $ArchiveName.Substring(0, $ArchiveName.Length - ".zip".Length)
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $Zip = [IO.Compression.ZipFile]::OpenRead($Archive)
    try {
        foreach ($Entry in $Zip.Entries) {
            $Name = $Entry.FullName.Replace('\', '/')
            if (-not $Name.StartsWith("$ArchiveRoot/", [StringComparison]::Ordinal) -or
                $Name -match '(^|/)\.\.(/|$)|:' -or
                (($Entry.ExternalAttributes -shr 16) -band 0xF000) -eq 0xA000) {
                Fail "archive contains an invalid path or symbolic link"
            }
        }
    } finally {
        $Zip.Dispose()
    }
    Expand-Archive -LiteralPath $Archive -DestinationPath $ExtractDirectory
    $ExtractedSdk = Join-Path $ExtractDirectory $ArchiveRoot
    if (-not (Test-PingCoreSdk $ExtractedSdk)) {
        Fail "archive does not contain a complete PingCore SDK"
    }
    if ($Update -and (Get-SdkVersion $ExtractedSdk) -ne $Version) {
        Fail "archive SDK version does not match $Version"
    }
    if ($Sdk) {
        Assert-SdkSelection $ExtractedSdk $Version
    }

    $Parent = Split-Path -Parent $Prefix
    New-Item -ItemType Directory -Path $Parent -Force | Out-Null
    if (-not $SdkLock) {
        $SdkLock = Lock-Sdk
    }
    if ($Sdk -and (Test-Path -LiteralPath $Prefix)) {
        Select-Sdk
        Write-SdkSelection
        exit 0
    }
    $StagingDirectory = Join-Path $Parent (".pingcore-install-" + [guid]::NewGuid())
    Move-Item -LiteralPath $ExtractedSdk -Destination $StagingDirectory
    if (Test-Path -LiteralPath $Prefix) {
        $BackupDirectory = Join-Path $Parent (".pingcore-backup-" + [guid]::NewGuid())
        Move-Item -LiteralPath $Prefix -Destination $BackupDirectory
    }
    try {
        Move-Item -LiteralPath $StagingDirectory -Destination $Prefix
        $StagingDirectory = $null
        if ($Update) {
            if ((Get-SdkVersion $Prefix) -ne $Version) {
                Fail "installed SDK version does not match $Version"
            }
        } elseif ($Sdk) {
            Select-Sdk
        } else {
            Set-UserEnvironment $Prefix
        }
    } catch {
        Remove-Item -LiteralPath $Prefix -Recurse -Force -ErrorAction SilentlyContinue
        if ($BackupDirectory -and (Test-Path -LiteralPath $BackupDirectory)) {
            Move-Item -LiteralPath $BackupDirectory -Destination $Prefix
            $BackupDirectory = $null
        }
        throw
    }

    if ($BackupDirectory -and (Test-Path -LiteralPath $BackupDirectory)) {
        Remove-Item -LiteralPath $BackupDirectory -Recurse -Force
        $BackupDirectory = $null
    }
} finally {
    if ($SdkLock) {
        $SdkLock.Dispose()
        Remove-Item -LiteralPath "$Prefix.pingcore-lock"
    }
    if ($StagingDirectory -and (Test-Path -LiteralPath $StagingDirectory)) {
        Remove-Item -LiteralPath $StagingDirectory -Recurse -Force
    }
    if ($BackupDirectory -and (Test-Path -LiteralPath $BackupDirectory)) {
        Write-Warning "Previous PingCore SDK remains at $BackupDirectory"
    }
    if (Test-Path -LiteralPath $TemporaryDirectory) {
        Remove-Item -LiteralPath $TemporaryDirectory -Recurse -Force
    }
}

if ($Sdk) {
    Write-SdkSelection
} else {
    Write-Host "PingCore SDK installed at $Prefix"
    if (-not $Update) {
        Write-Host "Restart the terminal to use the updated user environment."
    }
}
