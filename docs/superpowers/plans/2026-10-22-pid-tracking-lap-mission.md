# PID Tracking Lap Mission Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 在现有 STM32F103 小车工程中实现双通道 AO PID 循迹、MPU6050 Yaw 两圈计数、向右 180 度自动掉头、反向两圈后停车。

**Architecture:** PA7/PB0 由 GPIO 数字输入改为 ADC1 双通道扫描，DMA 循环提供左右最新采样；`car_control.c` 独占 PID 到电机的输出。`main.c` 保留经过验证的 MPU FIFO 消费链，并用单一任务状态机协调顺时针循迹、掉头、逆时针循迹、停止和错误，OLED/串口只作为低频观察通道。

**Tech Stack:** STM32F103 HAL、ADC1、DMA1 Channel 1、MPU6050 DMP/FIFO、TIM2 PWM、TB6612、SSD1306 OLED、Keil MDK ARM Compiler 5.06。

**Spec:** `docs/superpowers/specs/2026-10-22-pid-tracking-lap-state-machine-design.md`

## Global Constraints

- 指导书 PID 初值固定为 `Kp=1.5f`、`Ki=0.05f`、`Kd=2.0f`、`Base_Speed=3500`，第一版不预先优化。
- PID 控制周期为 50 ms；MPU 主动补读周期为 10 ms；OLED 与串口刷新周期不短于 200 ms。
- MPU DMP 保持 100 Hz；每次读取继续使用现有最多追读 16 帧并采用最新姿态的实现。
- 原 12 次 90 度姿态任务不再被任何按键、主循环或状态调用。
- 颜色、超声波、四路 DO 数字循迹保持静默，不得争抢电机。
- 中断只置位或递增轻量状态，不执行 I2C、OLED、串口、ADC 轮询或电机控制。
- 所有新增中文注释保持 UTF-8 BOM；不得重新引入乱码。
- 保留用户现有的 `MDK-ARM/timer_pwm.uvprojx` 未提交改动，不修改、不暂存该文件。
- 每个编译结论必须来自 Keil Rebuild；编译通过不得表述为已烧录或已通过实车验证。

## File Structure

- Modify: `timer_pwm.ioc` — PA7/PB0 的 ADC1 双通道与 DMA 真源配置。
- Modify: `Core/Src/adc.c` — 与 `.ioc` 一致的 ADC 扫描、连续转换、Rank 和模拟 GPIO 初始化。
- Modify: `Core/Src/gpio.c` — 不再把 PA7/PB0 初始化为数字输入，只保留 PA6/PB1 外侧 DO。
- Modify: `Core/Inc/main.h` — 移除不再有效的 LINE2/LINE3 数字引脚标签，保留 LINE1/LINE4。
- Modify: `Core/Src/car_control.c` — PID 算法、PID 状态复位、左右轮速度输出。
- Modify: `Core/Inc/car_control.h` — 导出 PID 控制与复位接口。
- Modify: `Core/Src/main.c` — ADC/DMA/MPU 初始化、任务状态机、Yaw 累计、按键、OLED 与串口调度。
- Modify: `Core/Src/stm32f1xx_it.c` — SysTick 只产生统一 50 ms 节拍，不再调度旧数字循迹。
- Create: `docs/changes/2026-10-22-pid-tracking-lap-mission.md` — 记录依据、修改、验证和未验证事项。
- Create: `学习资料/课内/PID模拟量循迹与姿态计圈/01-双路AO与Yaw计圈.md` — 独立学习笔记。
- Create: `学习资料/课内/PID模拟量循迹与姿态计圈/README.md` — 学习索引。
- Modify: `docs/changes/README.md` — 增加本次变更记录链接。

---

### Task 1: 双通道 ADC/DMA 与指导书 PID 控制

**Files:**
- Modify: `timer_pwm.ioc`
- Modify: `Core/Src/adc.c`
- Modify: `Core/Src/gpio.c`
- Modify: `Core/Inc/main.h`
- Modify: `Core/Src/car_control.c`
- Modify: `Core/Inc/car_control.h`

**Interfaces:**
- Consumes: PB0/ADC1_IN8 左内侧 AO，PA7/ADC1_IN7 右内侧 AO，`TB6612_SetSpeed()`。
- Produces: `void Line_Following_PID(uint16_t left_adc, uint16_t right_adc)`、`void Line_Following_PID_Reset(void)`、`int32_t Line_Following_PID_GetOutput(void)`。

- [ ] **Step 1: 记录当前配置检查应当失败的证据**

Run:

```powershell
rg -n "PA7.Signal=ADCx_IN7|PB0.Signal=ADCx_IN8|ADC1.NbrOfConversion=2|ADC_SCAN_ENABLE" timer_pwm.ioc Core/Src/adc.c
```

Expected: 当前 `.ioc` 仍显示 PA7/PB0 为 `GPIO_Input`，`adc.c` 仍为单通道 `ADC_SCAN_DISABLE`，因此找不到完整四项。

- [ ] **Step 2: 修改 `.ioc` 与生成源，使配置真源一致**

Set exactly:

```text
PB0.Signal=ADCx_IN8
PA7.Signal=ADCx_IN7
ADC1.ScanConvMode=ENABLE
ADC1.ContinuousConvMode=ENABLE
ADC1.NbrOfConversion=2
Rank 1 = ADC_CHANNEL_8
Rank 2 = ADC_CHANNEL_7
SamplingTime = ADC_SAMPLETIME_1CYCLE_5
DMA1_Channel1 = circular, halfword, memory increment
```

In `adc.c`, produce the matching initialization:

```c
hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
hadc1.Init.ContinuousConvMode = ENABLE;
hadc1.Init.DiscontinuousConvMode = DISABLE;
hadc1.Init.NbrOfConversion = 2;

sConfig.Channel = ADC_CHANNEL_8;
sConfig.Rank = ADC_REGULAR_RANK_1;
sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
HAL_ADC_ConfigChannel(&hadc1, &sConfig);

sConfig.Channel = ADC_CHANNEL_7;
sConfig.Rank = ADC_REGULAR_RANK_2;
HAL_ADC_ConfigChannel(&hadc1, &sConfig);
```

Initialize PB0 and PA7 as analog inputs in `HAL_ADC_MspInit()` and remove their digital-input initialization from `gpio.c`. Remove LINE2/LINE3 macros from `main.h`; retain PA6/LINE1 and PB1/LINE4.

- [ ] **Step 3: Replace the obsolete four-DO controller with the guide PID API**

Declare in `car_control.h`:

```c
void Line_Following_PID(uint16_t left_adc, uint16_t right_adc);
void Line_Following_PID_Reset(void);
int32_t Line_Following_PID_GetOutput(void);
```

Implement persistent controller state in `car_control.c`:

```c
#define PID_KP          1.5f
#define PID_KI          0.05f
#define PID_KD          2.0f
#define PID_BASE_SPEED  3500
#define PID_MAX_SPEED   7200
#define PID_I_LIMIT     8000

static int32_t pid_last_error = 0;
static int32_t pid_integral = 0;
static int32_t pid_output = 0;
```

`Line_Following_PID()` must calculate `right_adc - left_adc`, limit the integral to positive/negative 8000, limit each wheel to 0～7200, and write both motors in forward direction. `Line_Following_PID_Reset()` clears all three static variables; the getter only returns the latest output for low-frequency telemetry.

- [ ] **Step 4: Run focused static checks**

Run:

```powershell
rg -n "ADC_SCAN_ENABLE|ContinuousConvMode = ENABLE|ADC_CHANNEL_8|ADC_CHANNEL_7|NbrOfConversion = 2" Core/Src/adc.c
rg -n "Line_Following_PID|Line_Following_PID_Reset|PID_KP|PID_I_LIMIT" Core/Src/car_control.c Core/Inc/car_control.h
rg -n "LINE2|LINE3" Core/Src/car_control.c Core/Inc/main.h
```

Expected: ADC 配置项和 PID API 全部存在；最后一个命令无输出。

- [ ] **Step 5: Commit the independently reviewable ADC/PID layer**

```powershell
git add -- timer_pwm.ioc Core/Src/adc.c Core/Src/gpio.c Core/Inc/main.h Core/Src/car_control.c Core/Inc/car_control.h
git commit -m "feat: add dual ADC PID line control"
```

---

### Task 2: MPU 实时消费与计圈/掉头任务状态机

**Files:**
- Modify: `Core/Src/main.c`
- Modify: `Core/Src/stm32f1xx_it.c`

**Interfaces:**
- Consumes: `adc_dma_data[0]` 左 ADC、`adc_dma_data[1]` 右 ADC、`MPU6050_DMP_GetData()`、Task 1 的三个 PID API、`Smart_Car_Turn_Right()` 和 `Smart_Car_Stop()`。
- Produces: `TrackingMissionState`，以及只在 `main.c` 内使用的 `Tracking_Mission_Reset()`、`Tracking_Mission_Start()`、`Tracking_Mission_Update_Yaw(float)`、`Tracking_Mission_Process(uint8_t)`。

- [ ] **Step 1: 先让新状态机接口产生明确的编译失败**

Add the enum and prototypes, then call `Tracking_Mission_Process(control_tick_due)` once before defining the functions:

```c
typedef enum
{
  TRACK_WAIT_START = 0,
  TRACK_CW,
  TRACK_TURN_AROUND,
  TRACK_CCW,
  TRACK_COMPLETE,
  TRACK_ERROR
} TrackingMissionState;
```

Run the Keil rebuild command from Step 7 below.

Expected: FAIL with undefined tracking mission functions, proving the new path is actually compiled.

- [ ] **Step 2: Remove the old 12-by-90-degree task from scheduling**

Delete or fully disconnect `PoseTaskState`、`Pose_Task_Is_Active()`、`Pose_Task_Stop()`、`Pose_Task_Update_Yaw()` and `Pose_Task_Process()` plus their private variables. Remove all calls from USER, serial, color and ultrasonic paths. Keep the low-level MPU driver and DMP/FIFO functions unchanged.

Do not restore the old `POSE_TOTAL_TURNS=12U` or the old one-second repeated-turn behavior anywhere in the new mission.

- [ ] **Step 3: Implement wrap-safe signed Yaw accumulation**

Use these exact transition targets:

```c
#define TRACK_CW_TARGET_DEG       720.0f
#define TRACK_TURN_TARGET_DEG     180.0f
#define TRACK_CCW_TARGET_DEG      720.0f
#define TRACK_YAW_DIRECTION       (-1.0f)
```

Normalize the per-frame delta:

```c
yaw_delta = current_yaw - tracking_last_yaw;
if (yaw_delta > 180.0f) yaw_delta -= 360.0f;
else if (yaw_delta < -180.0f) yaw_delta += 360.0f;
```

Accumulate clockwise and turnaround progress with `yaw_delta * TRACK_YAW_DIRECTION`; accumulate counterclockwise progress with its negative. Clamp progress at zero. On every state transition, set the new Yaw baseline and clear PID history.

- [ ] **Step 4: Implement the single motor-owning mission state machine**

State behavior must be exactly:

```text
WAIT_START: stop
TRACK_CW: every 50 ms run PID; at 720 degrees reset PID and enter TURN_AROUND
TURN_AROUND: call Smart_Car_Turn_Right; at 180 degrees stop, reset PID, enter TRACK_CCW
TRACK_CCW: every 50 ms run PID; at 720 degrees stop and enter COMPLETE
COMPLETE: remain stopped
ERROR: remain stopped
```

USER behavior:

```text
WAIT/COMPLETE + valid ADC/MPU -> clear counters and start TRACK_CW
CW/TURN/CCW -> immediate stop and reset to WAIT_START
ERROR -> remain stopped; do not conceal the fault
```

- [ ] **Step 5: Restore ADC/DMA and MPU initialization for this experiment**

Initialize DMA, USART3, I2C1 and ADC1 together with GPIO/TIM2. Run:

```c
HAL_ADCEx_Calibration_Start(&hadc1);
HAL_ADC_Start(&hadc1);
HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_dma_data, 2U);
```

Then probe 0x68/0x69 and initialize DMP. Any ADC initialization failure or MPU detection/DMP failure sets `TRACK_ERROR` before the USER key can start the car.

- [ ] **Step 6: Enforce producer/consumer scheduling**

At the top of each main-loop pass, before OLED, UART and PID work:

```c
if ((init_flag == 1U) &&
    ((mems_data_rdy_flag == 1U) ||
     ((HAL_GetTick() - last_mpu_poll_ms) >= 10U)))
{
  mems_data_rdy_flag = 0U;
  last_mpu_poll_ms = HAL_GetTick();
  if (MPU6050_DMP_GetData(&Pitch, &Roll, &Yaw) == 0U)
  {
    Tracking_Mission_Update_Yaw(Yaw);
  }
}
```

Do not add printing or OLED calls inside this block. Continue relying on the existing driver’s bounded FIFO追读; do not replace it with a single-oldest-frame read.

- [ ] **Step 7: Simplify SysTick to one 50 ms request flag**

Keep `HAL_IncTick()` and make the 50 ms branch only set `control_tick_50ms = 1U`. Remove digital-mode checks and `measure_flag` writes. The main loop atomically consumes the flag once; it must not run catch-up PID iterations after a delay.

- [ ] **Step 8: Rebuild and inspect the mission path**

Run:

```powershell
$args = @('-j0','-r','D:\dev\stm32_projects\timer_pwm_4_4\MDK-ARM\timer_pwm.uvprojx','-o','D:\dev\stm32_projects\timer_pwm_4_4\MDK-ARM\pid-mission-rebuild.log')
$process = Start-Process -FilePath 'D:\Keil_v5\UV4\UV4.exe' -ArgumentList $args -Wait -PassThru -WindowStyle Hidden
Get-Content -LiteralPath 'MDK-ARM\pid-mission-rebuild.log'
```

Expected: `0 Error(s), 0 Warning(s)` and a regenerated HEX. Also run:

```powershell
rg -n "POSE_TOTAL_TURNS|Pose_Task_|Smart_Car_Automatic_Tracing" Core/Src/main.c Core/Src/stm32f1xx_it.c
```

Expected: no active legacy task or digital-tracking calls.

- [ ] **Step 9: Commit the mission state machine**

```powershell
git add -- Core/Src/main.c Core/Src/stm32f1xx_it.c
git commit -m "feat: add yaw counted tracking mission"
```

---

### Task 3: OLED/串口诊断、错误优先级与学习记录

**Files:**
- Modify: `Core/Src/main.c`
- Create: `docs/changes/2026-10-22-pid-tracking-lap-mission.md`
- Create: `学习资料/课内/PID模拟量循迹与姿态计圈/01-双路AO与Yaw计圈.md`
- Create: `学习资料/课内/PID模拟量循迹与姿态计圈/README.md`
- Modify: `docs/changes/README.md`

**Interfaces:**
- Consumes: current mission state, left/right ADC, latest Yaw, accumulated progress, `Line_Following_PID_GetOutput()` and `AdcErrorState`.
- Produces: `OLED_Update_Tracking_Mission()` and a throttled UART status report; no control decisions originate from either output path.

- [ ] **Step 1: Replace the obsolete five-channel OLED page**

Display four compact rows at no faster than 200 ms:

```text
MODE:WAIT/CW/TURN/CCW/DONE
L:xxxx R:xxxx
DEG:xxx/720 or DEG:xxx/180
YAW:+xxx.x
```

Avoid dynamic float formatting where a scaled integer can be shown. Do not call `OLED_Clear()` every frame; overwrite fixed-width fields to reduce I2C time.

- [ ] **Step 2: Give errors display and motor priority**

Priority order:

```text
ADC CAL ERR
ADC START ERR
ADC DMA ERR
ADC RUN ERR
MPU NOT FOUND
DMP INIT FAIL
normal mission page
```

When any error is active, force `TRACK_ERROR`, call `Smart_Car_Stop()`, and prevent USER from starting the mission. `HAL_ADC_ErrorCallback()` continues to only set an error value; the main loop performs display and stop handling.

- [ ] **Step 3: Add throttled UART evidence**

Every 200 ms at most, print one line containing state, ADC, PID output, Yaw and progress:

```c
printf("TRACK:%s L:%u R:%u PID:%ld YAW:%.2f DEG:%.1f\r\n",
       state_text,
       adc_dma_data[0],
       adc_dma_data[1],
       (long)Line_Following_PID_GetOutput(),
       Yaw,
       tracking_progress);
```

No per-frame MPU print and no print from an interrupt.

- [ ] **Step 4: Write durable project and learning documentation**

The change record must distinguish:

- guide requirements;
- the page-27 variable/channel typo correction;
- producer/consumer scheduling safeguards;
- compile verification;
- hardware steps still requiring the user: AO wiring, download, lifted-wheel check, clockwise laps, right U-turn, counterclockwise laps and PID tuning.

The learning note must explain physical AO voltage → ADC ranks → DMA array → PID error → motor PWM → Yaw accumulation → state transition, and include the final pin/rank mapping.

- [ ] **Step 5: Verify Chinese encoding and repository scope**

Run:

```powershell
$utf8 = [System.Text.UTF8Encoding]::new($false, $true)
$paths = @('Core/Src/main.c','Core/Src/car_control.c','docs/changes/2026-10-22-pid-tracking-lap-mission.md','学习资料/课内/PID模拟量循迹与姿态计圈/01-双路AO与Yaw计圈.md')
foreach($path in $paths){$null=$utf8.GetString([System.IO.File]::ReadAllBytes((Join-Path (Get-Location) $path))); "UTF8 OK: $path"}
git diff --check
git status --short
```

Expected: all files decode; no whitespace errors; `MDK-ARM/timer_pwm.uvprojx` remains modified but unstaged.

- [ ] **Step 6: Commit diagnostics and documentation**

```powershell
git add -- Core/Src/main.c docs/changes/2026-10-22-pid-tracking-lap-mission.md docs/changes/README.md '学习资料/课内/PID模拟量循迹与姿态计圈/README.md' '学习资料/课内/PID模拟量循迹与姿态计圈/01-双路AO与Yaw计圈.md'
git commit -m "docs: add PID tracking mission diagnostics"
```

---

### Task 4: Final build, review and hardware handoff

**Files:**
- Review only: all files changed by Tasks 1–3
- Do not stage: `MDK-ARM/timer_pwm.uvprojx`

**Interfaces:**
- Consumes: complete firmware and documentation from Tasks 1–3.
- Produces: verified HEX plus an exact lifted-wheel and track-test checklist for the user.

- [ ] **Step 1: Run the final Keil Rebuild**

Run the same hidden UV4 rebuild command from Task 2, writing `MDK-ARM/pid-mission-final-rebuild.log`.

Expected: `0 Error(s), 0 Warning(s)` and `FromELF: creating hex file`.

- [ ] **Step 2: Review the exact Git diff**

```powershell
git diff --check
git diff --stat HEAD~3..HEAD
git status --short
git diff -- MDK-ARM/timer_pwm.uvprojx
```

Expected: source/config/docs changes match the mission; the project file shows only the pre-existing user change and is not staged.

- [ ] **Step 3: Perform the non-hardware consistency audit**

```powershell
rg -n "PA7.Signal=ADCx_IN7|PB0.Signal=ADCx_IN8|ADC1.NbrOfConversion=2|ADC1.ContinuousConvMode=ENABLE" timer_pwm.ioc
rg -n "TRACK_CW_TARGET_DEG|TRACK_TURN_TARGET_DEG|TRACK_CCW_TARGET_DEG|MPU_POLL_INTERVAL_MS|TRACK_DISPLAY_INTERVAL_MS|TRACK_UART_INTERVAL_MS" Core/Src/main.c
rg -n "Line_Following_PID|Line_Following_PID_Reset|PID_KP|PID_KI|PID_KD|PID_BASE_SPEED" Core/Src/car_control.c Core/Inc/car_control.h
```

Expected: exact pin mapping, angle targets, task periods and guide PID values are present once in their owning modules.

- [ ] **Step 4: Push commits and hand off physical verification**

```powershell
git push origin main
```

Provide this ordered hardware checklist without claiming it has already passed:

```text
1. Connect inner-left AO to PB0 and inner-right AO to PA7.
2. Lift wheels; power on; confirm OLED has no ADC/MPU error.
3. Move black line under each inner sensor and confirm the correct L/R ADC changes.
4. Rotate the car slowly through ±180 degrees and confirm Yaw remains current, not delayed.
5. Press USER while lifted; confirm PID wheel correction and immediate second-press stop.
6. Place on track; verify two clockwise laps, right 180-degree turnaround, two counterclockwise laps, then stop.
7. Only after the sequence works, tune Base_Speed, then Kp, then Kd, and finally Ki.
```
