param([Parameter(Mandatory=$true)][string]$Config, [switch]$NoDialog)
$ErrorActionPreference = 'Stop'
$settings = Get-Content -LiteralPath $Config -Raw -Encoding UTF8 | ConvertFrom-Json
$replaced = $false
$child = $null
function File-Hash([string]$Path) {
    $stream = [System.IO.File]::OpenRead($Path)
    $algorithm = [System.Security.Cryptography.SHA256]::Create()
    try { return [BitConverter]::ToString($algorithm.ComputeHash($stream)).Replace('-', '').ToLowerInvariant() }
    finally { $stream.Dispose(); $algorithm.Dispose() }
}
function Save-Result([string]$Status, [string]$Detail) {
    @{status=$Status; detail=$Detail; child_pid=$(if ($child) { $child.Id } else { $null })} | ConvertTo-Json | Set-Content -LiteralPath $settings.result -Encoding UTF8
}
try {
    $parent = Get-Process -Id $settings.parent_pid -ErrorAction SilentlyContinue
    if ($parent) {
        if (-not $parent.WaitForExit(120000)) { throw 'Original installer did not exit within 120 seconds' }
    }
    if ((File-Hash $settings.candidate) -ne $settings.sha256) {
        throw 'Candidate checksum changed'
    }
    if ((File-Hash $settings.executable) -ne $settings.old_sha256) {
        throw 'Original installer changed during update'
    }
    [System.IO.File]::Replace($settings.candidate, $settings.executable, $settings.backup)
    $replaced = $true
    $arguments = '--resume "' + $settings.target + '" --self-update-ack "' + $settings.ack + '"'
    $child = Start-Process -FilePath $settings.executable -ArgumentList $arguments -PassThru
    $deadline = [DateTime]::UtcNow.AddSeconds(90)
    while ([DateTime]::UtcNow -lt $deadline) {
        if (Test-Path -LiteralPath $settings.ack) {
            $ack = Get-Content -LiteralPath $settings.ack -Raw | ConvertFrom-Json
            if ($ack.sha256 -ne $settings.sha256) { throw 'Replacement acknowledgment does not match' }
            Save-Result 'complete' 'New installer opened successfully. Previous installer retained in previous.exe.'
            exit 0
        }
        if ($child.HasExited) { throw 'New installer exited before opening its window' }
        Start-Sleep -Milliseconds 200
    }
    throw 'New installer did not acknowledge startup'
} catch {
    $detail = $_.Exception.Message
    if ($replaced) {
        try {
            if ($child -and -not $child.HasExited) {
                Stop-Process -Id $child.Id
                $child.WaitForExit(10000) | Out-Null
            }
            # The bootstrap may still be finishing after the UI closes.
            for ($attempt=0; $attempt -lt 50; $attempt++) {
                try {
                    [System.IO.File]::Replace($settings.backup, $settings.executable, $settings.candidate)
                    $replaced = $false
                    break
                } catch { Start-Sleep -Milliseconds 200 }
            }
            if ($replaced) { throw 'Could not restore previous installer. Backup retained at ' + $settings.backup }
            $detail += ' Previous installer restored.'
        } catch { $detail += ' ' + $_.Exception.Message }
    }
    Save-Result 'failed' $detail
    if (-not $NoDialog) {
        Add-Type -AssemblyName PresentationFramework
        [System.Windows.MessageBox]::Show($detail, '安装器自更新失败') | Out-Null
    }
    exit 1
}
