$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$roots = @('Core', 'docs', 'tests', 'tools')
$extensions = @('.c', '.h', '.md', '.ps1')
$strictUtf8 = New-Object System.Text.UTF8Encoding($false, $true)
$checked = 0
$failed = 0

foreach ($relativeRoot in $roots)
{
    $root = Join-Path $repoRoot $relativeRoot
    if (-not (Test-Path -LiteralPath $root))
    {
        continue
    }

    $files = Get-ChildItem -LiteralPath $root -Recurse -File |
        Where-Object {
            ($extensions -contains $_.Extension.ToLowerInvariant()) -and
            ($_.FullName -notmatch '[\\/]tests[\\/]native[\\/]build[\\/]')
        }

    foreach ($file in $files)
    {
        try
        {
            $bytes = [System.IO.File]::ReadAllBytes($file.FullName)
            $text = $strictUtf8.GetString($bytes)
            if ($text.Contains([char]0xFFFD))
            {
                throw 'Unicode replacement character found'
            }
            $checked++
        }
        catch
        {
            Write-Host "Invalid UTF-8 file: $($file.FullName)" -ForegroundColor Red
            $failed++
        }
    }
}

if ($failed -ne 0)
{
    throw "UTF-8 validation failed for $failed files"
}

Write-Host "UTF-8 validation passed: $checked files"
