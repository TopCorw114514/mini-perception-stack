<#
.SYNOPSIS
    Downloads and unpacks the UCI HAR dataset into a directory outside the repo.

.DESCRIPTION
    The dataset is roughly 60 MB of text files. Keeping it out of the repository
    keeps clones small; the repository only needs to know where it lives.

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File tools\fetch_uci_har.ps1
#>
[CmdletBinding()]
param(
    [string] $Destination = 'D:\datasets\UCI-HAR',
    [string] $CacheDirectory = 'D:\datasets'
)

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'

New-Item -ItemType Directory -Force -Path $CacheDirectory, $Destination | Out-Null

$archive = Join-Path $CacheDirectory 'uci-har-public.zip'
$url = 'https://archive.ics.uci.edu/static/public/240/human+activity+recognition+using+smartphones.zip'

# The UCI server is slow from some networks (single digit kilobytes per second
# has been observed) and it does not support range requests, so an interrupted
# download has to start over. curl.exe ships with Windows and reports failures
# instead of leaving a truncated file behind.
if (-not (Test-Path -LiteralPath $archive) -or (Get-Item -LiteralPath $archive).Length -eq 0) {
    Write-Host "downloading $url"
    Write-Host 'this can take a long time on a slow link'
    if (Get-Command curl.exe -ErrorAction SilentlyContinue) {
        & curl.exe -L --retry 5 --retry-delay 5 -o $archive $url
        if ($LASTEXITCODE -ne 0) {
            throw "curl failed with exit code $LASTEXITCODE - the archive is incomplete, delete it and retry"
        }
    }
    else {
        Invoke-WebRequest -Uri $url -OutFile $archive -UseBasicParsing
    }
}
else {
    Write-Host "using cached archive $archive"
}

Write-Host "unpacking into $Destination"
Expand-Archive -LiteralPath $archive -DestinationPath $Destination -Force

# The public archive ships a nested zip on some mirrors, so unpack it as well.
Get-ChildItem -LiteralPath $Destination -Filter '*.zip' -Recurse -ErrorAction SilentlyContinue |
    ForEach-Object {
        Write-Host "unpacking nested archive $($_.FullName)"
        Expand-Archive -LiteralPath $_.FullName -DestinationPath $Destination -Force
    }

$probe = Get-ChildItem -LiteralPath $Destination -Recurse -Filter 'body_acc_x_train.txt' -ErrorAction SilentlyContinue |
    Select-Object -First 1

if (-not $probe) {
    throw "body_acc_x_train.txt was not found under $Destination - the archive layout may have changed"
}

$datasetRoot = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $probe.FullName))
Write-Host ''
Write-Host "done. dataset root: $datasetRoot"
Write-Host "try: sensekit-stats `"$($probe.FullName)`""
