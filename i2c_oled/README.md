# i2c_oled

ESP32-C3 + SH1106 + U8g2 的 OLED 菜单工程。

## Layout

- `main/wououi_128_64.cpp`: 当前维护中的主程序
- `main/CMakeLists.txt`: 仅注册当前主程序源码
- `components/u8g2/`: vendored U8g2 组件
- `tools/resize_main_menu_icons.py`: 图标位图处理脚本
- `reference/sketch.ino`: 原始 Arduino 参考文件

## Hardware

- `GPIO10` -> OLED `SDA`
- `GPIO11` -> OLED `SCL`
- `GPIO6` -> encoder `A`
- `GPIO7` -> encoder `B`
- `GPIO2` -> `PUSH`
- `GPIO8` -> `BACK`
- `GPIO9` -> `OK` / `BOOT`

## Notes

- I2C runs at `800 kHz`.
- OLED address is auto-probed at `0x3C` and `0x3D`.
- `Bar Style` in Settings switches the right-side list indicator between two styles.
- USB HID is not enabled in this ESP32-C3 build.

## Build

Use the standard ESP-IDF flow:

```bash
idf.py build flash monitor
```
