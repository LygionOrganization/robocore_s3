$ErrorActionPreference = 'Stop'
$packageRoot = $PSScriptRoot
$manifest = Get-Content -LiteralPath (Join-Path $packageRoot 'manifest.json') -Raw | ConvertFrom-Json
foreach ($entry in $manifest.files.PSObject.Properties) {
    $file = Join-Path $packageRoot $entry.Name
    if (!(Test-Path -LiteralPath $file -PathType Leaf)) { throw "Missing file: $($entry.Name)" }
    $hash = (Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($hash -ne $entry.Value.sha256 -or (Get-Item -LiteralPath $file).Length -ne $entry.Value.bytes) {
        throw "Package verification failed: $($entry.Name)"
    }
}
Write-Output 'PASS: all package files match manifest.json. No hardware was accessed.'
