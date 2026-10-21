# 四路 TCRT5000 数字循迹基础实验

## 目标与依据

按照实验指导书第 11.2 节，使用四个 TCRT5000 的 DO 输出，通过 GPIO 输入每 50 ms 判断一次黑线位置，并用 USER 按键切换小车启动和停止。

## 修改内容

- PA6、PA7、PB0、PB1 分别配置为 LINE1、LINE2、LINE3、LINE4 数字输入。
- 在 `car_control.c` 中加入 `Smart_Car_Automatic_Tracing()`，判定顺序和转向动作保持指导书逻辑。
- USER 按键切换启动/停止；停止时立即清除循迹标志并关闭电机。
- SysTick 在小车启动时每 50 ms 置位 `measure_flag`，具体 GPIO 读取和电机控制在主循环执行。
- 当前实验模式不初始化和执行 ADC/DMA、颜色识别、MPU6050、超声波与串口任务，避免争抢引脚和电机。
- 保留原有模块代码，实验 10.4 的完整版本可从提交 `c747765` 恢复。
- 将 `car_control.c` 从 GBK 转换为 UTF-8 BOM，并修复中文注释乱码。

## 验证结果

- Keil ARMCC Rebuild：0 Error(s), 0 Warning(s)。
- PA6、PA7、PB0、PB1 的 `.ioc`、`main.h` 和 `gpio.c` 配置一致。
- `main.c` 与 `car_control.c` 均通过 UTF-8 解码检查并带 BOM。

## 尚未验证

- 尚未烧录到核心板。
- 尚未确认实物模块的 DO 黑白电平是否与指导书的 `GPIO_PIN_SET=黑线` 一致。
- 尚未完成四个灵敏度电位器、传感器高度、直行速度和强弱转向速度的实车调整。
