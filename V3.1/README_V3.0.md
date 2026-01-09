# Advanced Irrigation Controller V3.0 - User Guide

## Overview

This is a comprehensive upgrade to the original V2.2 irrigation controller, adding professional-grade features while maintaining backward compatibility with your existing hardware.

## New Features

### 1. **Full Menu System**
- Intuitive 4-button navigation (Menu, Up, Down, Select)
- Multi-level menu hierarchy
- Live status display with real-time updates
- Easy configuration without code changes

### 2. **Network Connectivity**
- **WiFi Support**: ESP32/ESP8266 integration
- **Ethernet Support**: Traditional wired connection
- Selectable at compile time
- Automatic NTP time synchronization
- Remote monitoring capability (foundation for future web interface)

### 3. **Multiple Watering Modes**

#### Moisture-Only Mode
- Original behavior - water based on soil moisture readings
- Temperature compensation supported
- Immediate response to dry conditions

#### Time-Based Mode
- Schedule watering by day and time
- Set specific start times and durations
- Select which days of the week to water
- Perfect for maintaining lawns or regular feeding schedules

#### Hybrid Mode
- Best of both worlds
- Only operates during scheduled time windows
- Uses moisture sensors to decide when to start/stop
- Prevents overwatering while maintaining schedules

#### Manual Mode
- Complete manual control via menu
- Override all automatic functions
- Useful for testing or special situations

### 4. **Enhanced Safety Features**
- Maximum daily watering limits (prevent flooding)
- Per-zone maximum duration timers
- Freeze protection (automatic shutoff below set temperature)
- Runtime tracking and statistics
- Emergency shutoff capabilities

### 5. **Data Logging**
- Configurable logging intervals
- Serial output of all sensor data
- Watering statistics per zone
- Foundation for SD card or network logging

## Hardware Requirements

### Required Components
- **Arduino Mega 2560** (required for sufficient pins and memory)
- **128x64 OLED Display** (U8g2 compatible, SSD1309)
- **AM2315C Temperature/Humidity Sensor** (I2C)
- **4 Pushbuttons** (Menu, Up, Down, Select)
- **16-channel Relay Board** (supports NC or NO relays)
- **Soil Moisture Sensors** (up to 16, capacitive recommended)
- **Power Supply** (adequate for relays and Arduino)

### Optional Components
- **DS3231 RTC Module** (maintains time during power loss)
- **Ethernet Shield** OR **ESP32/ESP8266 WiFi Module**
- **SD Card Module** (for future data logging enhancement)

### Pin Assignments

```
Display:
  - CS:    Pin 47
  - DC:    Pin 49
  - Reset: Pin 48

Buttons:
  - Menu:   Pin 22
  - Up:     Pin 24
  - Select: Pin 26
  - Down:   Pin 28

Soil Sensors: A0-A15 (Analog pins)

Relays: Pins 2-17 (Digital outputs)
  - Zone 1: Pin 2
  - Zone 2: Pin 3
  - Zone 3: Pin 4
  - ... etc

I2C (Temperature & RTC):
  - SDA: Pin 20
  - SCL: Pin 21
```

## Software Configuration

### Before First Upload

1. **Choose Network Type** (lines 26-27):
```cpp
#define USE_WIFI      // Uncomment for WiFi
// #define USE_ETHERNET  // Uncomment for Ethernet
```

2. **WiFi Credentials** (lines 74-75, if using WiFi):
```cpp
const char* ssid = "YourWiFiSSID";
const char* password = "YourWiFiPassword";
```

3. **Time Zone** (line 83):
```cpp
const int timeZone = -5;  // EST = -5, adjust for your timezone
```

4. **Ethernet MAC** (line 87, if using Ethernet):
```cpp
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
```

### Required Libraries

Install these libraries through Arduino IDE Library Manager:

1. **U8g2** (for OLED display)
2. **AM2315C** (temperature/humidity sensor)
3. **Time** / **TimeLib** (time management)
4. **RTClib** (if using RTC module)
5. **WiFi** (built-in for ESP32) OR **Ethernet** (for Ethernet shield)

## Menu System Guide

### Navigation
- **Menu Button**: Go back / Exit current screen
- **Up/Down Buttons**: Navigate menu items / Adjust values
- **Select Button**: Enter submenu / Confirm selection / Start editing

### Menu Structure

```
STATUS SCREEN (default)
└── MAIN MENU (press Select)
    ├── Zone Config
    │   ├── Select Zone
    │   └── Zone Settings
    │       ├── Enable/Disable
    │       ├── Watering Mode
    │       ├── Low Threshold %
    │       ├── High Threshold %
    │       ├── Calibration
    │       ├── Max Duration
    │       └── Reset Statistics
    │
    ├── Schedules
    │   ├── Select Zone
    │   └── Edit Schedule
    │       ├── Enable/Disable
    │       ├── Start Time
    │       ├── Duration
    │       └── Days of Week
    │
    ├── System Setup
    │   ├── Number of Zones
    │   ├── Temperature Units (F/C)
    │   ├── Relay Type (NC/NO)
    │   ├── Freeze Protection
    │   ├── Max Daily Watering
    │   └── Sensor Read Interval
    │
    ├── Network Setup
    │   ├── Enable/Disable Network
    │   ├── Network Type
    │   ├── NTP Enable/Disable
    │   └── Connection Status
    │
    ├── Time Setup
    │   ├── View Current Time
    │   ├── Time Zone Offset
    │   ├── 12/24 Hour Format
    │   └── Sync with NTP
    │
    ├── Manual Control
    │   └── Turn Zones On/Off
    │
    ├── Winterize
    │   └── Clear all valves
    │
    └── Data Log
        └── View logging status
```

## Initial Setup Procedure

### 1. First Boot
On first boot with blank EEPROM, the system will automatically load default settings:
- 6 zones enabled
- Moisture-only mode
- Default calibration values (125-550)
- 40% low / 70% high thresholds
- All schedules disabled

### 2. Configure Number of Zones
1. Press Select to enter Main Menu
2. Navigate to "System Setup"
3. Press Select
4. Navigate to "Num Zones"
5. Press Select to edit
6. Use Up/Down to set your zone count (1-16)
7. Press Select to confirm
8. Press Menu to return

### 3. Calibrate Soil Sensors
For each zone:
1. Main Menu → Zone Config → Select Zone X → Calibrate
2. **Dry Calibration**: 
   - Remove sensor from soil
   - Let it dry completely
   - Press Select to capture "dry" reading
3. **Wet Calibration**:
   - Submerge sensor in water
   - Press Select to capture "wet" reading
4. Press Menu to save and return

### 4. Set Moisture Thresholds
For each zone:
1. Main Menu → Zone Config → Select Zone X
2. Navigate to "Low %" (turn-on threshold)
3. Press Select, adjust with Up/Down (typically 30-50%)
4. Navigate to "High %" (turn-off threshold)
5. Press Select, adjust with Up/Down (typically 60-80%)

### 5. Choose Watering Mode
For each zone:
1. Main Menu → Zone Config → Select Zone X → Mode
2. Press Select to cycle through modes:
   - Moisture Only
   - Time Only
   - Hybrid
   - Manual

### 6. Configure Schedules (if using Time or Hybrid mode)
1. Main Menu → Schedules → Select Zone X
2. Enable schedule
3. Set start time (hour and minute)
4. Set duration (minutes)
5. Select days of week to water

### 7. Network Setup (optional)
1. Main Menu → Network Setup
2. Enable Network
3. Verify connection status on display
4. Enable NTP for automatic time sync
5. Main Menu → Time Setup → Sync with NTP

## Operating Modes Explained

### Moisture-Only Mode
**How it works:**
- Continuously monitors soil moisture
- Turns on when moisture drops below "Low %" threshold
- Turns off when moisture rises above "High %" threshold
- Temperature compensation adjusts thresholds in hot weather

**Best for:**
- Potted plants
- Container gardens
- Areas with variable water needs
- Drought-sensitive plants

**Configuration needed:**
- Calibrate sensors
- Set Low and High thresholds
- Optionally: temperature adjustment settings

### Time-Based Mode
**How it works:**
- Waters on specific days at specific times
- Runs for exact duration regardless of moisture
- Can water multiple times per day (one schedule per zone)
- Tracks last watering to prevent duplicates

**Best for:**
- Lawns
- Regular feeding schedules
- Areas with timers restrictions
- Consistent watering requirements

**Configuration needed:**
- Set schedule start time
- Set duration in minutes
- Select days of week
- Set maximum duration as safety limit

### Hybrid Mode
**How it works:**
- Combines time-based scheduling with moisture sensing
- Only operates during scheduled time windows
- Uses moisture sensors to decide when to START watering
- Uses moisture sensors to decide when to STOP watering
- Will not water if soil is already moist

**Best for:**
- Gardens with time-of-day restrictions
- Optimizing water usage
- Preventing overwatering
- Variable weather conditions

**Configuration needed:**
- Calibrate sensors
- Set moisture thresholds
- Set schedule time windows
- Select operating days

**Example:** 
- Schedule: 6:00 AM - 7:00 AM, Monday/Wednesday/Friday
- Low threshold: 40%, High threshold: 70%
- Behavior: Between 6-7 AM on M/W/F, if moisture is below 40%, start watering. Stop when moisture reaches 70% or 7:00 AM, whichever comes first.

### Manual Mode
**How it works:**
- All automatic functions disabled
- Control zones manually through menu
- No moisture or time-based control

**Best for:**
- Testing
- Maintenance
- Special situations
- Troubleshooting

## Safety Features

### Maximum Daily Watering
- Default: 180 minutes (3 hours) per zone per day
- Prevents flooding from sensor failures
- Resets at midnight
- Configurable in System Setup

### Maximum Duration
- Per-zone setting (default: 60 minutes)
- Emergency shutoff for single watering session
- Prevents runaway watering

### Freeze Protection
- Monitors temperature sensor
- Disables all watering below set temperature (default: 35°F)
- Prevents damage to frozen pipes and plants
- Can be disabled in System Setup

### Runtime Tracking
- Logs total watering time per zone
- Tracks daily watering minutes
- Shows runtime on status display
- Useful for diagnosing issues

## Temperature Compensation

The system can automatically adjust moisture thresholds during hot weather:

**System Config Settings:**
- **Temp Switch**: Temperature threshold (default: 92°F)
- **Temp Adj Low**: Added to low threshold when hot (default: +6%)
- **Temp Adj High**: Added to high threshold when hot (default: +4%)

**Example:**
- Normal: Water at 40%, stop at 70%
- When temp ≥ 92°F: Water at 46%, stop at 74%
- Provides more water during heat stress

## Network Features

### WiFi Setup
1. Edit code to add your WiFi credentials
2. Upload sketch
3. System will auto-connect on boot
4. Check Network Setup menu for IP address

### Ethernet Setup
1. Ensure #define USE_ETHERNET is uncommented
2. Check MAC address is unique on your network
3. Upload sketch
4. System will request DHCP address

### NTP Time Synchronization
- Automatically syncs time every hour
- Updates RTC module (if installed)
- Configurable time zone offset
- Requires working network connection

### Future Enhancements
The network foundation supports:
- Web interface for monitoring
- Remote control capabilities
- Email/SMS notifications
- Weather service integration
- Cloud data logging

## Troubleshooting

### Display Shows "EEPROM not initialized"
**Solution:** Normal on first boot. System loads defaults automatically.

### "Network Failed"
**Cause:** WiFi credentials wrong or no connection
**Solution:** 
- Check WiFi password in code
- Verify router is accessible
- Check antenna connection (ESP32)
- System operates normally without network

### Zone Won't Turn On
**Check:**
1. Zone enabled in configuration?
2. Correct watering mode selected?
3. Moisture reading correct? (view on status screen)
4. Schedule configured if using Time/Hybrid mode?
5. Daily limit not exceeded?
6. Not in freeze protection mode?

### Moisture Readings Incorrect
**Solution:**
1. Recalibrate sensors (Zone Config → Calibrate)
2. Check sensor wiring
3. Verify sensor is correct type (capacitive recommended)
4. Clean sensor probes

### Time Not Keeping
**Cause:** No RTC installed or not initialized
**Solution:**
- Install DS3231 RTC module
- Enable NTP sync if network available
- Time will reset on power loss without RTC

### Relay Clicking but Valve Not Opening
**Check:**
1. Relay type setting (NC vs NO)
2. Relay board has adequate power
3. Valve solenoid compatible with relay rating
4. Valve wiring correct
5. Test manually in Manual Control mode

## Relay Configuration

### NC (Normally Closed) Relays
- Set `sysConfig.relayOn = 0` (default)
- Valve closes when Arduino loses power (safer)
- Most common configuration

### NO (Normally Open) Relays
- Set `sysConfig.relayOn = 1`
- Valve opens when Arduino loses power
- Use if you want fail-open behavior

**To change:**
Main Menu → System Setup → Relay Type → toggle NC/NO

## Winterization Procedure

**Purpose:** Clear water from valves and pipes before freezing weather

**Procedure:**
1. Main Menu → Winterize
2. Press Select to confirm
3. System will:
   - Open each valve for 5 seconds
   - Close valve
   - Move to next zone
4. Manually blow out lines with compressed air
5. Close main water valve

## Data Logging

**Current Capability:**
- Logs to Serial Monitor
- Configurable interval (default: 60 minutes)
- Records all sensor data and valve states

**Logged Data:**
- Current date/time
- Temperature and humidity
- All zone moisture readings
- Valve on/off status
- Daily watering totals

**To view:**
- Open Arduino Serial Monitor (115200 baud)
- Data prints automatically at log interval

**Future Enhancement:**
- SD card logging support
- Network logging to server
- Export to CSV

## Maintenance Tips

### Weekly
- Check status display for any stuck zones
- Verify moisture readings make sense
- Check for leaking valves

### Monthly
- Review data logs for patterns
- Check total watering time per zone
- Clean soil sensor probes
- Verify temperature sensor accuracy

### Seasonally
- Recalibrate soil sensors
- Adjust thresholds for season
- Update schedules for daylight changes
- Test freeze protection before winter

### Annually
- Replace any failed sensors
- Check all wiring connections
- Clean relay contacts if necessary
- Update RTC battery (CR2032)

## Advanced Customization

### Changing Default Values
Edit `resetToDefaults()` function to change:
- Number of zones
- Default thresholds
- Temperature settings
- Safety limits
- Logging intervals

### Adding More Zones
System supports up to 16 zones (limited by Arduino Mega pins and relays)
- Adjust `sysConfig.zones` in System Setup
- Ensure relay board has enough channels
- Verify power supply adequate

### Display Customization
- Font: Line 191 in code
- Layout: Edit draw functions
- Update rate: Line 425 (currently 500ms)

### Sensor Read Interval
Configurable in System Setup menu
- Default: 5000ms (5 seconds)
- Faster: Better response, more processing
- Slower: Less overhead, smoother operation

## EEPROM Memory Map

Understanding data storage:

```
Address 0-1999:    Zone Configurations (125 bytes × 16)
Address 2000-2199: System Configuration (200 bytes)
Address 2200-4095: Reserved for future use
```

**Each Zone Config Contains:**
- Enabled flag
- Zone name
- Calibration values
- Thresholds
- Watering mode
- Schedule data
- Statistics

**Persistence:**
- Configuration survives power loss
- Schedules are maintained
- Statistics reset daily

## Pin Expansion Options

If you need more than 16 zones, you can:
1. Use I2C GPIO expanders (MCP23017)
2. Cascade relay boards
3. Use shift registers (74HC595)
4. Implement in code with expanded addressing

## Power Considerations

**Arduino Power:**
- 7-12V DC recommended
- Higher voltage for voltage regulators

**Relay Power:**
- Separate power supply recommended
- Match relay coil voltage requirements
- Calculate total current for all relays

**Example:**
- 16 relays × 70mA = 1.12A minimum
- Use 12V 2A supply for safety margin

## Serial Commands (Future Feature)

Currently outputs logging data. Can be enhanced to accept commands:
- Remote zone control
- Configuration changes
- Manual overrides
- Status queries

## Support and Contributions

**Based on original work by:**
- Jack Mamiye & Anna Sweeney (V2.2)

**Enhanced version:**
- Claude/Anthropic (V3.0)

**Suggested Improvements:**
- Web interface
- Mobile app support
- Weather API integration
- Soil salinity monitoring
- Rain sensor input
- Flow rate monitoring
- SD card logging
- Email/SMS alerts

## License

Use freely for personal and educational purposes. Credit original authors when sharing or modifying.

## Version History

**V3.0 (January 2026)**
- Complete menu system
- WiFi/Ethernet support
- NTP time sync
- Multiple watering modes
- Enhanced safety features
- Data logging
- RTC support

**V2.2 (July 2024)**
- Original moisture-based control
- Temperature compensation
- OLED display support
- EEPROM configuration

---

**End of User Guide**

For questions, issues, or enhancement ideas, document in your project notes or create issues in your repository.
