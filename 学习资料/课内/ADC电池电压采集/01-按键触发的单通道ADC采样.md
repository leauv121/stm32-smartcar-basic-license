# 按键触发的单通道 ADC 采样

## 信号路径

```text
锂电池 → 1:1电阻分压 → PA3/ADC1_IN3 → 12位ADC结果 → 电压换算 → USART3
```

锂电池满电电压可能达到 $4.2\text{ V}$，超过 STM32 ADC 的安全输入范围。两个相同电阻先把电池电压减半，使 PA3 上的电压保持在 $0\sim3.3\text{ V}$ 内。

## 程序执行顺序

1. 系统初始化 ADC1 和 USART3。
2. 程序启动后执行一次 ADC 自校准。
3. 主循环持续扫描 USER 按键，但一次按下只产生一次事件。
4. 按键事件启动 ADC1 单次转换。
5. 程序最多等待 10 ms。
6. 转换完成后读取 $0\sim4095$ 的结果。
7. 按分压比例还原电池电压并通过串口输出。
8. 停止 ADC，等待下一次按键。

## 换算关系

ADC 引脚电压为：

$$
V_{\mathrm{ADC}}=\frac{N_{\mathrm{ADC}}}{4095}\times3.3.
$$

由于分压比例为 1:1，电池电压约为：

$$
V_{\mathrm{BAT}}=V_{\mathrm{ADC}}\times2+0.02.
$$

合并后得到：

$$
V_{\mathrm{BAT}}=\frac{N_{\mathrm{ADC}}}{4095}\times3.3\times2+0.02.
$$

## 代码位置与职责

- `main()` 的局部变量区保存 `adc_value` 和 `voltage_value`。
- 初始化区调用 `HAL_ADCEx_Calibration_Start()`，它只需要在正常采样前执行一次。
- 按键分支调用 `HAL_ADC_Start()`、`HAL_ADC_PollForConversion()`、`HAL_ADC_GetValue()` 和 `HAL_ADC_Stop()`。
- `Key_Proc()` 仍然负责按键边沿识别，但不再启动原来的定角旋转任务。

## 注意事项

- 不要让 PA3 电压超过 3.3 V，也不要直接把满电锂电池接到 PA3。
- `HAL_ADC_PollForConversion()` 成功只表示数字转换完成，不表示换算一定准确。
- $3.3\text{ V}$、分压电阻误差和 $0.02\text{ V}$ 补偿都会影响最终结果，应使用万用表校准。
- 按键一次采样一次；如果串口连续刷屏，说明按键边沿检测或调用位置有问题。
