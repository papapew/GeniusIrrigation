# Quick Start Guide - Irrigation Controller V3.0

## 🚀 Fast Track Setup (15 minutes)

### Step 1: Hardware Connections (5 min)

```
1. Arduino Mega 2560
   ├── Display (SPI): CS→47, DC→49, Reset→48
   ├── Buttons: Menu→22, Up→24, Select→26, Down→28
   ├── I2C: SDA→20, SCL→21
   ├── Soil Sensors: A0-A15
   └── Relays: Digital Pins 2-17

2. Power Connections
   ├── Arduino: 7-12V DC
   └── Relays: Separate 12V supply recommended
```

### Step 2: Software Upload (3 min)

1. **Install Libraries** (Arduino IDE → Library Manager):
   - U8g2
   - AM2315C
   - TimeLib
   - RTClib (if using RTC)
   - WiFi or Ethernet (depending on choice)

2. **Edit Configuration** (sketch lines to modify):
   ```cpp
   Line 26-27: Choose WiFi or Ethernet
   Line 74-75: WiFi credentials (if using WiFi)
   Line 83: Time zone (-5 for EST, -8 for PST, etc.)
   ```

3. **Upload Sketch** to Arduino Mega

### Step 3: Initial Configuration (7 min)

#### A. Set Number of Zones (1 min)
```
Status Screen → Press SELECT
Main Menu → System Setup → SELECT
Num Zones → SELECT → UP/DOWN to set → SELECT
```

#### B. Quick Zone Setup (2 min per zone)
```
Main Menu → Zone Config → SELECT
Select your zone → SELECT

Quick Settings:
├── Enable: YES
├── Mode: Moisture Only (for now)
├── Low %: 40
└── High %: 70
```

#### C. Calibrate Sensors (3 min per zone) - IMPORTANT!
```
Zone Settings → Calibrate → SELECT

1. Dry Reading:
   - Remove sensor from soil
   - Wipe completely dry
   - Press SELECT

2. Wet Reading:
   - Submerge in water
   - Press SELECT

Done! Repeat for each zone.
```

### Step 4: Test & Go! (2 min)

```
1. Return to Status Screen (press MENU repeatedly)
2. Watch moisture readings update
3. Verify zones turn on when moisture drops below Low %
4. Verify zones turn off when moisture rises above High %
```

---

## 🎯 Common Quick Setups

### Setup A: Simple Moisture Control (Beginner)
**Perfect for: Container gardens, potted plants**

```
Mode: Moisture Only
Low %: 35-45 (turn on when this dry)
High %: 65-75 (turn off when this wet)
Max Duration: 30 minutes
```

### Setup B: Scheduled Watering (Intermediate)
**Perfect for: Lawns, scheduled feeding**

```
Mode: Time Only
Schedule: 6:00 AM
Duration: 20 minutes
Days: Mon, Wed, Fri
Max Duration: 30 minutes
```

### Setup C: Smart Hybrid (Advanced)
**Perfect for: Maximum efficiency, water restrictions**

```
Mode: Hybrid
Schedule Window: 6:00 AM - 8:00 AM
Days: Every day
Low %: 40
High %: 70
Duration: 60 (max allowed, will stop early if wet)
```

---

## 📋 Pre-Flight Checklist

Before going live, verify:

- [ ] All soil sensors calibrated
- [ ] Zones respond to moisture changes
- [ ] Relays clicking when zones activate
- [ ] Water actually flows to plants
- [ ] No leaks in system
- [ ] Temperature sensor reading correctly
- [ ] Display shows accurate moisture %
- [ ] Freeze protection enabled (if in cold climate)
- [ ] Max daily watering set appropriately
- [ ] Network connected (if using WiFi/Ethernet)
- [ ] Time synchronized (if using NTP)

---

## 🔧 Essential Settings Reference

### System Setup Menu
```
Number of Zones:     1-16 (match your hardware)
Temperature Units:   F or C
Relay Type:          NC (normally closed) - safest
Freeze Protection:   ON (recommended)
Freeze Temp:         35°F / 2°C
Max Daily Watering:  180 min (3 hours per zone)
Sensor Interval:     5000ms (5 seconds)
```

### Zone Settings (Typical Values)
```
VEGETABLES:
├── Low %: 40-50
├── High %: 70-80
└── Mode: Moisture Only

LAWNS:
├── Schedule: 6:00 AM, 20 min
├── Days: M/W/F
└── Mode: Time Only

FLOWERS/SHRUBS:
├── Low %: 35-45
├── High %: 65-75
├── Schedule: 6:00-8:00 AM
└── Mode: Hybrid
```

---

## ⚡ Emergency Procedures

### Stop All Watering Immediately
```
1. Press MENU until Main Menu appears
2. Navigate to Manual Control
3. Turn off all zones individually

OR

4. Remove power from relay board
```

### Reset to Factory Defaults
```
Option 1 (Code):
Uncomment lines 206-240 in setup()
Upload sketch
Re-comment and re-upload

Option 2 (Will be added):
System Setup → Advanced → Reset All
```

### Winterize for Freezing Weather
```
Main Menu → Winterize → SELECT
System will pulse each valve for 5 seconds
Then manually blow out lines with air compressor
```

---

## 🐛 Quick Troubleshooting

| Problem | Quick Fix |
|---------|-----------|
| Zone won't turn on | Check: Enabled? Moisture below Low %? |
| Zone won't turn off | Check: Moisture above High %? Sensor working? |
| Moisture reads 0% | Sensor not connected or calibration needed |
| Moisture reads 100% | Sensor shorted or wrong calibration |
| No display | Check SPI wiring: CS=47, DC=49, Reset=48 |
| All zones on | Wrong relay type setting (change NC↔NO) |
| Network failed | Normal - system works without network |
| Time resets | Need RTC module or enable NTP |

---

## 🎓 Learning Path

### Week 1: Master the Basics
- [ ] Navigate full menu system
- [ ] Calibrate all sensors
- [ ] Set thresholds for each zone
- [ ] Watch moisture-based operation
- [ ] Read data logs in Serial Monitor

### Week 2: Optimize Settings
- [ ] Fine-tune thresholds based on results
- [ ] Try different watering modes
- [ ] Set up schedules if desired
- [ ] Monitor daily watering totals
- [ ] Adjust temperature compensation

### Week 3: Advanced Features
- [ ] Configure network connectivity
- [ ] Set up NTP time sync
- [ ] Create hybrid mode schedules
- [ ] Enable data logging
- [ ] Experiment with freeze protection

### Month 2+: Fine-Tuning
- [ ] Seasonal adjustment of thresholds
- [ ] Different modes for different plants
- [ ] Optimize for water conservation
- [ ] Track long-term statistics
- [ ] Plan future enhancements

---

## 📊 Understanding the Status Display

```
┌────────────────┐
│09:45:32    72F │ ← Time and Temperature
│1: 45% ON  12m │ ← Zone 1: 45% moisture, ON, running 12 min
│2: 68% OFF   M │ ← Zone 2: 68% moisture, OFF, Moisture mode
│3: 52% OFF   T │ ← Zone 3: Time-based mode
│4: 41% ON   5m │ ← Zone 4: Running 5 minutes
│5: 73% OFF   H │ ← Zone 5: Hybrid mode
│6: 38% OFF   M │ ← Zone 6: Moisture mode
│MENU for opts │ ← Press to access menus
└────────────────┘

Mode Indicators:
M = Moisture Only
T = Time Only
H = Hybrid
X = Manual
```

---

## 🔌 Wiring Diagram Summary

```
ARDUINO MEGA 2560
┌─────────────────────────────────────┐
│                                     │
│  DISPLAY                   BUTTONS  │
│  ├─ CS → 47               Menu → 22│
│  ├─ DC → 49               Up → 24  │
│  └─ RST → 48              Sel → 26 │
│                           Down → 28│
│  I2C                               │
│  ├─ SDA → 20 (Temp+RTC)           │
│  └─ SCL → 21                       │
│                                     │
│  SENSORS (A0-A15)         RELAYS   │
│  Zone 1 → A0              Zone 1→2 │
│  Zone 2 → A1              Zone 2→3 │
│  ... etc                  ... etc  │
│  Zone 16 → A15            Zone16→17│
└─────────────────────────────────────┘

POWER:
Arduino: 7-12V DC, 1A minimum
Relays: 12V DC, 2A minimum (separate supply)
Sensors: Powered from Arduino 5V
```

---

## 💾 Settings Storage

**What's Saved in EEPROM:**
- ✅ Number of zones
- ✅ All zone configurations
- ✅ Moisture thresholds
- ✅ Calibration values
- ✅ Watering schedules
- ✅ System preferences
- ✅ Network settings
- ✅ Temperature compensation values

**What's NOT Saved:**
- ❌ Current time (needs RTC or NTP)
- ❌ Daily statistics (resets at midnight)
- ❌ Runtime counters (resets on power loss)
- ❌ WiFi connection state

**Backup Tip:** Document your settings! Use Serial Monitor to view current config, or manually note important values.

---

## 📱 Future App/Web Interface

The network connectivity in V3.0 provides foundation for:

**Coming Soon:**
- Web dashboard for remote monitoring
- Mobile app control
- Push notifications
- Weather integration
- Historical charts
- Remote schedule changes

**Current:** Serial monitor provides real-time data

---

## 🌡️ Temperature Compensation Explained

**Purpose:** Plants need more water in hot weather

**How it works:**
```
Normal Weather (temp < 92°F):
├── Turn on at: 40%
└── Turn off at: 70%

Hot Weather (temp ≥ 92°F):
├── Turn on at: 46% (40% + 6%)
└── Turn off at: 74% (70% + 4%)
```

**Configure:**
- Temp Switch: 92°F (when adjustment kicks in)
- Temp Adj Low: +6% (added to turn-on threshold)
- Temp Adj High: +4% (added to turn-off threshold)

**Disable:** Set both adjustments to 0

---

## 🎉 Success Indicators

You know it's working when:

1. ✅ Status display shows live moisture percentages
2. ✅ Zones turn on automatically when moisture drops
3. ✅ Zones turn off when moisture rises
4. ✅ Plants stay healthy and watered
5. ✅ No flooding or overwatering
6. ✅ Network shows "Connected" (if enabled)
7. ✅ Time displays correctly
8. ✅ Serial monitor shows periodic data logs

---

## 📞 Getting Help

**Check These First:**
1. Full README_V3.0.md (comprehensive guide)
2. Serial Monitor output (115200 baud)
3. Current sensor readings on display
4. EEPROM values in Serial Monitor on boot

**Common Issues Database:**
- See Troubleshooting section in main README
- Review hardware connections
- Verify library versions
- Check power supply adequacy

---

## ✨ Pro Tips

1. **Start Simple:** Use Moisture Only mode first, add complexity later
2. **Calibrate Properly:** Good calibration = accurate operation
3. **Test Before Deploying:** Run for a day while monitoring
4. **Conservative Thresholds:** Better too dry than too wet initially
5. **Enable Logging:** Data helps optimize settings
6. **Use Freeze Protection:** Saves headaches in winter
7. **Set Max Duration:** Safety net against failures
8. **Label Your Zones:** Edit zone names to remember what's what
9. **Document Changes:** Keep notes on threshold adjustments
10. **Seasonal Adjustments:** Review settings spring and fall

---

**Ready to start? Follow Step 1 and you'll be watering in 15 minutes!**

Happy Irrigating! 🌱💧
