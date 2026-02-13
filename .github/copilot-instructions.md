# Copilot Instructions — flower_CC2340R5_RGE

## Project Overview

Zigbee End Device (ZED) firmware for the DIYRuZ Flower sensor board based on TI CC2340R5 (VQFN RGE package). Measures soil moisture, illuminance, temperature, and battery voltage, reporting via Zigbee HA profile clusters. Supports OTA firmware updates via MCUBoot.

## Architecture

### Two-Project Structure

- **`mcuboot_flower_CC2340R5_RGE/`** — MCUBoot bootloader (starts at `0x00000000`, 16 KB). Validates and boots the application image.
- **`flower_ota_onchip_CC2340R5_RGE/`** — Main application firmware (primary slot at `0x00004000`, 250 KB). Contains all Zigbee logic, sensor drivers, and OTA client.

### Dual-Endpoint Zigbee Design

- **Endpoint 10** — Sensor/switch endpoint. Hosts input clusters: Basic, Identify, On/Off Switch Config, Power Config, Soil Moisture (`0x0408`), Illuminance Measurement, Temperature Measurement.
- **Endpoint 11** — OTA upgrade endpoint (isolated from sensor logic).

### Global Device Context

All device state lives in a single `g_dev_ctx` global of type `on_off_switch_ota_ctx_t`, which aggregates ZCL attribute structs for every cluster. Do not introduce additional globals for device state.

### Event-Driven Flow

The application is event-driven via the ZBOSS stack:
- `zboss_signal_handler()` manages network state transitions (join → steering → operational).
- `ZB_SCHEDULE_APP_ALARM()` schedules timed callbacks (sensor polling, button debounce).
- `timer_update()` runs every 10 seconds for periodic sensor reads and attribute reporting.

## Sensor Drivers

Each sensor driver follows the same pattern: `init() → write(mode) → read() → powerDown()`. I2C handles are opened/closed per transaction (`zclSampleSw_I2cInit()` / `zclSampleSw_I2cClose()`) to minimize memory.

Sensors are selected at compile time via `#ifdef`:

| Driver | Sensor | Cluster | I2C |
|--------|--------|---------|-----|
| `bh1750.c/h` | BH1750 ambient light | Illuminance | Yes |
| `opt3001.c/h` | OPT3001 optical | Illuminance | Yes |
| `tmp102.c/h` | TMP102 temperature | Temperature | Yes |
| `bme280i2c.c/h` | BME280 temp/humidity/pressure | Temperature | Yes |

**Soil moisture** uses ADC + PWM (capacitive sensing), not I2C. The raw ADC value is converted to 0–100% using voltage-compensated formulas (`AIR_COMPENSATION_FORMULA`, `WATER_COMPENSATION_FORMULA`).

All ZCL attribute values are 16-bit fixed-point (multiply physical value × 100).

## OTA Update Flow

1. OTA client queries coordinator every 12 × 60 minutes.
2. `ota_client_interface.c` handles: image validation → flash erase → block writes → integrity check → mark ready.
3. MCUBoot performs image swap on next boot.
4. OTA identity: manufacturer `0xBEBE`, image type `0x2340`.

## Build System

This is a **Code Composer Studio (CCS) 12.8** project using **TI SimpleLink Low Power F3 SDK 8.40.02.01**.

- Hardware configuration is in `.syscfg` files (TI SysConfig), which generate driver init code.
- Linker scripts: `lpf3_zigbee_freertos.cmd` (app), `mcuboot_cc2340r5.cmd` (bootloader).
- RTOS: FreeRTOS.
- Post-build step signs the application image using `imgtool`.

### Flashing

Use **TI UniFlash 9.2.0** to flash both images:
1. `mcuboot_flower_CC2340R5_RGE.hex` (bootloader)
2. `flower_ota_onchip_CC2340R5_RGE_ota.bin` (signed application)

### Version Management

`ver.py` auto-generates `version.c` with a build timestamp in `"DD.MM.YY HH:MM"` format. Do not edit `version.c` manually.

## Zigbee2MQTT Converters

- **`converters/DIYRuZ_FW2340R5.js`** — Current full converter (battery, soil moisture, illuminance, temperature, OTA).
- **`flower_ota_onchip_CC2340R5_RGE/DIYRuZ_SW2340R5.js`** — Simpler switch-only variant.

Converters bind endpoint 10 to the coordinator for Power Config, Soil Moisture, Illuminance, and Temperature clusters.

## Key Conventions

- **Function prefixes:** `dl_` for device logic, `zb_` for ZBOSS stack calls, `CONFIG_` for SysConfig-generated defines.
- **Cluster declarations** use ZBOSS X-macro pattern: `ZB_ZCL_DECLARE_*`.
- **Attribute reporting** uses min/max intervals (10–1800 s) with per-attribute deltas to prevent network flooding (soil: 300, illuminance: 2, temperature: 10).
- **Button handling:** Short press (<1 s) sends a report; long press (>4 s) triggers network join/leave.
- **GPIO pin mapping** is defined in the `.syscfg` file — Green LED: DIO3, Red LED: DIO4, Button: DIO20, ADC: DIO21, PWM: DIO24, I2C: DIO6/DIO8.

## Language

README and code comments are in Russian. Maintain this convention when adding user-facing strings or documentation.
