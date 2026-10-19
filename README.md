# STM32 智能小车实验工程

基于 STM32F103 的智能小车课程实验工程，使用 STM32CubeMX、HAL 库和 Keil MDK-ARM。

## 当前功能

- MPU6050 DMP 姿态角读取：Pitch、Roll、Yaw
- OLED 本地姿态显示
- 基于 Yaw 的定角旋转任务
- TCS34725 颜色识别与红停、绿行逻辑
- TB6612 电机驱动与轮速测量
- 串口命令控制和状态诊断
- HC-SR04 超声波避障代码保留，当前通过宏关闭

## 关键硬件连接

- MPU6050：IIC1，PB6/SCL、PB7/SDA，PA8/INT
- OLED：软件 IIC，PB5/SCL、PB4/SDA
- 核心板 LED：PA4
- USER 按键：以 `Core/Inc/main.h` 和 CubeMX `.ioc` 配置为准

## 当前姿态数据调度

- DMP/FIFO 输出频率：100 Hz
- MPU 主循环补读周期：10 ms
- OLED 刷新周期：200 ms
- 姿态串口输出周期：200 ms
- 颜色采样周期：60 ms
- 颜色串口输出周期：300 ms

姿态中断只负责置位标志，IIC/FIFO 读取在主循环中完成。读取函数会在 FIFO 积压时追读到较新的数据，避免 OLED 和串口阻塞导致长期显示旧姿态。

## 编译

1. 使用 Keil MDK-ARM 打开 `MDK-ARM/timer_pwm.uvprojx`。
2. 选择 `timer_pwm` target。
3. 执行 Rebuild。
4. 将生成的 HEX 下载到开发板。

已验证工具链：ARM Compiler 5.06 update 7，STM32F103 HAL 工程。

## 版本管理约定

- 一次提交只解决一个明确问题。
- 修改前先确认工作区状态，修改后至少执行一次完整 Rebuild。
- 需要实车验证的修改，在提交说明或实验记录中写明测试条件和结果。
- 不提交 Keil/IAR 构建产物和本机用户配置。
