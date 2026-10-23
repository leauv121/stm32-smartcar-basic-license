$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$appSources = Get-ChildItem -LiteralPath (Join-Path $repoRoot 'Core\Src\app') -Filter '*.c'
$violations = @()

foreach ($file in $appSources)
{
    if ($file.Name -eq 'app_runtime.c')
    {
        continue
    }
    $matches = Select-String -LiteralPath $file.FullName -Pattern 'HAL_|TB6612_|HAL_GPIO_'
    if ($matches)
    {
        $violations += $matches
    }
}

$motorCallers = rg -l 'TB6612_SetSpeed' (Join-Path $repoRoot 'Core\Src')
foreach ($caller in $motorCallers)
{
    $name = [System.IO.Path]::GetFileName($caller)
    if (($name -ne 'motion_control.c') -and ($name -ne 'car_control.c') -and
        ($name -ne 'tb6612.c'))
    {
        $violations += "Forbidden direct motor call: $caller"
    }
}

$compatibilityControl = Join-Path $repoRoot 'Core\Src\car_control.c'
if (-not (Select-String -LiteralPath $compatibilityControl -Pattern 'SafetyService_Resolve'))
{
    $violations += 'car_control.c must pass requests through SafetyService_Resolve'
}

if ($violations.Count -ne 0)
{
    $violations | ForEach-Object { Write-Host $_ }
    throw "Source boundary validation failed: $($violations.Count) item(s)"
}

Write-Host 'Source boundary validation passed'
