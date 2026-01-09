# Soil Moisture Irrigation Controller - Complete User Guide

## 📋 Version Overview

| Version | Hardware | Interface | Configuration | RAM Usage | Recommended For |
|---------|----------|-----------|---------------|-----------|-----------------|
| **V2.2 CONFIG** | Mega 2560 + OLED | 4-Button Menu | Full menu system | ~2KB | Simple standalone systems |
| **V3.0 FIXED** | Mega 2560 + OLED + Ethernet | 4-Button Menu | Full menu system | ~4KB | Systems needing NTP/scheduling |
| **V3.1 CONFIG** | Mega 2560 + OLED + Ethernet | **Web Browser** + OLED | Full web interface | ~3KB | Remote monitoring & config |
| **V4.0 FIXED** | GIGA R1 WiFi + Touchscreen | Touch GUI | Full touchscreen | ~10KB | Premium touchscreen systems |

---

## 🚀 Quick Start

### Which Version Should I Use?

```
Do you have a touchscreen?
├─ YES → Use V4.0 (GIGA R1 WiFi required)
└─ NO → Do you want web browser access?
         ├─ YES → Use V3.1 CONFIG (Ethernet shield required)
         └─ NO → Do you need scheduling/NTP time?
                  ├─ YES → Use V3.0 (Ethernet optional)
                  └─ NO → Use V2.2 CONFIG (simplest)
```

---

## 🔧 Hardware Requirements

### All Versions Need:
- Capacitive soil moisture sensors (1 per zone)
- Relay module (1 channel per zone, recommend 5V trigger)
- AM2315C temperature/humidity sensor (I2C)
- 12V/24V solenoid valves (1 per zone)
- Power supply appropriate for your valves

### Version-Specific Hardware:

| Component | V2.2 | V3.0 | V3.1 | V4.0 |
|-----------|------|------|------|------|
| Arduino Mega 2560 | ✅ | ✅ | ✅ | ❌ |
| Arduino GIGA R1 WiFi | ❌ | ❌ | ❌ | ✅ |
| 128x64 OLED (SSD1309) | ✅ | ✅ | ✅ | ❌ |
| GIGA Display Shield | ❌ | ❌ | ❌ | ✅ |
| Ethernet Shield (W5100/W5500) | ❌ | Optional | ✅ | ❌ |
| 4 Pushbuttons | ✅ | ✅ | Optional | ❌ |
| DS3231 RTC Module | Optional | Optional | Optional | ❌ |

### Pin Connections (Mega 2560 versions):

```
OLED Display (SPI):
  CS    → Pin 47
  DC    → Pin 49  
  RESET → Pin 48
  (Plus SPI pins: MOSI=51, SCK=52)

Buttons:
  MENU   → Pin 22 (with internal pullup)
  UP     → Pin 24
  SELECT → Pin 26
  DOWN   → Pin 28

Soil Sensors:
  Zone 1  → A0
  Zone 2  → A1
  ...
  Zone 16 → A15

Relay Outputs:
  Zone 1  → Pin 2
  Zone 2  → Pin 3
  ...
  Zone 16 → Pin 17

AM2315C (I2C):
  SDA → Pin 20
  SCL → Pin 21
```

---

## 📱 V2.2 CONFIG - Button Menu Guide

### Menu Navigation

| Button | Function |
|--------|----------|
| **MENU** | Enter menu / Go back / Cancel |
| **UP** | Move cursor up / Increase value |
| **DOWN** | Move cursor down / Decrease value |
| **SELECT** | Enter submenu / Confirm selection |

### Menu Structure

```
[Normal Operation Screen]
    │
    └─► MENU button
        │
        ├─► Num Zones ──────► Set 1-16 zones
        ├─► Temp Unit ──────► °C or °F
        ├─► Relay Type ─────► NC or NO
        ├─► Zone Config ────► Select Zone
        │       │               ├─► Set Dry Cal (put sensor in dry soil, press SELECT)
        │       │               ├─► Set Wet Cal (put sensor in wet soil, press SELECT)
        │       │               ├─► Low Thresh (turn ON below this %)
        │       │               └─► High Thresh (turn OFF above this %)
        ├─► Temp Switch ────► Temperature threshold for adjustment
        ├─► Temp Adjust ────► % to add when hot
        ├─► Winterize ──────► Pulse all valves for 5 sec each
        └─► Save & Exit ────► Save all settings to EEPROM
```

### Calibrating Sensors

1. Go to **Zone Config** → Select zone
2. Place sensor in **completely dry soil** (or air)
3. Select **Set Dry Cal** → Press SELECT
4. Place sensor in **saturated wet soil**
5. Select **Set Wet Cal** → Press SELECT
6. Return to main menu → **Save & Exit**

---

## 🌐 V3.1 CONFIG - Web Interface Guide

### Accessing the Web Interface

1. Connect Ethernet cable to your network
2. Power on the controller
3. Note the IP address shown on the OLED display
4. Open a web browser and go to `http://[IP_ADDRESS]`

### Web Interface Tabs

#### 🌱 Zones Tab
- View real-time moisture levels for all zones
- See which zones are currently watering
- Click **ON/OFF** button to manually toggle any zone

#### ⚙️ Zone Config Tab
- Select a zone from the dropdown
- **Name**: Custom name (up to 9 characters)
- **Enabled**: Enable/disable this zone
- **Mode**: 
  - *Moisture* - Water based on soil moisture only
  - *Time* - Water on schedule only
  - *Hybrid* - Schedule + moisture override
  - *Manual* - Only manual control
- **Low Thresh**: Start watering below this %
- **High Thresh**: Stop watering above this %
- **Dry/Wet Cal**: Calibration values
  - Click **Set Now** to capture current raw ADC reading
- Click **Save Zone** to store changes

#### 🔧 System Tab
- **Active Zones**: Number of zones (1-16)
- **Temp Display**: Fahrenheit or Celsius
- **Relay Type**: 
  - *NC* - Normally Closed (LOW signal = valve ON)
  - *NO* - Normally Open (HIGH signal = valve ON)
- **Freeze Protect**: Disable watering below set temperature
- **Temp Switch**: Temperature above which to adjust thresholds
- **Low/Hi Adj**: Percentage to add to thresholds when hot
- **NTP Sync**: Enable automatic time synchronization
- **Timezone**: Your UTC offset (e.g., -5 for EST)
- **Winterize**: Pulses all valves to clear water before freeze

---

## 🖥️ V3.0 - Menu System

V3.0 uses the same 4-button menu system as V2.2 but adds:

- **Schedule Config**: Set watering times per zone
- **Network Config**: Enable/disable network, NTP
- **Time Config**: View/sync time, set timezone
- **Manual Control**: Toggle zones from menu
- **Data Logging**: View watering history

---

## 📱 V4.0 - Touchscreen Interface

V4.0 features a full graphical touchscreen interface:

- **Home Screen**: Zone status overview with moisture bars
- **Zone Detail**: Tap any zone for full controls
- **Settings**: Swipe to access system configuration
- **Calibration**: On-screen calibration wizard
- **Scheduling**: Visual schedule editor

---

## ⚡ Relay Type Explained

### NC (Normally Closed) - Default
- Relay contact is **closed** when not energized
- Arduino sends **LOW** to turn valve **ON**
- **Fail-safe**: If Arduino loses power, valves close
- Most common for irrigation

### NO (Normally Open)
- Relay contact is **open** when not energized  
- Arduino sends **HIGH** to turn valve **ON**
- If Arduino loses power, valves open (could flood!)
- Used with specific valve types

**Check your relay module documentation!** Most relay modules have both NO and NC terminals - wire to the appropriate one.

---

## 🌡️ Temperature Adjustment Feature

When temperature exceeds **Temp Switch** threshold:
- Low threshold increases by **Low Adj** %
- High threshold increases by **Hi Adj** %

**Example:**
- Normal: Water when soil drops below 40%, stop at 70%
- Temp Switch = 92°F, Low Adj = 6%, Hi Adj = 4%
- When temp ≥ 92°F: Water below 46%, stop at 74%

This provides more water during hot weather automatically.

---

## ❄️ Winterize Function

The winterize function helps prevent freeze damage:

1. Opens each valve for 5 seconds
2. Allows residual water to drain
3. Closes valve and moves to next zone

**Run this before freezing weather!** It won't blow out lines (you need an air compressor for that) but helps clear standing water from valves.

---

## 🔍 Troubleshooting

### "Moisture always reads 0% or 100%"
- Check sensor wiring to analog pin
- Verify calibration values (Dry Cal should be < Wet Cal)
- Try recalibrating with known dry/wet conditions

### "Relay clicks but valve doesn't open"
- Check valve wiring and power supply
- Verify relay type setting (NC vs NO)
- Test valve manually with direct power

### "Web interface won't load"
- Verify Ethernet cable connection
- Check IP address on OLED display
- Ensure computer is on same network
- Try pinging the IP address

### "Settings don't save"
- Make sure to press **Save** button
- EEPROM may be worn out after ~100,000 writes
- Try factory reset and reconfigure

### "NTP time won't sync"
- Check internet connectivity
- Verify NTP is enabled in settings
- Firewall may block UDP port 123

### "Out of memory error during compile"
- Use the LEAN/CONFIG versions for Mega 2560
- Original V3.1 uses too much RAM - use V3.1 CONFIG instead

---

## 📊 Memory Usage Comparison

| Version | Flash | RAM | Status |
|---------|-------|-----|--------|
| V2.2 CONFIG | ~25KB | ~2KB | ✅ Good |
| V3.0 FIXED | ~45KB | ~4KB | ✅ Good |
| V3.1 FIXED (original) | ~83KB | ~33KB | ❌ Too big for Mega! |
| V3.1 CONFIG | ~35KB | ~3KB | ✅ Good |
| V4.0 FIXED | ~120KB | ~15KB | ✅ Good (GIGA has 1MB RAM) |

---

## 📚 Library Dependencies

### Required Libraries (Install via Arduino Library Manager)

| Library | Version | Used By |
|---------|---------|---------|
| U8g2 | Latest | V2.2, V3.0, V3.1 |
| AM2315C | Latest | All versions |
| ArduinoJson | **6.21.x** (NOT v7!) | V3.1, V4.0 |
| RTClib | Latest | V3.0, V3.1, V4.0 (optional) |
| TimeLib | Latest | V3.0, V3.1, V4.0 |
| Ethernet | Built-in | V3.0, V3.1 |

### V4.0 Additional Libraries
- Arduino_GigaDisplay_GFX
- Arduino_GigaDisplayTouch

**⚠️ ArduinoJson v7 has breaking changes! Use v6.21.x specifically.**

---

## �icing Factory Reset

If you need to reset to defaults:

### V2.2/V3.0/V3.1 (Mega)
The controller auto-detects corrupted EEPROM and loads defaults. To force reset:
1. Upload a blank sketch that writes 0xFF to EEPROM addresses 0-100
2. Re-upload the irrigation controller sketch

### V4.0 (GIGA)
Similar process, or use the touchscreen reset option if available.

---

## 📝 Changelog

### January 2026 - Bug Fix Release
- **V2.2**: Added full menu system (was completely missing!)
- **V2.2**: Fixed critical EEPROM.get() bug - config was never loaded
- **V2.2**: Fixed `=` vs `==` bugs in conditionals
- **V3.0**: Fixed min() template ambiguity
- **V3.1**: Complete rewrite for Arduino Mega compatibility
- **V3.1**: Created memory-optimized CONFIG version (~3KB RAM vs 33KB)
- **V3.1**: Added full web-based zone configuration
- **V4.0**: Fixed undefined variable bugs, touch library types

---

## 📄 Files Included

| Filename | Description |
|----------|-------------|
| `sketch_IrrigationController_V2_2_CONFIG.ino` | ⭐ V2.2 with full 4-button menu system |
| `sketch_IrrigationController_V3_0_FIXED.ino` | V3.0 with menu + optional Ethernet/NTP |
| `sketch_IrrigationController_V3_1_CONFIG.ino` | ⭐ V3.1 memory-optimized with full web config |
| `sketch_IrrigationController_V4_0_GIGA_FIXED.ino` | V4.0 for GIGA R1 WiFi + Touchscreen |

**⭐ = Recommended versions for most users**

### Documentation Files
| Filename | Description |
|----------|-------------|
| `IRRIGATION_CONTROLLER_USER_GUIDE.md` | This comprehensive user guide |
| `ALL_VERSIONS_FIX_SUMMARY.md` | Technical details of all bugs fixed |
| `V4_0_FIX_SUMMARY.md` | Detailed V4.0 bug analysis |

---

*Documentation updated January 2026*
