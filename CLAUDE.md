# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Itzal v1 is a Bluetooth numpad keyboard firmware built on ZMK (Zephyr Mechanical Keyboard) framework targeting the Nice!Nano v2 microcontroller (nRF52840).

## Build Commands

All commands run from `Firmware/ZMK/` directory:

```bash
# Initialize workspace (first time only)
west init -l itzal_v1
west update
west zephyr-export

# Build firmware
west build -s zmk/app -b nice_nano@2 -- \
  -DSHIELD=itzal_v1 \
  -DZMK_CONFIG="$(pwd)/itzal_v1/boards/shields/itzal_v1" \
  -DZEPHYR_EXTRA_MODULES="$(pwd)/itzal_v1;$(pwd)/itzal_v1/modules/battery_gauge" \
  -DBOARD_ROOT="$(pwd)/zmk/app"

# Output: build/zephyr/zmk.uf2
```

CI builds via GitHub Actions on push/PR to `Firmware/ZMK/itzal_v1/**`. Download artifacts from Actions tab.

## Architecture

```
Firmware/ZMK/itzal_v1/
├── boards/shields/itzal_v1/    # Shield definition
│   ├── itzal_v1.keymap         # Keybindings (2 layers)
│   ├── itzal_v1.overlay        # Matrix definition, behaviors, physical layout
│   ├── itzal_v1.conf           # Kconfig (sleep, BT, RGB, battery gauge)
│   └── boards/nice_nano.overlay # Board-specific (SPI3 for WS2812 LEDs)
├── modules/battery_gauge/      # Custom Zephyr module
│   ├── src/battery_gauge.c     # Battery display on RGB LEDs
│   ├── src/behavior_battery_gauge.c  # ZMK behavior binding
│   └── dts/bindings/           # Device tree bindings
└── west.yml                    # ZMK dependency manifest
```

## Hardware Configuration

- **Matrix**: 4 columns x 5 rows (col2row diodes)
- **Rows**: GPIO 2, 9, 6, 8, 7 (active low, pull-down)
- **Columns**: GPIO 15, 14, 18, 5 (active high)
- **RGB**: WS2812 via SPI3, chain-length configurable in keymap
- **Encoder**: EC11 support present but disabled (enable in .conf and .overlay)

## Keymap Layers

- **Layer 0**: Standard numpad (numlock/layer-tap, /, *, -, +, 0-9, ., enter)
- **Layer 1**: Functions (RGB toggle, battery gauge, ext power, BT profiles, bootloader)

## Custom Battery Gauge Module

Displays battery level on RGB LEDs with color gradient (green=full to red=low). Triggered by `&bat_gauge` behavior in keymap. Auto-off after configurable timeout.

Key Kconfig options in `modules/battery_gauge/Kconfig`:
- `CONFIG_BATTERY_GAUGE_LED_COUNT` - Number of LEDs (default 5)
- `CONFIG_BATTERY_GAUGE_DISPLAY_MS` - Display timeout (default 3000ms)
- `CONFIG_BATTERY_GAUGE_BRIGHTNESS` - LED brightness 0-255

## Key Files to Edit

| Task | File |
|------|------|
| Change key bindings | `itzal_v1.keymap` |
| Modify matrix/GPIO pins | `itzal_v1.overlay` |
| Enable/disable features | `itzal_v1.conf` |
| RGB LED chain length | `itzal_v1.keymap` (led_strip node) |
| Board-specific hardware | `boards/nice_nano.overlay` |
