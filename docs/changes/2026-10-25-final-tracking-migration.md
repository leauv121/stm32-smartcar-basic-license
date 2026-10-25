# 2026-10-25：最终循迹代码迁移与兼容接口统一

## 现象

`final-work` 烧录后循迹效果不符合原工程当前调参版本。主运行路径使用了重新抽取的 `LineFollowControl_Update()`，没有完整迁移原工程工作区中的最新循迹修改。

## 证据与根因

1. 原工程 `D:\dev\stm32_projects\timer_pwm_4_4` 的外侧数字传感器以高电平表示黑线；新框架曾按低电平有效读取，极性相反。
2. 原工程当前救线参数为外轮 `4500`、内轮 `1400`；新控制器曾令内轮为 `0`。
3. 原工程当前 PID 微分项使用一阶低通滤波；新控制器曾直接使用相邻误差差值。
4. 从救线返回 PID 时，新控制器没有同步当前误差，可能产生一次微分突跳。
5. `Core/Src/car_control.c` 仍编译在工程中，但曾保留第二套旧 PID 和救线参数，形成两套行为。

根因是最初 Git 克隆只复制了原仓库已经提交的内容，没有包含原工程工作区中的最后调参修改；随后新框架的硬件极性转换和兼容接口也没有与参考实现统一。

## 修改位置

- `Core/Src/app/app_runtime.c`：外侧数字传感器恢复为高电平有效。
- `Core/Src/control/line_follow_control.c`：迁移 `4500/1400` 弱救线、微分低通和救线恢复同步。
- `Core/Inc/config/car_parameters.h`：集中保存救线速度参数。
- `Core/Src/services/driver_algorithms.c`：增加外侧数字电平到“检测到黑线”的明确转换。
- `Core/Src/car_control.c`：旧兼容接口统一委托给 `LineFollowControl_Update()`，不再维护第二套 PID 状态。
- `tests/native/`：增加外侧极性、弱救线、微分滤波、模式恢复和兼容接口回归测试。

## 验证结果

- 修改前的回归测试准确出现以下失败：内轮为 `0`、未滤波 PID 输出为 `300`、救线恢复输出为 `600`，兼容接口仍使用旧行为。
- 修复后 Windows GCC 主机测试全部通过。
- Keil ARM Compiler 5.06 update 7 全量 Rebuild：`0 Error(s), 0 Warning(s)`。
- 已生成 `MDK-ARM/timer_pwm/timer_pwm.hex`。
- 所有新增和修改的 C、H 文件均按 UTF-8 检查，未发现替换字符。
- Keil 自动重写的 `MDK-ARM/timer_pwm.uvprojx` 未纳入功能提交。

## 未验证事项

- 尚未把本次生成的 HEX 下载到开发板。
- 尚未在实际赛道验证直线、普通弯、急弯、红绿灯和避障的完整组合流程。
- 编译和主机测试不能替代实车验证；烧录后应先低速架空检查左右轮方向，再进行赛道测试。
