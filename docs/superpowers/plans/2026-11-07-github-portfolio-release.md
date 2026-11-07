# GitHub Portfolio Release Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Publish the current STM32 smart-car project as a truthful, recruiter-friendly public GitHub repository named `leauv121/stm32-smartcar-basic-license`.

**Architecture:** Keep the complete Git history and embedded project intact, add one curated portfolio-release commit, and publish the current branch as GitHub `main`. The latest tree removes internal agent workflow files, while retaining source, build projects, tests, experiment records, and learning notes. The README leads with verified normal-license results and places unverified advanced-mode code in an explicitly experimental section.

**Tech Stack:** STM32F103C8T6, C99, STM32 HAL/CMSIS, Keil MDK ArmCC 5.06, STM32CubeMX, PowerShell, MinGW GCC host tests, Git, GitHub CLI.

**Spec:** `docs/changes/2026-11-07-github-portfolio-release.md`

## Global Constraints

- Preserve the complete existing Git history; do not rebase, squash, filter, or force-push.
- Do not change control algorithms, hardware mappings, or tuned parameters during this release.
- Present the normal/basic license task as the completed focus; label advanced-route code as an unverified extension framework.
- Distinguish Keil compilation, host tests, firmware download, and physical-car verification.
- Preserve the two known `test_line_follow_control.c` expectation failures and do not claim that the full host suite passes.
- Keep the existing local `origin`; add GitHub as remote `github`.
- Publish as public repository `leauv121/stm32-smartcar-basic-license` with default branch `main`.
- License original project code under MIT; retain and acknowledge third-party licenses under `Drivers/`.

---

### Task 1: Curate the public project tree and recruiter-facing documentation

**Files:**
- Rewrite: `README.md`
- Create: `LICENSE`
- Create: `THIRD_PARTY_NOTICES.md`
- Modify: `docs/changes/README.md`
- Modify: `docs/changes/2026-10-22-final-work-framework.md`
- Modify: `docs/changes/2026-10-25-final-tracking-migration.md`
- Modify: `docs/changes/2026-10-26-baseline-line-follow-migration.md`
- Delete: `.cursor/rules/change-documentation.mdc`
- Delete: `AGENTS.md`
- Delete: `docs/superpowers/`
- Test: repository content checks executed from PowerShell

**Interfaces:**
- Consumes: the approved release design, current source tree, `docs/changes/`, `学习资料/`, and existing Git history.
- Produces: a self-contained public tree with a recruiter-first README, portable documentation links, and explicit licensing.

- [ ] **Step 1: Run publication checks that must fail on the current tree**

Run:

```powershell
$repo = 'D:\dev\final-work'
$failures = @()
if (Test-Path "$repo\.cursor") { $failures += '.cursor exists' }
if (Test-Path "$repo\AGENTS.md") { $failures += 'AGENTS.md exists' }
if (Test-Path "$repo\docs\superpowers") { $failures += 'docs/superpowers exists' }
if (Select-String -Path "$repo\README.md" -Pattern 'D:\\dev|codex/final-work-framework') {
    $failures += 'README contains local workflow references'
}
if (-not (Test-Path "$repo\LICENSE")) { $failures += 'LICENSE missing' }
if (-not (Test-Path "$repo\THIRD_PARTY_NOTICES.md")) { $failures += 'third-party notice missing' }
$failures
if ($failures.Count -eq 0) { throw 'Expected pre-publication checks to fail' }
```

Expected: at least the three internal-file checks, README local-reference check, license check, and third-party notice check report failures.

- [ ] **Step 2: Rewrite `README.md` with the approved information hierarchy**

Use these exact top-level headings in order:

```markdown
# STM32 智能小车：初级驾照任务系统

## 项目概览
## 简历项目亮点
## 初级驾照任务流程
## 系统架构
## 硬件与引脚
## 混合四探头循迹
## 状态机与安全策略
## 调试方法与工程经验
## 构建、烧录与运行
## 验证状态
## 已知限制
## 目录结构
## 深入阅读
## 简历描述示例
## License
```

The README content must include these concrete facts:

- MCU and toolchain: STM32F103C8T6, HAL, CubeMX, Keil MDK, ArmCC 5.06.
- Normal task flow: start indication, outbound line following, red stop/green release, ultrasonic obstacle stop, yaw-assisted U-turn, low-speed reacquisition, return tracking, terminal stop.
- Line sensors: PA6 outer digital, PA7 inner analog, PB0 inner analog, PB1 outer digital; ADC uses DMA.
- Control behavior: weighted four-probe position, PID correction, outer-probe direction assistance, reacquisition direction latch, stable-center handoff, output limiting, and soft start.
- Engineering services: non-blocking scheduling, OLED diagnostics, USART3 commands and telemetry, centralized error management, safety arbitration.
- Verified statement: latest Keil rebuild produces `0 Error(s), 0 Warning(s)` after Task 2 reruns it.
- Host-test statement: most native executables pass, but two assertions in `test_line_follow_control.c` remain inconsistent with the final tuned parameters by explicit project decision.
- Advanced-mode statement: route framework remains in the repository but is not presented as a completed physical-car result.
- Resume example: a three-bullet Chinese description covering architecture, control algorithm, and debugging/verification without claiming unverified achievements.
- Links to `docs/changes/README.md`, `学习资料/课内/PID模拟量循迹与姿态计圈/README.md`, `MDK-ARM/timer_pwm.uvprojx`, and `timer_pwm.ioc`.

Do not include local absolute paths, the branch name `codex/final-work-framework`, AI/agent workflow details, fabricated performance figures, badges without CI, or claims that all tests pass.

- [ ] **Step 3: Add MIT license and third-party notice**

Create `LICENSE` with:

```text
MIT License

Copyright (c) 2026 leauv121

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

Create `THIRD_PARTY_NOTICES.md` stating that the MIT license covers original project code and documentation only, while STM32 HAL, CMSIS, MPU6050/DMP, and other third-party components retain the license notices in their source directories/files. Link to `Drivers/STM32F1xx_HAL_Driver/LICENSE.txt` and `Drivers/CMSIS/LICENSE.txt` when those paths exist; otherwise link to the nearest tracked license file found by `rg --files Drivers | rg 'LICENSE'`.

- [ ] **Step 4: Remove internal workflow artifacts from the latest tree**

Run from `D:\dev\final-work`:

```powershell
git rm -r -- .cursor docs/superpowers
git rm -- AGENTS.md
```

This deliberately removes this implementation plan from the latest public tree after it has been loaded and executed. Do not rewrite history.

- [ ] **Step 5: Replace current-tree local paths with portable references**

Change only the current files identified by:

```powershell
rg -n 'D:\\dev|C:\\Users|codex/final-work-framework|D:\\Keil_v5' README.md docs/changes 学习资料 -g '*.md'
```

Required replacements:

- Replace project root paths with `仓库根目录` or a relative Markdown link.
- Replace the old source repository path with `旧版基线工程`.
- Replace the branch name with `开发分支` only where the historical fact still matters; otherwise remove it.
- Replace machine-specific Keil commands with the portable GUI instruction `打开 MDK-ARM/timer_pwm.uvprojx，选择 timer_pwm 并执行 Rebuild`.
- Replace the machine-specific MinGW path with `将 MinGW GCC 加入 PATH`.

- [ ] **Step 6: Update the change-record index**

Append this item to `docs/changes/README.md`:

```markdown
17. [2026-11-07：GitHub 简历作品集发布设计](2026-11-07-github-portfolio-release.md)
```

- [ ] **Step 7: Run the publication checks and confirm they pass**

Run:

```powershell
$repo = 'D:\dev\final-work'
if (Test-Path "$repo\.cursor") { throw '.cursor still exists' }
if (Test-Path "$repo\AGENTS.md") { throw 'AGENTS.md still exists' }
if (Test-Path "$repo\docs\superpowers") { throw 'docs/superpowers still exists' }
if (-not (Test-Path "$repo\LICENSE")) { throw 'LICENSE missing' }
if (-not (Test-Path "$repo\THIRD_PARTY_NOTICES.md")) { throw 'third-party notice missing' }
$bad = rg -n 'D:\\dev|C:\\Users|codex/final-work-framework|D:\\Keil_v5' `
    "$repo\README.md" "$repo\docs\changes" "$repo\学习资料" -g '*.md'
if ($LASTEXITCODE -eq 0) { $bad; throw 'machine-specific references remain' }
git -C $repo diff --check
if ($LASTEXITCODE -ne 0) { throw 'git diff check failed' }
```

Expected: no exception and `git diff --check` produces no errors.

- [ ] **Step 8: Commit the curated public tree**

Run:

```powershell
git add -A
git commit -m "docs: prepare public portfolio release"
```

Expected: one commit containing README, licensing, documentation portability fixes, and removal of internal workflow files; no firmware behavior changes.

---

### Task 2: Verify the release candidate and record exact evidence

**Files:**
- Modify only if evidence differs: `README.md`
- Test: `tests/run_native_tests.ps1`
- Test: `tests/check_source_boundaries.ps1`
- Test: `tools/check_utf8.ps1`
- Build: `MDK-ARM/timer_pwm.uvprojx`

**Interfaces:**
- Consumes: the curated tree from Task 1.
- Produces: fresh compilation, test, hygiene, and secret-scan evidence for the exact commit to publish.

- [ ] **Step 1: Run a fresh Keil rebuild**

Run:

```powershell
$args = @('-b', 'D:\dev\final-work\MDK-ARM\timer_pwm.uvprojx',
          '-t', 'timer_pwm', '-o', 'D:\dev\final-work\MDK-ARM\portfolio-rebuild.log')
$p = Start-Process -FilePath 'D:\Keil_v5\UV4\UV4.exe' `
    -ArgumentList $args -Wait -PassThru -WindowStyle Hidden
Get-Content 'D:\dev\final-work\MDK-ARM\portfolio-rebuild.log' -Encoding Default
if ($p.ExitCode -ne 0) { throw "Keil rebuild failed: $($p.ExitCode)" }
```

Expected: `timer_pwm.axf` reports `0 Error(s), 0 Warning(s)`.

- [ ] **Step 2: Run the native suite and capture the known result**

Run:

```powershell
$env:Path = 'C:\Users\OOO\tools\mingw-16.1.0\mingw64\bin;' + $env:Path
powershell -ExecutionPolicy Bypass -File tests\run_native_tests.ps1
```

Expected: all executables other than `test_line_follow_control.c` pass; that executable reports exactly two failures at the assertions equivalent to `outer_output.pid_output <= 1500` and expected wheel-target sum `10000` versus actual `8500`. Any additional failure blocks publication.

- [ ] **Step 3: Run source-boundary and UTF-8 checks independently**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File tests\check_source_boundaries.ps1
if ($LASTEXITCODE -ne 0) { throw 'source-boundary check failed' }
powershell -ExecutionPolicy Bypass -File tools\check_utf8.ps1
if ($LASTEXITCODE -ne 0) { throw 'UTF-8 check failed' }
```

Expected: both commands exit successfully.

- [ ] **Step 4: Scan the tracked release tree for secrets and machine-specific references**

Run:

```powershell
$tracked = git ls-files
$secretPatterns = 'gho_[A-Za-z0-9]+|ghp_[A-Za-z0-9]+|github_pat_[A-Za-z0-9_]+|AKIA[0-9A-Z]{16}|BEGIN (RSA |OPENSSH )?PRIVATE KEY|password\s*[:=]|token\s*[:=]'
$secretHits = $tracked | ForEach-Object {
    if (Test-Path -LiteralPath $_ -PathType Leaf) {
        Select-String -LiteralPath $_ -Pattern $secretPatterns -AllMatches -ErrorAction SilentlyContinue
    }
}
if ($secretHits) { $secretHits; throw 'possible secret found in tracked files' }
$localHits = rg -n 'D:\\dev|C:\\Users|codex/final-work-framework|D:\\Keil_v5' `
    README.md docs/changes 学习资料 -g '*.md'
if ($LASTEXITCODE -eq 0) { $localHits; throw 'local path reference remains' }
```

Expected: no secret or local-path hits.

- [ ] **Step 5: Remove the temporary build log and verify a clean tree**

Run:

```powershell
Remove-Item -LiteralPath 'MDK-ARM\portfolio-rebuild.log' -Force -ErrorAction SilentlyContinue
git status --short --branch
```

Expected: no untracked build log and no source/document changes. If the README evidence required correction, commit only that correction as `docs: align portfolio verification status` and rerun the affected checks.

---

### Task 3: Create and verify the public GitHub repository

**Files:**
- No source files changed.
- Git remote configuration gains `github`.
- GitHub repository metadata gains description and topics.

**Interfaces:**
- Consumes: the verified clean release commit from Task 2 and authenticated GitHub CLI account `leauv121`.
- Produces: public GitHub repository `https://github.com/leauv121/stm32-smartcar-basic-license` with default branch `main`.

- [ ] **Step 1: Confirm authentication and repository absence**

Run:

```powershell
gh auth status
gh repo view leauv121/stm32-smartcar-basic-license --json nameWithOwner,isPrivate,url 2>$null
```

Expected: authentication succeeds as `leauv121`; repository lookup fails because the new name is unused. If the repository already exists, stop and inspect it rather than overwriting it.

- [ ] **Step 2: Create the empty public repository**

Run:

```powershell
gh repo create leauv121/stm32-smartcar-basic-license `
    --public `
    --description 'STM32F103 smart car for a basic driving-license course: hybrid line following, sensor fusion, safety state machine, and field-debugging notes.'
```

Expected: GitHub returns `https://github.com/leauv121/stm32-smartcar-basic-license`.

- [ ] **Step 3: Add the GitHub remote without changing local `origin`**

Run:

```powershell
$url = 'https://github.com/leauv121/stm32-smartcar-basic-license.git'
if (git remote get-url github 2>$null) {
    if ((git remote get-url github) -ne $url) { throw 'remote github points elsewhere' }
} else {
    git remote add github $url
}
git remote -v
```

Expected: `origin` still points to the existing local repository and `github` points to the new GitHub repository.

- [ ] **Step 4: Push the current verified commit as GitHub `main`**

Run:

```powershell
git push -u github HEAD:refs/heads/main
```

Expected: a normal non-force push creates `main`.

- [ ] **Step 5: Set repository metadata**

Run:

```powershell
gh repo edit leauv121/stm32-smartcar-basic-license `
    --default-branch main `
    --description 'STM32F103 smart car for a basic driving-license course: hybrid line following, sensor fusion, safety state machine, and field-debugging notes.'
gh api --method PUT repos/leauv121/stm32-smartcar-basic-license/topics `
    -H 'Accept: application/vnd.github+json' `
    -f 'names[]=stm32' `
    -f 'names[]=stm32f103' `
    -f 'names[]=embedded-systems' `
    -f 'names[]=smart-car' `
    -f 'names[]=pid-control' `
    -f 'names[]=robotics' `
    -f 'names[]=keil' `
    -f 'names[]=hal'
```

Expected: default branch is `main`, description matches, and all eight topics are present.

- [ ] **Step 6: Synchronize the release commit to the existing local remote**

Run:

```powershell
git push origin HEAD:refs/heads/main HEAD:refs/heads/codex/final-work-framework
```

Expected: both existing local remote branches fast-forward; do not use force.

- [ ] **Step 7: Verify the public result from GitHub**

Run:

```powershell
gh repo view leauv121/stm32-smartcar-basic-license `
    --json nameWithOwner,isPrivate,url,defaultBranchRef,description,repositoryTopics,licenseInfo
gh api repos/leauv121/stm32-smartcar-basic-license/readme --jq '.html_url'
git ls-remote github refs/heads/main
git rev-parse HEAD
git status --short --branch
```

Expected:

- `isPrivate` is `false`.
- URL is `https://github.com/leauv121/stm32-smartcar-basic-license`.
- default branch is `main`.
- MIT license is detected.
- README endpoint returns successfully.
- remote `main` commit equals local `HEAD`.
- worktree is clean.
