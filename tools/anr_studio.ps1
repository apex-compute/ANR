# ANR Studio Launcher (PowerShell)
$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$PyScript = Join-Path $ScriptDir "anr_studio.py"

if (-not (Test-Path $PyScript)) {
    Write-Error "anr_studio.py not found at $PyScript"
    exit 1
}

# Run with pythonw if available for windowed (no console) experience, else python
$PythonW = Get-Command "pythonw.exe" -ErrorAction SilentlyContinue
if ($PythonW) {
    Start-Process $PythonW.Source -ArgumentList @($PyScript)
} else {
    python $PyScript
}
