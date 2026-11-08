# Verifies that a VST3 payload DLL only depends on system DLLs.
# Fails on any third-party DLL (a missing runtime on end-user machines).
#
# Usage: pwsh tools/check_dll_dependencies.ps1 -DllPath <path-to-payload.dll>
param(
    [Parameter(Mandatory = $true)][string]$DllPath
)

$ErrorActionPreference = 'Stop'

if (-not (Test-Path $DllPath)) { throw "DLL not found: $DllPath" }

$dumpbin = & vswhere -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find VC\Tools\MSVC\*\bin\Hostx64\x64\dumpbin.exe | Select-Object -First 1
if (-not $dumpbin) { throw "dumpbin.exe not found (no MSVC toolset on this machine)" }

$deps = & $dumpbin /dependents $DllPath | Select-String -Pattern '\.dll$' | ForEach-Object { $_.Line.Trim() }
if (-not $deps) { throw "dumpbin reported no dependencies" }

# System DLLs a JUCE VST3 may legitimately import. Anything else means the
# artifact would fail to load on a clean Windows machine.
$allowed = @(
    'kernel32.dll', 'user32.dll', 'gdi32.dll', 'advapi32.dll', 'ole32.dll',
    'oleaut32.dll', 'shell32.dll', 'shlwapi.dll', 'combase.dll', 'rpcrt4.dll',
    'ucrtbase.dll', 'winmm.dll', 'ws2_32.dll', 'dbghelp.dll', 'version.dll',
    'vcruntime140.dll', 'vcruntime140_1.dll', 'msvcp140.dll', 'msvcp140_1.dll',
    'msvcp140_2.dll', 'concrt140.dll'
)

$unexpected = @()
foreach ($d in $deps) {
    $name = $d.ToLowerInvariant()
    $ok = $allowed -contains $name
    if (-not $ok -and $name -like 'api-ms-win-*') { $ok = $true }
    if (-not $ok -and $name -like 'ext-ms-*') { $ok = $true }
    if (-not $ok) { $unexpected += $d }
}

Write-Host "Dependencies:"
$deps | ForEach-Object { Write-Host "  $_" }

if ($unexpected.Count -gt 0) {
    Write-Host "FAIL: non-system dependencies found (plugin would not load on a clean Windows machine):"
    $unexpected | ForEach-Object { Write-Host "  $_" }
    exit 1
}

Write-Host "RUNTIME DEPENDENCIES OK (system DLLs only; VC++ Redistributable required on end-user machines, see docs/LICENSING.md)"
