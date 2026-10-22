$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$nativeRoot = Join-Path $PSScriptRoot 'native'
$buildRoot = Join-Path $nativeRoot 'build'
$gcc = (Get-Command gcc -ErrorAction Stop).Source

New-Item -ItemType Directory -Force -Path $buildRoot | Out-Null

$sourceMap = @{
    'test_smoke.c' = @()
    'test_error_manager.c' = @('Core\Src\services\error_manager.c')
    'test_safety_service.c' = @(
        'Core\Src\services\error_manager.c',
        'Core\Src\services\safety_service.c'
    )
    'test_indicator_service.c' = @('Core\Src\services\indicator_service.c')
    'test_diagnostic_view.c' = @('Core\Src\services\diagnostic_view.c')
    'test_scheduler.c' = @('Core\Src\services\scheduler.c')
    'test_sensor_service.c' = @(
        'Core\Src\services\error_manager.c',
        'Core\Src\services\sensor_service.c'
    )
    'test_line_follow_control.c' = @('Core\Src\control\line_follow_control.c')
    'test_turn_control.c' = @('Core\Src\control\turn_control.c')
}

$tests = Get-ChildItem -LiteralPath $nativeRoot -Filter 'test_*.c' | Sort-Object Name
$failed = 0

foreach ($test in $tests)
{
    $output = Join-Path $buildRoot ($test.BaseName + '.exe')
    $arguments = @(
        '-std=c99',
        '-Wall',
        '-Wextra',
        '-Werror',
        '-I', (Join-Path $repoRoot 'Core\Inc'),
        '-I', $nativeRoot,
        $test.FullName
    )

    foreach ($relativeSource in $sourceMap[$test.Name])
    {
        $arguments += Join-Path $repoRoot $relativeSource
    }

    $arguments += @('-o', $output)

    Write-Host "[BUILD] $($test.Name)"
    & $gcc @arguments
    if ($LASTEXITCODE -ne 0)
    {
        throw "Test compilation failed: $($test.Name)"
    }

    Write-Host "[RUN] $($test.Name)"
    & $output
    if ($LASTEXITCODE -ne 0)
    {
        $failed++
    }
}

if ($failed -ne 0)
{
    throw "$failed test executables failed"
}

Write-Host "All native tests passed: $($tests.Count)"
