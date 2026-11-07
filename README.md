# STM32 智能小车：初级驾照任务系统

基于 STM32F103C8T6 的智能小车课程项目。系统围绕“初级驾照”任务构建了一条完整控制链：启动提示、混合四探头循迹、红停绿行、超声波障碍停车、姿态辅助掉头、重新找线、返程与终点停车。

项目不仅实现车辆动作，还把传感器采样、任务状态机、运动控制、安全仲裁、故障诊断、OLED 显示和串口遥测拆成可测试模块，用实际调试记录说明每个结论的证据边界。

技术栈：STM32F103C8T6、C99、STM32 HAL/CMSIS、STM32CubeMX、Keil MDK、Arm Compiler 5.06、MinGW GCC、PowerShell。

## 项目概览

初级驾照任务的主流程为：

```text
上电自检
  -> 等待启动
  -> 启动灯光提示
  -> 去程循迹
  -> 红牌停车 / 绿牌放行
  -> 20 cm 障碍确认与停车
  -> MPU6050 航向增量辅助掉头
  -> 低速找回黑线
  -> 返程循迹
  -> 再次检测终点并停车
```

程序以 `AppRuntime` 为运行入口。传感器数据先进入统一快照，任务状态机生成运动请求，`SafetyService` 再根据致命故障、用户停止和任务状态进行最终仲裁，只有通过仲裁的请求能够到达电机输出。

仓库还保留圆形、八字、稳定性和挑战模式的扩展框架，用于展示任务层的可扩展性；这些高级路线没有作为本仓库的已完成实车成果。

## 简历项目亮点

- 设计分层嵌入式架构，将硬件采样、控制算法、任务状态机、安全仲裁、故障管理和显示通信解耦，避免业务状态直接操作电机。
- 使用两路模拟量内侧探头与两路数字量外侧探头构建混合循迹：内侧 ADC 提供连续位置误差，外侧探头提供大偏差方向信息，并通过方向锁存和稳定居中确认完成控制权交接。
- 基于 ADC DMA、MPU6050 DMP、TCS34725、HC-SR04、编码器和 OLED 完成多传感器协作，使用不同节拍调度控制、显示与串口任务，减少阻塞操作对姿态 FIFO 和控制实时性的影响。
- 建立集中故障管理和安全停车机制：致命故障立即覆盖运动请求；故障恢复后仍需显式确认，避免传感器短暂恢复导致车辆自行重新启动。
- 保留问题复现、失败方案、调参回退和实车观察记录，使“代码推断、编译结果、主机测试、烧录与实车验证”可以分别追溯。

## 初级驾照任务流程

初级任务由 `Core/Src/app/normal_mission.c` 管理，主要状态包括：

```text
NORMAL_WAIT_START
  -> NORMAL_START_FLASH
  -> NORMAL_TRACK_OUT
  -> NORMAL_RED_WAIT
  -> NORMAL_OBSTACLE_HOLD
  -> NORMAL_AVOID_TURN
  -> NORMAL_REACQUIRE_LINE
  -> NORMAL_RETURN_CLEAR
  -> NORMAL_RETURN
  -> NORMAL_COMPLETE
```

关键行为：

- 红牌事件进入锁存停车状态，移开红牌不会自动继续；只有连续确认绿牌才恢复行驶。
- 超声波使用检测阈值、释放阈值和连续样本确认，避免单次噪声触发掉头。
- 掉头阶段累计 MPU6050 的航向增量，达到标定角度后不直接高速返程，而是进入低速找线。
- 重新压线后要求中间区域持续稳定，再恢复正常 PID，降低斜着切回赛道造成的二次丢线。
- 返程终点检测具有离开解锁过程，避免刚掉头时仍处于原障碍距离而立即误判完成。

## 系统架构

```text
GPIO / ADC DMA / I2C / Timer Capture / UART
                    |
                    v
              SensorService
                    |
                    v
 AppController + NormalMission + TurnControl
                    |
                    v
             MotionRequest
                    |
                    v
              SafetyService <--- ErrorManager
                    |
                    v
              MotionControl
                    |
                    v
               TB6612 / Motors

DisplayService、DiagnosticView、LineTelemetry 和 UartCommandService
从同一份状态与传感器快照生成可观察信息。
```

主要模块：

- `Core/Src/app/`：应用总控、普通任务、高级扩展框架、命令解析与运行时编排。
- `Core/Src/control/`：循迹、直行/运动和定角转向控制。
- `Core/Src/services/`：传感器快照、自检、调度、安全、故障、显示、串口与遥测。
- `Core/Inc/config/car_parameters.h`：控制周期、循迹阈值、速度、掉头角度和距离阈值的集中配置。
- `Core/Src/car_control.c`：兼容旧接口并统一进入新的控制与安全链路。

## 硬件与引脚

- 主控：STM32F103C8T6，72 MHz。
- 电机驱动：TB6612，TIM2 CH1/CH2 输出 PWM。
- 循迹：PA6 右外侧数字探头、PA7 右内侧模拟量、PB0 左内侧模拟量、PB1 左外侧数字探头。
- 模拟量采集：ADC1 双通道连续扫描，DMA 循环搬运。
- 姿态：MPU6050，I2C1 使用 PB6/SCL、PB7/SDA，PA8/INT。
- 颜色：TCS34725，用于红牌和绿牌识别。
- 测距：HC-SR04，PA12 触发，TIM1 CH4 输入捕获回波。
- 测速：TIM4 CH4/CH3 采集左右轮脉冲。
- OLED：软件 I2C，PB5/SCL、PB4/SDA。
- 串口：USART3，115200 8N1。
- USER 按键：PA5。

完整配置以 [timer_pwm.ioc](timer_pwm.ioc) 和 [Core/Inc/main.h](Core/Inc/main.h) 为准。

## 混合四探头循迹

循迹控制器位于 [line_follow_control.c](Core/Src/control/line_follow_control.c)。四个探头从车体右侧到左侧使用位置权重 `+3、+1、-1、-3`：

- 两个内侧探头读取模拟量，先根据黑白标定值归一化，再参与加权位置计算。
- 两个外侧探头是数字量，检测到黑线时提供更强的方向信息，帮助车辆从急弯和大偏差状态回线。
- PID 输出形成左右轮差速，并经过纠偏限幅、基础速度选择和软启动。
- 丢线时沿用最后一次有效纠偏方向，不把无效样本伪装成新的中心位置。
- 掉头后的强制找线采用较低的外轮/内轮速度；只有外侧探头释放、中间两路达到黑线阈值、两路误差足够小并连续确认后，才重新交还普通 PID。

这套设计的重点不是把所有传感器都塞进同一个公式，而是根据传感器特性分工：模拟量负责连续控制，数字量负责边界事件和方向提示。

## 状态机与安全策略

- `AppController` 管理 `SELF_TEST、WAIT、RUNNING、FAULT` 四类应用状态。
- `NormalMission` 只描述任务阶段并输出运动意图，不直接写 PWM。
- `ErrorManager` 按模块、错误码和等级登记故障，并为 OLED 提供轮播记录。
- `SafetyService` 对用户急停、任务完成和致命故障拥有最高优先级；停车锁存解除前，普通任务不能重新驱动电机。
- `SelfTest` 检查关键模块，允许安全降级项与致命项采用不同处理。
- `UartCommandService` 支持模式选择、启动、停止、状态查询、错误查询和确认命令。

常用串口命令：

```text
MODE=NORMAL
START
STOP
STATUS?
ERRORS?
ACK
```

循迹遥测输出包含时间、内侧 ADC、外侧数字量、PID 输出、左右 PWM 目标和控制模式，可直接导入串口上位机绘图。

## 调试方法与工程经验

项目中的重要反馈被整理为以下工程结论：

1. 先验证传感器极性，再调 PID。外侧数字探头极性相反时，所有救线方向都会整体反转，继续修改增益只会掩盖根因。
2. 传感器要按能力分工。模拟量适合连续位置反馈，数字量适合判断是否到达边界；外侧数字探头不应直接、永久覆盖内侧 PID。
3. 状态切换必须处理控制器历史。停车、掉头和重新循迹前后需要清理积分、微分和旧误差，否则恢复瞬间可能反打。
4. 回中不能依赖单帧。振动和黑线边缘会让数字探头短暂翻转，需要方向锁存和连续居中确认。
5. 更高频率不等于更实时。曾尝试提高控制频率，但 OLED、串口、软件 I2C 和 DMP FIFO 的总时间预算不足，恢复 20 ms 控制周期后更稳定。
6. 姿态少读优先检查数据链。MPU6050 角度滞后曾来自 FIFO 消费不及时，不能用角度比例系数补偿旧数据。
7. 交通灯需要非对称确认。红牌要快速停车并锁存，绿牌要更严格确认，避免牌面移开或光照波动导致误放行。
8. 不混淆验证层级。源码分析、主机测试、Keil 编译、固件烧录和完整实车通过分别记录，不能用其中一项替代另一项。

完整问题记录见 [工程变更记录](docs/changes/README.md)，循迹与姿态专题笔记见 [PID 模拟量循迹与姿态计圈](学习资料/课内/PID模拟量循迹与姿态计圈/README.md)。

## 构建、烧录与运行

### Keil MDK

1. 打开 [MDK-ARM/timer_pwm.uvprojx](MDK-ARM/timer_pwm.uvprojx)。
2. 选择目标 `timer_pwm`。
3. 执行 Rebuild。
4. 使用 CMSIS-DAP 或工程中配置的下载器烧录。
5. 上电后先观察 OLED 自检页，确认无致命故障，再按 USER 或发送 `START`。

工程使用 Arm Compiler 5.06。发布前的完整 Rebuild 结果为 `0 Error(s), 0 Warning(s)`。

### 主机端检查

需要把 MinGW GCC 加入 `PATH`：

```powershell
powershell -ExecutionPolicy Bypass -File tests\run_native_tests.ps1
powershell -ExecutionPolicy Bypass -File tests\check_source_boundaries.ps1
powershell -ExecutionPolicy Bypass -File tools\check_utf8.ps1
```

主机测试覆盖状态机、调度、安全仲裁、故障管理、循迹控制、转向控制、显示、串口命令和算法工具。测试不能代替烧录和实车验证。

## 验证状态

已确认：

- Keil Arm Compiler 5.06 全量编译：`0 Error(s), 0 Warning(s)`。
- 主机测试中的应用状态机、安全仲裁、错误管理、显示、命令解析、遥测等测试可执行。
- 源码边界和 UTF-8 检查可独立运行。
- 历史记录包含循迹极性、掉头惯性、红绿牌阈值和 MPU6050 FIFO 延迟等实车观察。

已知测试状态：

- `test_line_follow_control.c` 当前保留两条与最终调参值不一致的断言：外侧探头 PID 上限期望，以及弯道左右目标和期望。
- 因此本仓库不声明“全部主机测试通过”；这两条记录保留为参数演进与测试期望需要同步的证据。

验证边界：

- 编译成功不等于固件已烧录。
- 单模块实车现象不等于完整赛道流程通过。
- README 只把已有记录明确支持的实车观察写成实车结论。

## 已知限制

- 两轮主要使用 PWM 开环控制，编码器数据尚未形成完整的左右轮闭环速度控制。
- 循迹阈值、颜色阈值、掉头角度和超声波距离需要按车辆机械状态、环境光和赛道材料重新标定。
- 电池分压 ADC 尚未接入完整应用链，OLED 对电池状态显示 `N/A`，不输出伪造电压。
- 高级路线框架仍需完成路点规则和完整实车验证，不属于本仓库声明完成的初级驾照成果。
- 最新 OLED 字段和部分遥测改动已通过编译或主机检查，但仍需要在目标板上逐项观察。

## 目录结构

```text
Core/
  Inc/                 接口、配置和板级声明
  Src/app/             应用总控与任务状态机
  Src/control/         循迹、运动和转向控制
  Src/services/        调度、传感器、安全、故障、显示和通信
  Src/                 CubeMX 外设与硬件驱动
Drivers/               STM32 HAL、CMSIS 等第三方组件
MDK-ARM/               Keil 工程
EWARM/                 IAR 工程文件
tests/                 Windows/MinGW 主机端测试
tools/                 编码和辅助检查
docs/changes/          问题、修改、验证和未验证事项
学习资料/课内/         从实验整理的专题笔记
timer_pwm.ioc           STM32CubeMX 配置
```

## 深入阅读

- [工程变更记录索引](docs/changes/README.md)
- [GitHub 简历作品集发布设计](docs/changes/2026-11-07-github-portfolio-release.md)
- [四路数字循迹基础](学习资料/课内/四路数字循迹基础/README.md)
- [PID 模拟量循迹与姿态计圈](学习资料/课内/PID模拟量循迹与姿态计圈/README.md)
- [MPU6050 姿态控制](学习资料/课内/MPU6050姿态控制/README.md)

## 简历描述示例

- 基于 STM32F103C8T6 与 HAL 库开发智能小车初级驾照任务系统，采用分层状态机组织启动、循迹、交通灯响应、避障掉头、重新找线和返程流程，并通过安全仲裁统一约束电机输出。
- 设计两路 ADC 模拟量与两路 GPIO 数字量融合的四探头循迹控制，完成位置 PID、外侧方向辅助、救线锁存、稳定居中交接、软启动和输出限幅，提升急弯与掉头后回线的可控性。
- 集成 MPU6050 DMP、TCS34725、HC-SR04、编码器、OLED 与 USART3，建立传感器快照、故障分级、串口遥测和主机测试体系，并通过调试记录复盘 FIFO 延迟、传感器极性和控制周期等工程问题。

## License

项目原创代码与文档采用 [MIT License](LICENSE)。STM32 HAL、CMSIS、MPU6050/DMP 等第三方组件保留各自许可证，详见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。
