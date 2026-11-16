# ANR Mathematical Quality Evaluator (PowerShell)
param (
    [Parameter(Mandatory=$true)]
    [string]$Ref,
    [Parameter(Mandatory=$true)]
    [string]$Test,
    [string]$Out = ""
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$PyScript = Join-Path $ScriptDir "nr_metrics.py"

if (-not (Test-Path $PyScript)) {
    Write-Error "nr_metrics.py not found at $PyScript"
    exit 1
}

$ArgsList = @($PyScript, "--ref", $Ref, "--test", $Test)
if ($Out) {
    $ArgsList += @("--out", $Out)
}

python @ArgsList
