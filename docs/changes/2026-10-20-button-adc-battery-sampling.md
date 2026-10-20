# USER 按键触发单通道 ADC 电池电压采样

## 目标

将《第6次课程实验指导书》第5～7页的单通道 ADC 采样流程接入现有工程：按下 USER 按键后读取 PA3 上的 ADC1_IN3，将分压后的结果还原为电池电压，并通过 USART3 输出。原先由 USER 按键启动或停止的定角旋转任务不再触发。

## 修改前证据

- `timer_pwm.ioc` 已把 PA3 配置为 ADC1_IN3，ADC 时钟为 12 MHz。
- `Core/Src/adc.c` 已生成 ADC1 单通道、软件触发、单次转换配置。
- `Core/Src/main.c` 已调用 `MX_ADC1_Init()`，USART3 的 `printf()` 重定向也已存在。
- USER 按键原先调用 `Pose_Task_Start()`，启动 12 段、每段 90° 的姿态任务。

## 修改位置

- `Core/Src/main.c`
  - 在 `USER CODE BEGIN 1` 中增加 ADC 原始值和电池电压变量。
  - 在 `USER CODE BEGIN 2` 中增加 STM32F1 ADC 自校准。
  - 将现有 USER 按键分支改为单次 ADC 启动、等待、读取、换算、打印和停止。
  - 移除只由 USER 按键调用的 `Pose_Task_Start()`，使原按键旋转功能保持沉默。
  - 为新增流程增加中文注释和超时、启动失败提示。

## 电压换算

扩展板使用 1:1 电阻分压，指导书采用的计算式为：

$$
V_{\mathrm{BAT}}=\frac{N_{\mathrm{ADC}}}{4095}\times3.3\times2+0.02.
$$

其中 $0.02\text{ V}$ 是指导书给出的经验补偿，实车应使用万用表复核。

## 验证结果

- 已使用 ARM Compiler 5.06 update 7 执行 Keil Rebuild。
- 编译结果：`0 Error(s), 0 Warning(s)`，已生成 HEX。
- 未执行程序烧录。
- 未执行电池实测，串口数值与万用表读数仍需在硬件上比较。

## 实机检查步骤

1. 确认电池分压输出连接 PA3，且 ADC 引脚电压不超过 3.3 V。
2. 打开 USART3 串口，配置为 115200、8-N-1。
3. 每按一次 USER 按键，应只出现一行 `Battery ADC:... Voltage:...V`。
4. 按键不应再启动或停止定角旋转任务。
5. 使用万用表测量电池端电压，并与串口计算值比较。
