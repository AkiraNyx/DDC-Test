# DDC-Test

[**中文版本 (Chinese README)**](README_CN.md)

A lightweight command-line tool to test DDC/CI availability and control monitor brightness/contrast via the DDC/CI protocol.

**Author:** [AkiraNyx](https://github.com/AkiraNyx)

## Features

- Enumerate all connected monitors and detect DDC/CI support
- Read current brightness and contrast values
- Set brightness for all DDC/CI-capable monitors
- Show VCP feature details (low-level DDC/CI)
- Automatic brightness range mapping (DDC/CI raw values <-> 0-100%)
- Bilingual output: English (default) and Simplified Chinese (`--lang cn`)

## Usage

```
ddc_test                    Enumerate monitors and show DDC/CI info
ddc_test --get              Show current brightness of all monitors
ddc_test --set <N>          Set all monitors brightness to N% (0-100)
ddc_test --lang cn          Use Simplified Chinese output
ddc_test --lang cn --set 50 Combine language with other commands
ddc_test --help             Show help
```

### Examples

```
> ddc_test

========================================
  DDC/CI Monitor Test Tool
  Author: AkiraNyx  |  Version: 1.1.0
========================================

--- Monitor #1 ---
  Device: \\.\DISPLAY1
  Area: (0,0)-(2560,1440)
  Primary: Yes
  Physical monitors: 1

  Physical monitor #0:
    Description: Generic PnP Monitor
    [OK] DDC/CI Brightness available
    Range: 0 - 100
    Current: 70 (raw DDC/CI value)
    Current: 70% (mapped)
    Mapping: 1:1 (no conversion needed)
    [OK] DDC/CI Contrast available
    Range: 0 - 100, Current: 50

> ddc_test --set 50
Setting all DDC/CI monitors to 50%...
  Monitor #1.0: brightness set to 50% (DDC: 50, range 0-100)
Done, processed 1 monitor(s)
```

## Download

Prebuilt binaries are available in [Releases](../../releases).

## Build

```cmd
cl /EHsc /O2 ddc_test.cpp /link dxva2.lib user32.lib
```

## Requirements

- Windows 10/11 x64
- Monitor with DDC/CI support (check your monitor's OSD menu for DDC/CI toggle)
- HDMI, DisplayPort, or DVI connection (VGA may not support DDC/CI)

## How it works

Uses the Windows [Monitor Configuration API](https://learn.microsoft.com/en-us/windows/win32/monitor/monitor-configuration):
- `GetPhysicalMonitorsFromHMONITOR` - enumerate physical monitors
- `GetMonitorBrightness` / `SetMonitorBrightness` - DDC/CI brightness (VCP 0x10)
- `GetMonitorContrast` - DDC/CI contrast (VCP 0x12)
- `GetVCPFeatureAndVCPFeatureReply` - low-level VCP access

## License

MIT License
