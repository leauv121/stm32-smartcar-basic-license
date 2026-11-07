# 最终大作业工程框架

## 目标

在独立克隆与独立分支中建立分层小车工程，复用现有循迹、姿态、电机和传感器代码，并增加集中故障管理、OLED 故障显示和安全停车。

## 当前证据

- 工作目录：独立工程目录。
- 开发方式：在独立开发分支中完成，避免影响旧版基线工程。
- 原工程只作为克隆来源，不在本次工作中修改。
- 已建立配置、控制、服务、应用和硬件适配分层。
- 已接入循迹 ADC、MPU6050 DMP、轮速、颜色、超声波、OLED、左右灯、USER 按键和 USART3。
- 电池监测没有可用的独立 ADC 通道，当前在自检中明确跳过，不伪造硬件检测结果。
- 普通任务和四种高级模式已具备状态机；A/B/C/D 的物理识别规则仍需按实际赛道标定。

## 设计依据

- 配置、控制、服务、应用与硬件适配分层。
- 任务状态机只产生运动请求，安全仲裁后才能写入电机输出。
- 重大修改通过本目录的变更记录保留问题、证据与验证边界。

## 验证记录

- 主机测试：17 个测试程序全部通过，编译参数包含 `-std=c99 -Wall -Wextra -Werror`。
- 架构边界：`tests/check_source_boundaries.ps1` 通过。
- UTF-8：`tools/check_utf8.ps1` 检查 124 个文件通过，无替换字符。
- Git 差异：`git diff --check` 通过。
- Keil 全量 Rebuild：ARM Compiler 5.06 update 7，`0 Error(s), 0 Warning(s)`。
- 固件：`MDK-ARM/timer_pwm/timer_pwm.hex`，2026-10-23 10:35:13 生成，大小 123552 字节。

执行命令：

```powershell
powershell -ExecutionPolicy Bypass -File tests\run_native_tests.ps1
powershell -ExecutionPolicy Bypass -File tests\check_source_boundaries.ps1
powershell -ExecutionPolicy Bypass -File tools\check_utf8.ps1
# 打开 MDK-ARM/timer_pwm.uvprojx，选择 timer_pwm 并执行 Rebuild
```

## 尚未验证

- 尚未烧录本分支固件。
- 尚未在实车上验证普通驾照和高级驾照路线。
- 颜色、超声波、轮速、路点识别和 PID 参数仍需实车标定。
- 电池检测必须先增加独立 ADC 分压通道，才能启用并验证。
