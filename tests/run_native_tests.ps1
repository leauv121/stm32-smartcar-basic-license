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
    'test_command_parser.c' = @('Core\Src\app\command_parser.c')
    'test_car_control_compat.c' = @('Core\Src\control\line_follow_control.c')
    'test_normal_mission.c' = @('Core\Src\app\normal_mission.c')
    'test_advanced_mission.c' = @('Core\Src\app\advanced_mission.c')
    'test_route_detector.c' = @('Core\Src\app\route_detector.c')
    'test_app_controller.c' = @(
        'Core\Src\app\app_controller.c',
        'Core\Src\app\normal_mission.c',
        'Core\Src\app\advanced_mission.c',
        'Core\Src\services\error_manager.c'
    )
    'test_self_test.c' = @(
        'Core\Src\services\self_test.c',
        'Core\Src\services\error_manager.c'
    )
    'test_driver_algorithms.c' = @('Core\Src\services\driver_algorithms.c')
    'test_display_service.c' = @('Core\Src\services\display_service.c')
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
