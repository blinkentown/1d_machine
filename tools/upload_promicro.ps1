[CmdletBinding()]
param(
    [string]$Environment = "sparkfun_promicro16",
    [string]$ApplicationPort,
    [ValidateRange(10, 300)]
    [int]$TimeoutSeconds = 90,
    [ValidateRange(100, 2000)]
    [int]$PortStabilizationMs = 350,
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent $PSScriptRoot
$platformIo = Join-Path $env:USERPROFILE ".platformio\penv\Scripts\platformio.exe"
$avrdude = Join-Path $env:USERPROFILE ".platformio\packages\tool-avrdude\avrdude.exe"
$avrdudeConfig = Join-Path $env:USERPROFILE ".platformio\packages\tool-avrdude\avrdude.conf"
$firmwareRelative = ".pio\build\$Environment\firmware.hex"
$firmware = Join-Path $projectRoot $firmwareRelative
$proMicroHardwarePattern =
    "VID_1B4F&PID_(9205|9206)|VID_2341&PID_(0037|8037)"

function Get-ProMicroPorts {
    return @(Get-CimInstance Win32_SerialPort -ErrorAction SilentlyContinue |
        Where-Object { $_.PNPDeviceID -match $proMicroHardwarePattern })
}

function Get-PortKey($port) {
    return "$($port.DeviceID)|$($port.PNPDeviceID)"
}

function Show-Ports($ports) {
    $ports | ForEach-Object {
        Write-Host "  $($_.DeviceID)  $($_.PNPDeviceID)"
    }
}

foreach ($requiredTool in @($platformIo, $avrdude, $avrdudeConfig)) {
    if (-not (Test-Path -LiteralPath $requiredTool)) {
        throw "Required PlatformIO tool not found: $requiredTool"
    }
}

if (-not $SkipBuild) {
    Push-Location $projectRoot
    try {
        & $platformIo run -e $Environment
        if ($LASTEXITCODE -ne 0) {
            throw "PlatformIO build failed with exit code $LASTEXITCODE"
        }
    }
    finally {
        Pop-Location
    }
}

if (-not (Test-Path -LiteralPath $firmware)) {
    throw "Firmware image not found: $firmware"
}

$initialPorts = @(Get-ProMicroPorts)
if ($initialPorts.Count -eq 0) {
    throw "No SparkFun/Arduino ATmega32U4 serial interface was detected."
}

if ($ApplicationPort) {
    $application = $initialPorts |
        Where-Object { $_.DeviceID -ieq $ApplicationPort } |
        Select-Object -First 1
    if (-not $application) {
        Write-Host "Detected Pro Micro ports:"
        Show-Ports $initialPorts
        throw "Requested application port $ApplicationPort was not detected."
    }
}
elseif ($initialPorts.Count -eq 1) {
    $application = $initialPorts[0]
}
else {
    Write-Host "Multiple Pro Micro ports were detected:"
    Show-Ports $initialPorts
    throw "Use -ApplicationPort COMx to select the intended board."
}

$applicationKey = Get-PortKey $application
$otherInitialKeys = @($initialPorts |
    Where-Object { (Get-PortKey $_) -ne $applicationKey } |
    ForEach-Object { Get-PortKey $_ })

Write-Host "Application interface: $($application.DeviceID)"
Write-Host "Upload watcher armed for $TimeoutSeconds seconds."
Write-Host "Make exactly one quick RST-to-GND contact, then release it."

$deadline = (Get-Date).AddSeconds($TimeoutSeconds)
$applicationDisappeared = $false
$bootloader = $null

while ((Get-Date) -lt $deadline -and -not $bootloader) {
    $currentPorts = @(Get-ProMicroPorts)
    $currentKeys = @($currentPorts | ForEach-Object { Get-PortKey $_ })

    if ($applicationKey -notin $currentKeys) {
        $applicationDisappeared = $true
    }

    if ($applicationDisappeared) {
        $targetCandidates = @($currentPorts | Where-Object {
            (Get-PortKey $_) -notin $otherInitialKeys
        })

        if ($targetCandidates.Count -eq 1) {
            $bootloader = $targetCandidates[0]
            break
        }
        elseif ($targetCandidates.Count -gt 1) {
            Write-Host "Ambiguous ports after reset:"
            Show-Ports $targetCandidates
            throw "Refusing to guess which port is the bootloader."
        }
    }

    Start-Sleep -Milliseconds 75
}

if (-not $bootloader) {
    throw "No Pro Micro USB re-enumeration was detected before timeout."
}

Write-Host "Bootloader interface: $($bootloader.DeviceID)"
Start-Sleep -Milliseconds $PortStabilizationMs

$stableBootloader = @(Get-ProMicroPorts) |
    Where-Object { $_.DeviceID -eq $bootloader.DeviceID } |
    Select-Object -First 1
if (-not $stableBootloader) {
    throw "Bootloader port disappeared before it became ready. Try again."
}

Push-Location $projectRoot
try {
    & $avrdude -p atmega32u4 -C $avrdudeConfig -c avr109 -b 57600 `
        -P $bootloader.DeviceID -U "flash:w:${firmwareRelative}:i"
    if ($LASTEXITCODE -ne 0) {
        throw "avrdude upload failed with exit code $LASTEXITCODE"
    }
}
finally {
    Pop-Location
}

Write-Host "Upload and flash verification completed successfully."
