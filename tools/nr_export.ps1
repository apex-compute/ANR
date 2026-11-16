# Universal Hardware-Accelerated Video Exporter (QuickSync / NVENC / AMF / CPU)
param (
    [Parameter(Mandatory=$true)]
    [string]$InDir,
    [string]$Out = "",
    [ValidateSet("av1", "hevc", "h264")]
    [string]$Codec = "av1",
    [ValidateSet("lossless", "high", "medium")]
    [string]$Quality = "high",
    [string]$Ffmpeg = "ffmpeg"
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

$PyScript = Join-Path $ScriptDir "nr_export.py"
if (-not (Test-Path $PyScript)) {
    Write-Error "nr_export.py not found at $PyScript"
    exit 1
}

$ArgsList = @($PyScript, "--in-dir", $InDir, "--codec", $Codec, "--quality", $Quality, "--ffmpeg", $Ffmpeg)
if ($Out) {
    $ArgsList += @("--out", $Out)
}

python @ArgsList
