# 实验 10.4：五通道 ADC + DMA + OLED

## 依据

- 指导书第 12 页：多通道扫描采用连续转换，关闭间断模式；DMA 使用外设到存储器、存储器地址自增、半字对齐和循环模式。
- 指导书第 13 页：先调用 `HAL_ADC_Start()`，再调用 `HAL_ADC_Start_DMA()`，主循环按固定周期读取 DMA 数组。
- 实验 10.4：同时采集电池电压和四路 TCRT5000 AO 电压，并显示在 OLED。

## 本次修改

- ADC1 的五个 Rank 依次设为 IN3、IN6、IN7、IN8、IN9，采样时间均为 239.5 个周期。
- 新增 DMA1 Channel1，配置为循环、外设到内存、内存自增、半字对齐。
- OLED 每 100 ms 显示电池与四路传感器电压，并显示 ADC 启动、DMA 和运行期错误。
- 颜色传感器初始化、读取、日志与控制由 `COLOR_SENSOR_ENABLE=0` 静默，原代码保留。
- ADC 实验模式下禁止姿态信息覆盖 OLED；MPU 姿态采集和串口报告仍保留。
- 新增 PB1/ADC1_IN9，作为第四路 TCRT5000 AO 输入。

## 验证

- Keil ARMCC Rebuild：0 Error(s), 0 Warning(s)。
- `Core/Src/main.c` 保持 UTF-8 BOM，中文注释可由当前编辑器正确识别。
- 尚未代替实物完成下载与接线验证；上板前需确认第四路 AO 已连接 PB1。
