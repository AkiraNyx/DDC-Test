# DDC-Test

一个轻量级命令行工具，用于测试 DDC/CI 可用性并通过 DDC/CI 协议控制显示器亮度/对比度。

**作者:** [AkiraNyx](https://github.com/AkiraNyx)

## 功能

- 枚举所有连接的显示器并检测 DDC/CI 支持
- 读取当前亮度和对比度值
- 设置所有支持 DDC/CI 的显示器的亮度
- 显示 VCP 特性详情（低级 DDC/CI）
- 自动亮度范围映射（DDC/CI 原始值 <-> 0-100%）
- 双语输出：英语（默认）和简体中文（`--lang cn`）

## 用法

```
ddc_test                    枚举所有显示器并显示 DDC/CI 信息
ddc_test --get              显示所有显示器当前亮度
ddc_test --set <N>          设置所有显示器亮度为 N% (0-100)
ddc_test --lang cn          使用简体中文输出
ddc_test --lang cn --set 50 组合使用语言和其他命令
ddc_test --help             显示帮助
```

### 示例

```
> ddc_test --lang cn

========================================
  DDC/CI 显示器测试工具
  作者: AkiraNyx  |  版本: 1.0.0
========================================

--- 显示器 #1 ---
  设备名称: \\.\DISPLAY1
  区域: (0,0)-(2560,1440)
  主显示器: 是
  物理显示器数量: 1

  物理显示器 #0:
    描述: Generic PnP Monitor
    [OK] DDC/CI 亮度控制可用
    亮度范围: 0 - 100
    当前亮度: 70 (DDC/CI 原始值)
    当前亮度: 70% (映射后)
    映射: 1:1 (无需转换)
    [OK] DDC/CI 对比度控制可用
    对比度范围: 0 - 100, 当前: 50

> ddc_test --lang cn --set 50
设置所有 DDC/CI 显示器亮度到 50%...
  显示器 #1.0: 亮度设置为 50% (DDC值: 50, 范围 0-100)
完成，共处理 1 台显示器
```

## 下载

预编译的二进制文件可在 [Releases](../../releases) 页面下载。

## 编译

```cmd
cl /EHsc /O2 ddc_test.cpp /link dxva2.lib user32.lib
```

## 系统要求

- Windows 10/11 x64
- 显示器需支持 DDC/CI（在显示器 OSD 菜单中确认 DDC/CI 已开启）
- HDMI、DisplayPort 或 DVI 连接（VGA 可能不支持 DDC/CI）

## 工作原理

使用 Windows [Monitor Configuration API](https://learn.microsoft.com/en-us/windows/win32/monitor/monitor-configuration)：
- `GetPhysicalMonitorsFromHMONITOR` - 枚举物理显示器
- `GetMonitorBrightness` / `SetMonitorBrightness` - DDC/CI 亮度控制 (VCP 0x10)
- `GetMonitorContrast` - DDC/CI 对比度控制 (VCP 0x12)
- `GetVCPFeatureAndVCPFeatureReply` - 低级 VCP 访问

## 许可证

MIT License
