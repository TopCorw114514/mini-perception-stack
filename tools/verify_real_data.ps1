<#
.SYNOPSIS
    Runs the stage 1 acceptance check on the real UCI HAR data.

.DESCRIPTION
    Two independent implementations of the same statistic are compared:

      1. sensekit-stats (C++, the thing this repository is about)
      2. tools/cross_check_numpy.py (numpy, written separately)

    If they agree, the numbers in the feature tables of stage 2 can be trusted.

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File tools\verify_real_data.ps1
#>
[CmdletBinding()]
param(
    [string] $DatasetRoot = 'D:\datasets\UCI-HAR',
    [string] $Signal = 'train\Inertial Signals\body_acc_x_train.txt',
    [string] $Exe = 'build\msvc\Release\sensekit-stats.exe',
    [string] $Python = 'D:\Python312\python.exe',
    [string] $Columns = '0,1,2',
    [double] $Tolerance = 0.01
)

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$signalPath = Get-ChildItem -LiteralPath $DatasetRoot -Recurse -Filter (Split-Path -Leaf $Signal) -ErrorAction SilentlyContinue |
    Where-Object { $_.FullName -like "*$Signal" } |
    Select-Object -First 1

if (-not $signalPath) {
    throw "could not find $Signal under $DatasetRoot - run tools\fetch_uci_har.ps1 first"
}

$exePath = Join-Path $repoRoot $Exe
if (-not (Test-Path -LiteralPath $exePath)) {
    throw "missing $exePath - build the Release preset first"
}

$scratch = Join-Path $repoRoot '.bootstrap\verify'
New-Item -ItemType Directory -Force -Path $scratch | Out-Null
$cppCsv = Join-Path $scratch 'cpp.csv'
$numpyCsv = Join-Path $scratch 'numpy.csv'

Write-Host "signal : $($signalPath.FullName)"
Write-Host "columns: $Columns"
Write-Host ''

& $exePath $signalPath.FullName --columns $Columns --output $cppCsv
if ($LASTEXITCODE -ne 0) { throw "sensekit-stats exited with $LASTEXITCODE" }

& $Python (Join-Path $repoRoot 'tools\cross_check_numpy.py') $signalPath.FullName --columns $Columns |
    Set-Content -LiteralPath $numpyCsv -Encoding ascii

$cpp = Import-Csv $cppCsv
$numpy = Import-Csv $numpyCsv

if ($cpp.Count -ne $numpy.Count) {
    throw "row count mismatch: c++ $($cpp.Count) vs numpy $($numpy.Count)"
}

$worst = 0.0
$failures = 0

Write-Host ('{0,-12} {1,-8} {2,-20} {3,-20} {4}' -f 'column', 'field', 'c++', 'numpy', 'relative diff')
foreach ($index in 0..($cpp.Count - 1)) {
    foreach ($field in 'mean', 'variance', 'rms') {
        $left = [double] $cpp[$index].$field
        $right = [double] $numpy[$index].$field
        $scale = [Math]::Max([Math]::Abs($right), 1e-12)
        $relative = [Math]::Abs($left - $right) / $scale
        if ($relative -gt $worst) { $worst = $relative }
        if ($relative -gt $Tolerance) { $failures++ }
        Write-Host ('{0,-12} {1,-8} {2,-20} {3,-20} {4:P6}' -f $cpp[$index].column, $field, $left, $right, $relative)
    }
}

Write-Host ''
Write-Host "worst relative difference: $($worst.ToString('P6'))"

if ($failures -gt 0) {
    throw "$failures field(s) disagree by more than $($Tolerance.ToString('P2'))"
}

Write-Host 'PASS - the C++ and the numpy numbers agree'
