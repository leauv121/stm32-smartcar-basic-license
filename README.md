# STM32 智能小车最终大作业

本工程位于 `D:\dev\final-work`，基于 STM32F103C8T6、HAL 库和 Keil MDK-ARM。开发分支为 `codex/final-work-framework`，原工程 `D:\dev\stm32_projects\timer_pwm_4_4` 仅作为克隆来源，没有在本次开发中修改。

## 已实现的模块

- 应用总控：上电自检、等待、运行、故障四种总状态。
- 普通任务：启动闪灯、循迹、红停绿行、障碍停车、双闪等待、定角避障、返程和到家停车。
- 高级任务：圆形、八字、稳定性和挑战模式的路线顺序、圈数及到点提示框架。
- 电脑控制：USART3 接收模式选择、启停、状态、故障查询和确认命令。
- 集中故障管理：按模块登记、去重、分级、轮播，并由 OLED 显示故障模块与代码。
- 安全仲裁：致命故障、用户停车或任务完成时覆盖运动请求，恢复后必须 `ACK` 才允许重新运行。
- 控制层：循迹 PID、外侧数字探头救线、Yaw 定角转弯和唯一电机输出适配器。
- 传感器层：ADC 循迹、MPU6050 DMP、TCS34725、HC-SR04、左右轮速的统一快照与超时检测。
- 显示与灯光：OLED 正常页/故障页、左右转向灯、双闪和启动三闪。

## 串口使用

USART3 配置为 115200、8 数据位、1 停止位、无校验。每条命令以回车或换行结束。

```text
MODE=NORMAL
MODE=CIRCLE
MODE=EIGHT
MODE=STABLE
MODE=CHALLENGE
START
STOP
STATUS?
ERRORS?
ACK
```

推荐操作顺序：先发送 `MODE=...`，再发送 `START`。运行中修改模式会返回忙状态；存在致命故障时 `START` 和 `ACK` 都不能绕过故障。排除故障并使错误记录清除后，再发送 `ACK`。USER 按键可对已选模式执行启动或停车。

## OLED 检错机制

正常时 OLED 显示任务状态、模式、左右轮速、距离、颜色、Yaw 和电池占位值。存在故障时自动切换为四行诊断页，例如：

```text
ERROR 01/03
MOD:MPU6050
CODE:DATA TIME
MOTOR STOP
```

致命错误会立即锁定停车；多个同等级错误每 2 秒轮换。当前可定位系统、ADC、循迹、MPU6050、左右测速、颜色、超声波、电池、电机、任务和显示模块。

## 关键硬件连接

- MPU6050：I2C1，PB6/SCL、PB7/SDA，PA8/INT。
- OLED：软件 I2C，PB5/SCL、PB4/SDA。
- TB6612 PWM：TIM2 CH1/CH2，方向引脚见 `Core/Inc/main.h`。
- 左右灯：PC13、PC14；核心板 LED：PA4；USER 按键：PA5。
- 外侧循迹数字量：PA6、PB1；内侧循迹模拟量：ADC DMA 两通道。
- HC-SR04：TIM1 CH4 输入捕获，PA12 触发。
- 左右测速：TIM4 CH4/CH3。

## 构建与测试

```powershell
powershell -ExecutionPolicy Bypass -File tests\run_native_tests.ps1
powershell -ExecutionPolicy Bypass -File tests\check_source_boundaries.ps1
powershell -ExecutionPolicy Bypass -File tools\check_utf8.ps1
```

Keil：打开 `MDK-ARM\timer_pwm.uvprojx`，选择 `timer_pwm`，执行 Rebuild。已使用 ARM Compiler 5.06 update 7 完成全量构建，并生成 `MDK-ARM\timer_pwm\timer_pwm.hex`。

## 实车前必须完成

- 确认左右电机正反方向、左右灯有效电平和外侧探头极性。
- 标定循迹 ADC 中心值、PID、基础速度、救线速度和弯道表现。
- 标定 TCS34725 红/绿 Hue 范围；当前需连续 3 次一致才确认。
- 标定 20 cm 障碍阈值、避障角度及双闪等待时间。
- 根据赛道实际标记校准 A/B/C/D 路点规则；当前映射为双外侧=A、左外侧=B、右外侧=C、红色=D。
- 增加电池分压 ADC 通道后再启用电池自检；当前 OLED 电池值为占位数据。
- 分别完成烧录验证、架空轮测试、低速地面测试和完整赛道测试。

## 设计与记录

- [架构设计](docs/superpowers/specs/2026-10-22-final-work-architecture-design.md)
- [实施计划](docs/superpowers/plans/2026-10-22-final-work-framework.md)
- [变更与验证记录](docs/changes/2026-10-22-final-work-framework.md)
