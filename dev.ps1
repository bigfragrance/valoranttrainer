param(
    [ValidateSet('status', 'test', 'build-capture', 'capture', 'decompile', 'prepare-maps', 'map-test', 'scene-test', 'duel1-test', 'play', 'play-local')]
    [string]$Action = 'status',
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$Rvas
)
$ErrorActionPreference = 'Stop'
$taskRuntimeRoot = Join-Path $PSScriptRoot '.local\runtime'
$taskEnvironment = @{
    TEMP = (Join-Path $taskRuntimeRoot 'temp')
    TMP = (Join-Path $taskRuntimeRoot 'temp')
    LOCALAPPDATA = (Join-Path $taskRuntimeRoot 'LocalAppData')
    APPDATA = (Join-Path $taskRuntimeRoot 'Roaming')
    'UE-LocalDataCachePath' = (Join-Path $taskRuntimeRoot 'DerivedDataCache')
    PYTHONPYCACHEPREFIX = (Join-Path $taskRuntimeRoot 'pycache')
    PYTHONUTF8 = '1'
}
$taskPreviousEnvironment = @{}
foreach ($taskKey in $taskEnvironment.Keys) {
    $taskPreviousEnvironment[$taskKey] = [Environment]::GetEnvironmentVariable($taskKey, 'Process')
    if ($taskKey -ne 'PYTHONUTF8') {
        [void][IO.Directory]::CreateDirectory($taskEnvironment[$taskKey])
    }
    [Environment]::SetEnvironmentVariable($taskKey, $taskEnvironment[$taskKey], 'Process')
}
Push-Location -LiteralPath $PSScriptRoot
try {
    switch ($Action) {
        'status' { & python tools/project_status.py }
        'test' { & python -m unittest discover -s tests -v }
        'play' { & python tools/training_server.py --open --start }
        'play-local' { & python tools/start_local_server_console.py --open --start }
        'build-capture' { & python tools/run_native_probe.py --build }
        'capture' { & python tools/run_native_probe.py --build --capture }
        'prepare-maps' { & python tools/run_map_probe.py --all }
        'duel1-test' {
            if ($Rvas) { throw 'duel1-test is fixed to Duel 1 and accepts no map argument' }
            & python tools/run_map_probe.py --map 1 --mode navigation --run --seconds 90
        }
        { $_ -in 'map-test', 'scene-test' } {
            $mapNumber = if ($Rvas) { $Rvas[0] } else { '1' }
            if ($Rvas.Count -gt 1 -or $mapNumber -notmatch '^[1-5]$') { throw 'Supply one map number from 1 to 5' }
            $probeMode = if ($Action -eq 'scene-test') { 'scene' } else { 'original' }
            & python tools/run_map_probe.py --map $mapNumber --mode $probeMode --run
        }
        'decompile' {
            if (-not $Rvas) { throw 'Supply exact entry RVAs, e.g. .\dev.ps1 decompile 0x19e4060' }
            & (Join-Path $PSScriptRoot 'work/RunTargeted.ps1') @Rvas
        }
    }
    if ($LASTEXITCODE -ne 0) { throw "Command failed (exit $LASTEXITCODE). Inspect its recorded result." }
}
finally {
    Pop-Location
    foreach ($taskKey in $taskPreviousEnvironment.Keys) {
        [Environment]::SetEnvironmentVariable($taskKey, $taskPreviousEnvironment[$taskKey], 'Process')
    }
}

