# find_dlssg.ps1: Locates nvngx_dlssg.dll on the system (NVIDIA driver, game installs, OptiScaler)
# Fixes Issue #63 (run.cmd missing find_dlssg.ps1)

[CmdletBinding()]
param()

# 1. Registry lookup (Official NGX Core registry key)
try {
    $reg = Get-ItemProperty -Path "HKLM:\SOFTWARE\NVIDIA Corporation\Global\NGXCore" -Name "FullPath" -ErrorAction SilentlyContinue
    if ($reg -and $reg.FullPath) {
        $candidate = Join-Path $reg.FullPath "nvngx_dlssg.dll"
        if (Test-Path $candidate) {
            Write-Output $candidate
            exit 0
        }
    }
} catch {}

# 2. DriverStore search
$driverStore = "$env:SystemRoot\System32\DriverStore\FileRepository"
if (Test-Path $driverStore) {
    $match = Get-ChildItem -Path $driverStore -Filter "nvngx_dlssg.dll" -Recurse -ErrorAction SilentlyContinue -Depth 2 | Select-Object -First 1
    if ($match) {
        Write-Output $match.FullName
        exit 0
    }
}

# 3. Known Steam and game library folders
$steamCommon = "$env:ProgramFiles (x86)\Steam\steamapps\common"
if (Test-Path $steamCommon) {
    $match = Get-ChildItem -Path $steamCommon -Filter "nvngx_dlssg.dll" -Recurse -ErrorAction SilentlyContinue -Depth 4 | Select-Object -First 1
    if ($match) {
        Write-Output $match.FullName
        exit 0
    }
}

# 4. OptiScaler custom directories
$localOpti = "$env:LOCALAPPDATA\Programs\OptiScaler"
if (Test-Path $localOpti) {
    $match = Get-ChildItem -Path $localOpti -Filter "nvngx_dlssg.dll" -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($match) {
        Write-Output $match.FullName
        exit 0
    }
}

Write-Warning "nvngx_dlssg.dll could not be located on this system."
exit 1
