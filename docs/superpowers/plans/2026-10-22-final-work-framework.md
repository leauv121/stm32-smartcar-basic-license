# 自动驾驶小车最终工程框架实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 在独立分支中建立可编译、可诊断、可扩展的最终小车工程框架，并安全接入现有可复用代码。

**Architecture:** 保留 CubeMX/HAL 生成层，将纯逻辑拆到配置、控制、服务和应用模块。纯逻辑使用 Windows GCC 做测试驱动开发，硬件适配通过 Keil Rebuild 验证；集中故障管理负责 OLED、串口、LED 和安全停车。

**Tech Stack:** STM32F103C8T6、STM32 HAL、Keil ARM Compiler 5、C99、PowerShell、Windows GCC、OLED SSD1306、MPU6050 DMP、TCS34725、HC-SR04、TB6612。

**Spec:** `docs/superpowers/specs/2026-10-22-final-work-architecture-design.md`

## Global Constraints

- 所有新增或修改的 C、H、Markdown 文件使用 UTF-8，禁止出现 Unicode 替换字符。
- CubeMX 生成文件只在 `USER CODE` 区域内手工修改。
- 不使用动态内存。
- 中断只写数据或标志，不调用 OLED、串口、复杂 I2C 或任务状态机。
- 所有电机输出必须经过 `MotionControl_Apply()` 和 `SafetyService_Resolve()`。
- 严重故障必须覆盖任务运动请求为安全停车，恢复后不得自动重启。
- 当前 `origin` 指向原本地工程；实施期间不得执行 `git push`，除非用户明确授权并重新配置远端。
- 每个任务完成后执行本任务测试、全部主机测试、`git diff --check`，再提交单一主题 commit。
- 最终必须执行 Keil Rebuild；编译、烧录和实车验证分别记录。

---

### Task 1: 建立目录边界、编码检查与主机测试工具

**Files:**
- Create: `Core/Inc/config/app_config.h`
- Create: `Core/Inc/config/car_parameters.h`
- Create: `Core/Inc/config/hardware_map.h`
- Create: `tests/native/test_assert.h`
- Create: `tests/native/test_smoke.c`
- Create: `tests/run_native_tests.ps1`
- Create: `tools/check_utf8.ps1`
- Modify: `.gitignore`
- Create: `docs/changes/2026-10-22-final-work-framework.md`

**Interfaces:**
- Produces: `APP_FEATURE_*` compile-time switches, parameter constants, `TEST_ASSERT_*` macros, repeatable native test and encoding commands.
- Consumes: existing STM32 pin definitions from `Core/Inc/main.h`.

- [ ] **Step 1: Write the failing smoke test**

Create `tests/native/test_smoke.c` so compilation fails because `app_config.h` does not yet exist:

```c
#include "config/app_config.h"
#include "test_assert.h"

int main(void)
{
    TEST_ASSERT_EQ_INT(1, APP_FEATURE_ERROR_MANAGER);
    TEST_ASSERT_EQ_INT(1, APP_FEATURE_DIAGNOSTIC_DISPLAY);
    return test_failures;
}
```

- [ ] **Step 2: Create the test assertion header and runner, then verify RED**

`test_assert.h` defines `test_failures`, `TEST_ASSERT_TRUE` and `TEST_ASSERT_EQ_INT`. `tests/run_native_tests.ps1` locates GCC, compiles every `test_*.c` with `-std=c99 -Wall -Wextra -Werror`, and runs each executable.

Run:

```powershell
powershell -ExecutionPolicy Bypass -File tests/run_native_tests.ps1
```

Expected: compile failure stating `config/app_config.h` is missing.

- [ ] **Step 3: Add minimal configuration headers**

`app_config.h` must define integer switches for error manager, diagnostic display, speed measurement, color, ultrasonic, battery, normal mission and advanced mission. Keep hardware-dependent features enabled in the build but runtime-gated by self-test.

`car_parameters.h` must centralize existing 20 ms control period, 200 ms OLED period, 200 ms UART period, PWM maximum 7199, current PID values and diagnostic rotation period 2000 ms.

`hardware_map.h` must include `main.h`, `adc.h`, `i2c.h`, `tim.h` and `usart.h`, without redefining CubeMX pins.

- [ ] **Step 4: Add UTF-8 validation**

`tools/check_utf8.ps1` recursively checks `Core`, `docs`, `tests` and `tools`; it must decode files with a strict UTF-8 decoder and fail when the text contains character `U+FFFD`.

Run:

```powershell
powershell -ExecutionPolicy Bypass -File tools/check_utf8.ps1
```

Expected: PASS with a checked-file count.

- [ ] **Step 5: Verify GREEN and commit**

Run native tests, UTF-8 check and `git diff --check`. Expected: all exit 0.

Commit:

```bash
git add .gitignore Core/Inc/config tests tools docs/changes/2026-10-22-final-work-framework.md
git commit -m "test: add host harness and project configuration"
```

---

### Task 2: 实现集中故障管理

**Files:**
- Create: `Core/Inc/services/error_manager.h`
- Create: `Core/Src/services/error_manager.c`
- Create: `tests/native/test_error_manager.c`
- Modify: `tests/run_native_tests.ps1`

**Interfaces:**
- Produces:
  - `void ErrorManager_Init(void)`
  - `void ErrorManager_Report(ErrorModule module, ErrorCode code, ErrorLevel level, uint32_t now_ms, uint8_t auto_recover)`
  - `void ErrorManager_Clear(ErrorModule module, ErrorCode code, uint32_t now_ms)`
  - `uint8_t ErrorManager_GetActiveCount(void)`
  - `uint8_t ErrorManager_HasFatal(void)`
  - `const ErrorRecord *ErrorManager_GetHighest(void)`
  - `const ErrorRecord *ErrorManager_GetDisplayRecord(uint32_t now_ms)`
- Consumes: only `<stdint.h>`; no HAL dependency.

- [ ] **Step 1: Write failing tests for registration and deduplication**

Test that reporting the same module and code twice creates one active record, keeps the first timestamp, updates the latest timestamp, and increments the occurrence count.

- [ ] **Step 2: Verify RED**

Run only `test_error_manager.c`. Expected: missing header or unresolved functions.

- [ ] **Step 3: Implement the fixed-size error table**

Use 16 static records. Define all module identifiers from the spec and generic codes including `NOT_FOUND`, `INIT_FAILED`, `DATA_TIMEOUT`, `VALUE_INVALID`, `COMMUNICATION`, `LOW_VOLTAGE`, `STATE_INVALID` and `RESOURCE_CONFLICT`.

- [ ] **Step 4: Add failing tests for priority, rotation and recovery**

Cover:

```c
TEST_ASSERT_EQ_INT(ERROR_LEVEL_FATAL, ErrorManager_GetHighest()->level);
TEST_ASSERT_TRUE(ErrorManager_GetDisplayRecord(0U) != NULL);
TEST_ASSERT_TRUE(ErrorManager_GetDisplayRecord(2000U) != NULL);
TEST_ASSERT_EQ_INT(0, ErrorManager_HasFatal());
```

The final assertion must fail until fatal clearing is implemented.

- [ ] **Step 5: Implement priority and rotation**

Order records by severity, then first occurrence. Rotate equal-severity records every 2000 ms. Clearing deactivates a matching record but retains timestamps for diagnostics.

- [ ] **Step 6: Verify and commit**

Run all native tests, UTF-8 check and `git diff --check`.

Commit:

```bash
git add Core/Inc/services/error_manager.h Core/Src/services/error_manager.c tests
git commit -m "feat: add centralized error manager"
```

---

### Task 3: 实现安全仲裁、灯光模式和诊断显示模型

**Files:**
- Create: `Core/Inc/control/motion_types.h`
- Create: `Core/Inc/services/safety_service.h`
- Create: `Core/Src/services/safety_service.c`
- Create: `Core/Inc/services/indicator_service.h`
- Create: `Core/Src/services/indicator_service.c`
- Create: `Core/Inc/services/diagnostic_view.h`
- Create: `Core/Src/services/diagnostic_view.c`
- Create: `tests/native/test_safety_service.c`
- Create: `tests/native/test_indicator_service.c`
- Create: `tests/native/test_diagnostic_view.c`

**Interfaces:**
- Produces:
  - `MotionRequest` and `MotionMode` in `motion_types.h`
  - `MotionRequest SafetyService_Resolve(MotionRequest requested, uint8_t user_stop, uint8_t mission_complete)`
  - `void SafetyService_Acknowledge(void)`
  - `void IndicatorService_Start(IndicatorPattern pattern, uint32_t now_ms)`
  - `IndicatorOutput IndicatorService_Update(uint32_t now_ms)`
  - `void DiagnosticView_Format(const ErrorRecord *record, uint8_t index, uint8_t count, DiagnosticLines *out)`
- Consumes: `ErrorManager_HasFatal()` and error records from Task 2.

- [ ] **Step 1: Write failing safety tests**

Verify a normal request passes through, while fatal error, USER stop or task completion produces `MOTION_STOP`. Verify clearing a fatal error still leaves a latched stop until `SafetyService_Acknowledge()` is called.

- [ ] **Step 2: Verify RED and implement minimal safety arbitration**

No HAL calls are allowed. `MotionRequest` contains signed left and right targets plus a mode enum.

- [ ] **Step 3: Write failing indicator timing tests**

Cover three startup flashes at 500 ms, turn signal toggling at 300 ms, hazard toggling at 500 ms and `INDICATOR_OFF`.

- [ ] **Step 4: Implement indicator state machine**

Return logical left, right and core LED states. GPIO writes remain outside this pure module.

- [ ] **Step 5: Write failing diagnostic formatting tests**

For an MPU timeout, expect four null-terminated lines no longer than 16 characters:

```text
ERROR 01/03
MOD:MPU6050
CODE:DATA TIME
MOTOR STOP
```

- [ ] **Step 6: Implement bounded formatting and verify all tests**

Use fixed buffers and `snprintf`; truncate human-readable names deterministically.

- [ ] **Step 7: Commit**

```bash
git add Core/Inc/services Core/Src/services tests
git commit -m "feat: add safety indicators and diagnostic view"
```

---

### Task 4: 建立调度器、传感器快照和运行时超时检测

**Files:**
- Create: `Core/Inc/services/scheduler.h`
- Create: `Core/Src/services/scheduler.c`
- Create: `Core/Inc/services/sensor_service.h`
- Create: `Core/Src/services/sensor_service.c`
- Create: `tests/native/test_scheduler.c`
- Create: `tests/native/test_sensor_service.c`

**Interfaces:**
- Produces:
  - `SchedulerEvents Scheduler_Update(uint32_t now_ms)`
  - `void SensorService_Init(void)`
  - typed update functions for line ADC, outer line pins, Yaw, wheel speeds, color, distance and battery
  - `const SensorSnapshot *SensorService_GetSnapshot(void)`
  - `void SensorService_CheckTimeouts(uint32_t now_ms, uint8_t mission_requires_yaw)`
- Consumes: `ErrorManager_Report()` and `ErrorManager_Clear()`.

- [ ] **Step 1: Write failing wrap-safe scheduler tests**

Verify 20 ms, 50 ms, 100 ms, 200 ms and 500 ms events, including `uint32_t` tick wraparound.

- [ ] **Step 2: Implement scheduler and verify GREEN**

Each event is edge-triggered once; late calls do not replay a backlog.

- [ ] **Step 3: Write failing sensor validity tests**

Verify value updates set validity and timestamps. Verify required Yaw older than its timeout reports fatal MPU timeout, while disabled color and ultrasonic sources do not report errors.

- [ ] **Step 4: Implement snapshot and timeout checks**

Keep a single static snapshot. Do not call HAL or drivers from this module.

- [ ] **Step 5: Verify and commit**

```bash
git add Core/Inc/services Core/Src/services tests
git commit -m "feat: add scheduler and sensor snapshot service"
```

---

### Task 5: 提取循迹和定角转弯纯控制逻辑

**Files:**
- Create: `Core/Inc/control/line_follow_control.h`
- Create: `Core/Src/control/line_follow_control.c`
- Create: `Core/Inc/control/turn_control.h`
- Create: `Core/Src/control/turn_control.c`
- Create: `Core/Inc/control/motion_control.h`
- Create: `Core/Src/control/motion_control.c`
- Create: `tests/native/test_line_follow_control.c`
- Create: `tests/native/test_turn_control.c`
- Modify: `Core/Src/car_control.c`
- Modify: `Core/Inc/car_control.h`

**Interfaces:**
- Produces:
  - `LineFollowOutput LineFollowControl_Update(LineFollowInput input)`
  - `void LineFollowControl_Reset(void)`
  - `void TurnControl_Start(float current_yaw, float target_deg, TurnDirection direction, uint32_t now_ms)`
  - `TurnResult TurnControl_Update(float current_yaw, uint8_t yaw_valid, uint32_t now_ms)`
  - `void MotionControl_Apply(MotionRequest request)`
- Consumes: current PID constants and existing `TB6612_SetSpeed()`.

- [ ] **Step 1: Write failing line-follow tests**

Cover centered AO values, positive and negative error, outer-right rescue, outer-left rescue, saturation and soft start.

- [ ] **Step 2: Implement pure line-follow controller**

Move the current error, PID, deadband, integral limit, soft start and outer DO logic without changing direction conventions. Return requested left and right targets instead of writing GPIO.

- [ ] **Step 3: Write failing Yaw unwrap and timeout tests**

Cover `179` to `-179`, `-179` to `179`, right 180° completion, wrong-direction noise and timeout.

- [ ] **Step 4: Implement turn controller**

Accumulate signed wrapped deltas. Invalid Yaw during an active turn reports `TURN_SENSOR_ERROR`; elapsed time above the configured limit reports `TURN_TIMEOUT`.

- [ ] **Step 5: Add the sole hardware output adapter**

`MotionControl_Apply()` clamps targets, maps signs to TB6612 direction and applies zero for stop. Keep existing `car_control` functions as compatibility wrappers calling the new interfaces.

- [ ] **Step 6: Verify and commit**

```bash
git add Core/Inc/control Core/Src/control Core/Inc/car_control.h Core/Src/car_control.c tests
git commit -m "refactor: separate line and turn control logic"
```

---

### Task 6: 实现上电自检状态机和硬件适配接口

**Files:**
- Create: `Core/Inc/services/self_test.h`
- Create: `Core/Src/services/self_test.c`
- Create: `Core/Inc/services/hardware_adapter.h`
- Create: `Core/Src/services/hardware_adapter.c`
- Create: `tests/native/test_self_test.c`

**Interfaces:**
- Produces:
  - `void SelfTest_Start(uint32_t now_ms)`
  - `SelfTestState SelfTest_Update(const SelfTestInputs *inputs, uint32_t now_ms)`
  - `uint8_t SelfTest_IsComplete(void)`
  - `uint8_t SelfTest_IsSafeToRun(void)`
  - hardware adapter initialization and polling functions returning `DriverStatus`
- Consumes: existing ADC, MPU6050, TCS34725, HC-SR04, speed, OLED and USART drivers.

- [ ] **Step 1: Write failing self-test sequence tests**

Verify ordered progression through ADC, MPU, color, ultrasonic, speed, battery and display. Verify disabled modules become `SELF_TEST_SKIPPED`, warning modules permit degraded completion, and fatal MPU failure prevents `safe_to_run`.

- [ ] **Step 2: Implement the pure self-test state machine**

The state machine only consumes injected results and reports errors; it does not call HAL.

- [ ] **Step 3: Implement hardware adapters**

Adapters call existing drivers and translate HAL results into `DRIVER_OK`, `DRIVER_NOT_READY`, `DRIVER_TIMEOUT` or `DRIVER_ERROR`. Do not yet alter `main.c`.

- [ ] **Step 4: Correct reusable driver defects**

Before each correction, add a focused host-testable helper test:

- HC-SR04 distance conversion has no fixed 3.8 cm subtraction.
- no-echo becomes timeout rather than stale distance.
- speed conversion has a documented unit and stopped-wheel timeout.
- TCS34725 requires consecutive matching classifications before emitting red or green.
- key debounce requires stable press duration before the down event.

- [ ] **Step 5: Verify and commit**

```bash
git add Core/Inc/services Core/Src/services Core/Inc Core/Src tests
git commit -m "feat: add self test and reusable hardware adapters"
```

---

### Task 7: 建立普通与高级任务框架

**Files:**
- Create: `Core/Inc/app/app_controller.h`
- Create: `Core/Src/app/app_controller.c`
- Create: `Core/Inc/app/normal_mission.h`
- Create: `Core/Src/app/normal_mission.c`
- Create: `Core/Inc/app/advanced_mission.h`
- Create: `Core/Src/app/advanced_mission.c`
- Create: `Core/Inc/app/route_detector.h`
- Create: `Core/Src/app/route_detector.c`
- Create: `tests/native/test_app_controller.c`
- Create: `tests/native/test_normal_mission.c`
- Create: `tests/native/test_advanced_mission.c`

**Interfaces:**
- Produces:
  - `void AppController_Init(void)`
  - `AppOutput AppController_Update(const AppInput *input, uint32_t now_ms)`
  - mission-specific start, update, stop and status functions
- Consumes: sensor snapshot, self-test state, error state, route events and control requests.

- [ ] **Step 1: Write failing app-controller tests**

Verify boot enters self-test, successful self-test enters wait, USER starts the selected mission, USER during run stops, and fatal error enters fault.

- [ ] **Step 2: Implement controller transitions**

No HAL calls. The output contains a motion request, indicator pattern, display page and mission status.

- [ ] **Step 3: Write failing normal mission tests**

Cover startup three-flash completion, tracking, red stop, green resume, obstacle stop, hazard hold, turn request, return and final stop using injected events.

- [ ] **Step 4: Implement normal mission framework**

Implement all state names and safe transitions. Route-specific thresholds come from configuration and remain subject to real-car calibration.

- [ ] **Step 5: Write failing advanced mission tests**

Cover A-B-C-D-A circle event order, A-C-B-D-A figure-eight event order, point flash request, lap count, timing result and challenge route alternation.

- [ ] **Step 6: Implement advanced framework and route detector contract**

`route_detector` accepts contextual sensor events and emits only enumerated route events. Keep physical point-recognition policy isolated for later calibration.

- [ ] **Step 7: Verify and commit**

```bash
git add Core/Inc/app Core/Src/app tests
git commit -m "feat: add normal and advanced mission frameworks"
```

---

### Task 8: 接入 OLED、灯光、串口和主循环

**Files:**
- Create: `Core/Inc/services/display_service.h`
- Create: `Core/Src/services/display_service.c`
- Modify: `Core/Src/main.c`
- Modify: `Core/Src/stm32f1xx_it.c`
- Modify: `Core/Src/gpio.c` only inside generated user regions if necessary
- Modify: `Core/Src/adc.c` only through CubeMX-compatible configuration changes
- Modify: `Core/Src/tim.c` only through CubeMX-compatible configuration changes
- Modify: `timer_pwm.ioc`

**Interfaces:**
- Produces: a thin `main.c` orchestration loop using all previous modules.
- Consumes: app output, diagnostic view, sensor adapters, motion control and indicator output.

- [ ] **Step 1: Add a source-boundary test that fails on direct control calls**

Create a PowerShell static test that fails if `Core/Src/app/*.c` contains `HAL_`, `TB6612_` or direct GPIO calls, or if more than `motion_control.c` calls `TB6612_SetSpeed()` outside compatibility wrappers.

- [ ] **Step 2: Verify RED against the pre-integration layout**

Expected: fail until directories and allow-list are recognized.

- [ ] **Step 3: Implement display service**

Normal pages show mission, real wheel speed, distance, color, battery and Yaw. Fault pages use `DiagnosticView_Format()`. OLED refresh remains at 200 ms and updates fixed-width lines without high-frequency clearing.

- [ ] **Step 4: Replace current main mission body with orchestration**

Preserve ADC DMA start and highest-priority MPU FIFO consumption. Initialize enabled timers and adapters, update services, run app controller, resolve safety, apply motion, update lights, display and UART. Remove duplicated mission business logic from `main.c` only after equivalent tests are green.

- [ ] **Step 5: Wire timer and EXTI callbacks**

Callbacks dispatch to MPU, HC-SR04 and speed capture handlers only when the matching peripheral and feature are active. Keep callbacks non-blocking.

- [ ] **Step 6: Verify native and static tests**

Run all tests, UTF-8 check and `git diff --check`.

- [ ] **Step 7: Commit**

```bash
git add Core timer_pwm.ioc tests docs/changes/2026-10-22-final-work-framework.md
git commit -m "feat: integrate final work application framework"
```

---

### Task 9: 更新 Keil 工程并执行完整验证

**Files:**
- Modify: `MDK-ARM/timer_pwm.uvprojx`
- Modify: `README.md`
- Modify: `docs/changes/2026-10-22-final-work-framework.md`
- Modify: relevant `学习资料/课内/*/README.md` only when new reusable knowledge was established

**Interfaces:**
- Produces: buildable Keil target containing every new production source and include directory.
- Consumes: all files from Tasks 1-8.

- [ ] **Step 1: Add new include paths and source groups to Keil project**

Add `Core/Inc/config`, `Core/Inc/control`, `Core/Inc/services`, `Core/Inc/app` and matching source files. Keep XML ordering stable and avoid user-specific Keil files.

- [ ] **Step 2: Run all host verification**

```powershell
powershell -ExecutionPolicy Bypass -File tests/run_native_tests.ps1
powershell -ExecutionPolicy Bypass -File tools/check_utf8.ps1
git diff --check
```

Expected: all exit 0 with no warnings treated as errors.

- [ ] **Step 3: Run Keil Rebuild**

Use the installed Keil executable discovered on the host:

```powershell
& 'D:\Keil_v5\UV4\UV4.exe' -b 'MDK-ARM\timer_pwm.uvprojx' -t 'timer_pwm' -o 'MDK-ARM\timer_pwm\final-work-rebuild.log'
```

Expected log: `0 Error(s), 0 Warning(s)` and a non-empty `MDK-ARM/timer_pwm/timer_pwm.hex`.

- [ ] **Step 4: Inspect build log and artifact**

Verify the log timestamp belongs to this run, all new source files compiled, and HEX length is non-zero. Do not infer flashing or real-car success.

- [ ] **Step 5: Finish documentation**

Update README with architecture, build, diagnostic display, error codes and real-car checklist. Complete the change record with evidence, exact commands, results and remaining hardware validation.

- [ ] **Step 6: Review diff and commit**

```bash
git status --short
git diff --check
git diff --stat
git add MDK-ARM/timer_pwm.uvprojx README.md docs 学习资料
git commit -m "docs: record final framework verification"
```

- [ ] **Step 7: Final branch audit**

Confirm the current branch is `codex/final-work-framework`, the original source directory has not changed, and no push occurred. Report separately:

- native test result;
- UTF-8 check result;
- Keil Rebuild result;
- HEX result;
- flashing status;
- real-car validation status.
