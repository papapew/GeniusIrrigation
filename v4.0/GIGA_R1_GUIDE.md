# Arduino GIGA R1 WiFi Irrigation Controller - V4.0 Guide

## 🚀 Welcome to V4.0 - The GIGA Edition!

Version 4.0 is a complete redesign for the **Arduino GIGA R1 WiFi** with **GIGA Display Shield**, bringing professional-grade touch controls and modern visual design to your irrigation system.

---

## ✨ What's New in V4.0

### 🎨 **Beautiful Touch Interface**
- **480x320 Full-Color TFT Display** - Crystal clear graphics
- **Capacitive Touchscreen** - No physical buttons needed!
- **Modern Dark Theme** - Professional appearance with vibrant accents
- **Smooth Animations** - Polished, responsive UI
- **Real-Time Graphs** - Visual moisture history for each zone

### 💪 **GIGA R1 Power**
- **Dual-Core STM32H747XI** - M7 @ 480MHz + M4 @ 240MHz
- **Built-in WiFi** - No external module needed
- **14 ADC Inputs** - Support for 14 zones (GIGA limitation)
- **More RAM** - 1MB for complex operations
- **Faster Processing** - Instant response to touch

### 📱 **Touch-First Design**
- **No Physical Buttons Required** - All touch control
- **Intuitive Navigation** - Familiar smartphone-like interface
- **Visual Feedback** - See moisture bars, colors, status instantly
- **Zone Cards** - Beautiful cards showing all zone info
- **Live Graphs** - Historical moisture data visualization

### 🌐 **Network Features Retained**
- Built-in WiFi (no module needed!)
- Web interface for remote control
- RESTful API
- NTP time synchronization
- All V3.1 network features

---

## 📦 Hardware Requirements

### **Required Components**

#### **Main Controller**
- ✅ **Arduino GIGA R1 WiFi**
  - Part#: ABX00063
  - Built-in WiFi
  - Dual-core processor
  - 14 analog inputs (A0-A13)

#### **Display**
- ✅ **Arduino GIGA Display Shield**
  - Part#: ASX00039
  - 480x320 TFT LCD
  - Capacitive touch
  - Mounts directly on GIGA R1

#### **Sensors**
- ✅ **AM2315C Temperature/Humidity Sensor** (I2C)
- ✅ **14x Capacitive Soil Moisture Sensors** (max)

#### **Relay Control**
- ✅ **14-Channel Relay Board**
  - 12V DC coil voltage
  - Optocoupler isolated
  - Supports NC or NO relays

#### **Power Supply**
- ✅ **12V 2A DC** for relay board
- ✅ **7-12V 1A DC** for GIGA R1
- OR **USB-C** for GIGA R1 (5V 3A)

### **Wiring Changes from Mega Version**

#### **Display**
- **No wiring needed!** Display shield plugs directly onto GIGA R1
- Automatic pin connections via shield headers

#### **Analog Inputs**
```
Zone 1  → A0      Zone 8  → A7
Zone 2  → A1      Zone 9  → A8
Zone 3  → A2      Zone 10 → A9
Zone 4  → A3      Zone 11 → A10
Zone 5  → A4      Zone 12 → A11
Zone 6  → A5      Zone 13 → A12
Zone 7  → A6      Zone 14 → A13
```

**⚠️ Important:** GIGA R1 has only 14 ADC inputs (vs 16 on Mega)

#### **Digital Outputs (Relays)**
```
Zone 1  → Pin 2      Zone 8  → Pin 9
Zone 2  → Pin 3      Zone 9  → Pin 10
Zone 3  → Pin 4      Zone 10 → Pin 11
Zone 4  → Pin 5      Zone 11 → Pin 12
Zone 5  → Pin 6      Zone 12 → Pin 13
Zone 6  → Pin 7      Zone 13 → Pin 14
Zone 7  → Pin 8      Zone 14 → Pin 15
```

#### **I2C (Temperature Sensor)**
- SDA → Pin 20
- SCL → Pin 21
- Same as Mega!

---

## 📚 Required Libraries

### **GIGA-Specific Libraries**

Install these via Library Manager:

1. **Arduino_GigaDisplay_GFX**
   - For: GIGA Display Shield graphics
   - Search: "Arduino_GigaDisplay_GFX"
   - Install the official Arduino version

2. **Arduino_GigaDisplayTouch**
   - For: Touch input handling
   - Search: "Arduino_GigaDisplayTouch"  
   - Install the official Arduino version

### **Standard Libraries (Same as V3.1)**

3. **AM2315C** by Rob Tillaart
4. **ArduinoJson** (v6.21+, NOT v7)
5. **TimeLib** by Michael Margolis
6. **WiFi** - Built-in with GIGA board package
7. **WebServer** - Built-in with GIGA board package

### **Built-in Libraries**
- Wire (I2C)
- SPI
- EEPROM

### **Installation Steps**

```
1. Install GIGA R1 WiFi board support:
   Tools → Board → Boards Manager
   Search "Arduino Mbed OS Giga Boards"
   Install

2. Install libraries via Library Manager:
   Arduino_GigaDisplay_GFX
   Arduino_GigaDisplayTouch
   AM2315C
   ArduinoJson (v6.21+)
   TimeLib

3. Select board:
   Tools → Board → Arduino Mbed OS Giga Boards → Arduino GIGA R1 WiFi

4. Upload!
```

---

## 🎨 User Interface Guide

### **Screen Layout**

```
┌─────────────────────────────────────────────┐
│ HEADER (45px)                               │
│ Time | Temp/Humidity | Active Zones | WiFi │
├─────────────────────────────────────────────┤
│                                             │
│                                             │
│           MAIN CONTENT AREA                 │
│              (235px)                        │
│                                             │
│                                             │
├─────────────────────────────────────────────┤
│ FOOTER - Navigation (40px)                  │
│ [Home] [Zones] [Schedule] [System] [Network]│
└─────────────────────────────────────────────┘
```

### **Color Theme**

The interface uses a modern dark theme:

- **Background**: Deep blue-gray (#0B1A)
- **Cards**: Medium blue-gray (#2965)
- **Accent**: Bright cyan (#04DF)
- **Success**: Green (#07E0) - Zone watering
- **Warning**: Orange (#FD20) - Moderate moisture
- **Danger**: Red (#F800) - Dry soil
- **Text**: White (#FFFF)

---

## 📱 Touch Interface Screens

### **1. Home Screen**

**What you see:**
- Large system status cards
- Active zones count
- Total watering today
- Live zone status list with:
  - Zone name
  - Moisture percentage
  - Color-coded bar
  - ON/OFF status
  - Runtime (if watering)

**Touch actions:**
- Tap any zone → Go to Zone Detail
- Tap "View All Zones" → Go to Zones screen
- Use footer to navigate

**Perfect for:** Quick overview of entire system

---

### **2. Zones Screen**

**What you see:**
- Grid of zone cards (3 columns × 2 rows)
- Each card shows:
  - Zone number badge
  - Zone name
  - Large moisture percentage
  - Color-coded progress bar
  - Watering mode
  - Current status

**Touch actions:**
- Tap any zone card → Zone Detail screen
- Cards highlight on active zones
- Swipe for more zones (if > 6)

**Perfect for:** Monitoring all zones at a glance

---

### **3. Zone Detail Screen**

**What you see:**
- Huge moisture display (5x font size!)
- Current status badge
- Runtime (if watering)
- **Historical graph** - Last 60 readings
- Configuration summary:
  - Operating mode
  - Thresholds
  - Today's total
  - Max duration

**Touch actions:**
- **Turn ON/OFF** - Manual zone control
- **Configure** - Edit settings (future)
- **Calibrate** - Sensor calibration
- **< Back** - Return to Zones

**Perfect for:** Deep dive into single zone

---

### **4. Calibration Screen**

**What you see:**
- Current raw sensor reading
- Step-by-step instructions
- Dry calibration value
- Wet calibration value

**Touch actions:**
- **Set Dry Value** - Capture dry reading
- **Set Wet Value** - Capture wet reading
- **< Back** - Return to Zone Detail

**Calibration procedure:**
1. Remove sensor from soil
2. Let dry completely (10 min)
3. Tap "Set Dry Value"
4. Submerge in water
5. Wait for stabilization (2 min)
6. Tap "Set Wet Value"
7. Done! Values saved to EEPROM

**Perfect for:** Accurate sensor calibration

---

### **5. Schedules Screen**

Currently shows: "Schedule configuration coming soon..."

**Planned features:**
- Time-based schedule editor
- Day-of-week selection
- Visual timeline
- Multiple schedules per zone

---

### **6. System Settings Screen**

**What you see:**
- Number of zones
- Temperature units (F/C)
- Freeze protection status
- Freeze temperature
- Max daily watering limit

**Touch actions:**
- Edit values (future update)
- Toggle settings

**Perfect for:** System configuration

---

### **7. Network Screen**

**What you see:**
- WiFi connection status
- IP address
- Signal strength (RSSI)
- Web interface URL

**Touch actions:**
- View network status
- Copy IP for web access

**Perfect for:** Network troubleshooting

---

## 🎯 Touch Gestures

### **Supported Gestures**

**Single Tap**
- Primary interaction
- Buttons, cards, controls
- Instant visual feedback

**Touch and Hold**
- Future: Context menus
- Future: Detailed information

**Swipe** (Future)
- Scroll through zones
- Navigate screens
- Timeline scrubbing

### **Visual Feedback**

All touch interactions provide immediate feedback:
- Buttons change color on touch
- Cards highlight when tapped
- Status updates instantly
- Smooth transitions

---

## 💡 Using the Touch Interface

### **First-Time Setup**

1. **Power On**
   - Display shows splash screen
   - "Initializing..." message
   - Loads configuration from EEPROM

2. **Home Screen Appears**
   - Shows current system status
   - All zones visible
   - Ready for touch input!

3. **Explore the Interface**
   - Tap footer buttons to navigate
   - Tap zone cards for details
   - Use back buttons to return

### **Daily Operation**

**Check System Status:**
1. Touch screen to wake (if sleeping)
2. View Home screen
3. Check active zones count
4. Review moisture levels

**View Specific Zone:**
1. Go to Zones screen (footer)
2. Tap desired zone card
3. View detail screen with graph
4. Return via Back button

**Manual Control:**
1. Navigate to zone detail
2. Tap "Turn ON" or "Turn OFF"
3. Watch status update
4. Zone responds immediately

**Calibrate Sensor:**
1. Go to zone detail
2. Tap "Calibrate"
3. Follow on-screen instructions
4. Values save automatically

### **Screen Timeout**

To save power and prevent burn-in:
- Screen dims after 5 minutes (configurable)
- Touch anywhere to wake
- Continues watering in background
- No data loss during sleep

---

## 🎨 Understanding Visual Elements

### **Progress Bars**

Color indicates moisture status:
- **Red** (0-39%): Dry - needs water
- **Orange** (40-69%): Moderate
- **Green** (70-100%): Wet - adequate

### **Status Badges**

Small colored labels showing zone status:
- **Green "ON"**: Currently watering
- **Gray "OFF"**: Not watering
- **Green "WATERING"**: Active irrigation

### **Zone Cards**

Complete zone information at a glance:
- **Blue circle** with number: Zone ID
- **Large percentage**: Current moisture
- **Progress bar**: Visual moisture level
- **Mode label**: Operating mode
- **Green border**: Active (watering)
- **Gray border**: Inactive

### **Graphs**

Historical moisture tracking:
- **X-axis**: Time (60 readings)
- **Y-axis**: Moisture percentage (0-100%)
- **Cyan line**: Moisture trend
- **Grid lines**: Reference marks

---

## 🔧 Configuration via Touch

### **Current Touch Configuration Options**

**Available Now:**
- ✅ Manual zone ON/OFF
- ✅ Sensor calibration (dry/wet)
- ✅ View all settings
- ✅ Network status

**Coming in Future Updates:**
- ⏳ Edit zone names
- ⏳ Adjust thresholds
- ⏳ Set schedules
- ⏳ Configure system settings
- ⏳ WiFi credential entry

**Workaround for Now:**
Use the web interface for detailed configuration, or edit sketch and re-upload.

---

## 🌐 Web Interface

The V4.0 GIGA version **retains full web interface** from V3.1:

**Access:**
```
http://[GIGA IP ADDRESS]
```

**Find IP Address:**
1. Check Network screen on display
2. OR check Serial Monitor
3. Shown on header if connected

**Web Features:**
- All V3.1 web functionality
- Configure zones remotely
- Set schedules
- Adjust thresholds  
- View status
- Control zones

**Best Practice:**
- Use touch interface for monitoring
- Use web interface for configuration
- Dual interface provides flexibility!

---

## 🔌 Hardware Assembly

### **Step 1: Prepare GIGA R1**

1. Remove from packaging
2. Inspect for damage
3. Connect USB-C cable to computer
4. Verify board appears in IDE

### **Step 2: Mount Display Shield**

1. **Align headers carefully**
   - Display shield has female headers
   - GIGA R1 has male headers
   - Match pin 1 to pin 1

2. **Press firmly**
   - Even pressure on all sides
   - Headers should seat completely
   - No gaps visible

3. **Verify connection**
   - Shield sits flat on GIGA
   - No pins bent
   - Secure fit

**⚠️ Important:** Never force! If resistance, check alignment.

### **Step 3: Connect Sensors**

**Temperature Sensor (I2C):**
```
AM2315C        GIGA R1
───────────────────────
VCC     →      5V
GND     →      GND
SDA     →      Pin 20
SCL     →      Pin 21
```

**Soil Moisture Sensors:**
```
Sensor     GIGA Pin
────────────────────
Zone 1     A0
Zone 2     A1
...
Zone 14    A13
```

Each sensor needs:
- VCC → 5V
- GND → GND  
- Signal → Analog pin

### **Step 4: Connect Relays**

**Relay Board Connections:**
```
GIGA Pin    Relay Board
─────────────────────────
2           IN1 (Zone 1)
3           IN2 (Zone 2)
...
15          IN14 (Zone 14)

GND         GND (common)
```

**Relay Board Power:**
- VCC → 12V DC (separate supply!)
- GND → Common with GIGA
- JD-VCC → 12V (if jumper removed)

**⚠️ Critical:** Never power relays from GIGA 5V!

### **Step 5: Power Supply**

**Option A: USB-C Power (Recommended for testing)**
```
USB-C 5V 3A → GIGA R1 USB-C port
12V 2A      → Relay board
```

**Option B: DC Power (Recommended for deployment)**
```
7-12V 1A    → GIGA VIN pin
12V 2A      → Relay board
```

**Option C: Shared Power (Advanced)**
```
12V 3A supply:
├─ GIGA VIN (via voltage regulator)
└─ Relay Board VCC
```

---

## 💻 Software Setup

### **1. Install Arduino IDE**

- Download Arduino IDE 2.x
- Install for your OS
- Launch IDE

### **2. Install GIGA Board Support**

```
File → Preferences → Additional Board Manager URLs
Add: (automatically included in IDE 2.x)

Tools → Board → Boards Manager
Search: "Arduino Mbed OS Giga Boards"
Install latest version
```

### **3. Install Required Libraries**

Via Library Manager (Tools → Manage Libraries):

```
1. Arduino_GigaDisplay_GFX
2. Arduino_GigaDisplayTouch
3. AM2315C
4. ArduinoJson (version 6.21+)
5. TimeLib
```

### **4. Configure WiFi Credentials**

Edit sketch (lines ~95-96):

```cpp
const char* ssid = "YourWiFiSSID";          // Your network name
const char* password = "YourWiFiPassword";   // Your password
```

### **5. Select Board & Port**

```
Tools → Board → Arduino Mbed OS Giga Boards → Arduino GIGA R1 WiFi
Tools → Port → [Your GIGA's port]
```

### **6. Upload Sketch**

```
Click Upload button (→)
Wait for "Done uploading"
Display should show splash screen!
```

---

## 🐛 Troubleshooting

### **Display Issues**

**Blank Screen**
- Check: Display shield properly seated?
- Check: Power supply adequate?
- Try: Re-seat display shield
- Try: Upload sketch again

**Garbled Display**
- Check: Arduino_GigaDisplay_GFX installed?
- Try: Update to latest library version
- Check: Sketch uploaded to GIGA (not other board)

**Touch Not Working**
- Check: Arduino_GigaDisplayTouch installed?
- Try: Clean screen surface
- Check: Shield fully seated
- Try: Recalibrate (future feature)

### **Compilation Errors**

**"Arduino_GigaDisplay_GFX.h not found"**
- Install Arduino_GigaDisplay_GFX library
- Restart Arduino IDE

**"Arduino_GigaDisplayTouch.h not found"**
- Install Arduino_GigaDisplayTouch library
- Restart IDE

**"Wrong board selected"**
- Tools → Board → Arduino GIGA R1 WiFi
- NOT Mega, Uno, or other boards

**ArduinoJson errors**
- Use version 6.21.x
- NOT version 7.x
- Uninstall v7 if present

### **Runtime Issues**

**WiFi Won't Connect**
- Check credentials in sketch
- Verify 2.4GHz WiFi (5GHz not supported)
- Check signal strength
- Try restarting router
- Check Serial Monitor for errors

**Zones Don't Respond**
- Check relay wiring
- Verify relay power supply
- Test relays manually
- Check digital pin numbers
- Verify sysConfig.relayOn setting

**Moisture Readings Wrong**
- Calibrate sensors via Calibrate screen
- Check sensor wiring
- Verify sensors are capacitive type
- Check 5V power to sensors

**Touch Unresponsive**
- Clean screen
- Check for screen protector interference
- Restart GIGA
- Re-seat display shield

---

## 🚀 First-Time Startup Checklist

Before powering on:

**Hardware:**
- [ ] Display shield properly seated
- [ ] All sensors connected
- [ ] Relays wired correctly
- [ ] Power supplies adequate
- [ ] No shorts or loose wires
- [ ] Sprinkler valves isolated (for testing)

**Software:**
- [ ] GIGA board support installed
- [ ] All libraries installed
- [ ] WiFi credentials entered
- [ ] Correct board selected
- [ ] Sketch compiles without errors
- [ ] Uploaded successfully

**Configuration:**
- [ ] Number of zones set (default: 6)
- [ ] Relay type configured (NC/NO)
- [ ] Time zone set
- [ ] Freeze protection enabled

**First Boot:**
1. Power on GIGA
2. Watch splash screen
3. Display shows "Ready!"
4. Note IP address
5. Touch screen to interact!

---

## 📊 Performance Specifications

### **GIGA R1 WiFi Specs**

**Processor:**
- M7 core @ 480MHz
- M4 core @ 240MHz
- Dual-core operation

**Memory:**
- 1MB RAM
- 2MB Flash
- More than enough for V4.0!

**Connectivity:**
- WiFi 802.11 b/g/n
- Bluetooth (not used in V4.0)
- Built-in antenna

**Display:**
- 480×320 pixels
- 65K colors
- Capacitive touch
- LED backlight

### **System Performance**

**Response Times:**
- Touch to action: <100ms
- Screen updates: 60 FPS capable
- Sensor reads: ~100ms each
- Network requests: <500ms

**Power Consumption:**
- GIGA R1: ~300mA @ 5V
- Display: ~150mA with backlight
- Total system: ~650mA + relays

---

## 🎓 Advanced Features

### **Moisture History Graphs**

Each zone stores 60 moisture readings:
- Updates with each sensor read
- Circular buffer (oldest replaced)
- Displayed on Zone Detail screen
- Helps identify trends
- Useful for diagnostics

**Interpreting Graphs:**
- Flat line: Stable moisture
- Upward trend: Getting wetter
- Downward trend: Drying out
- Spikes: Watering events
- Consistent: Good threshold settings

### **Screen Saver**

Power-saving feature:
- Activates after timeout (default: 5 min)
- Screen goes black
- Touch to wake
- Irrigation continues
- Configurable in System Settings

### **Status Indicators**

**Header always shows:**
- Current time (HH:MM:SS)
- Temperature
- Humidity
- Active zones count
- WiFi status with IP

**Updates automatically** - no refresh needed!

---

## 🔮 Future Enhancements

### **Planned for V4.1**

**Touch Configuration:**
- ✨ Edit zone names
- ✨ Adjust thresholds via sliders
- ✨ Schedule editor with calendar
- ✨ WiFi setup wizard
- ✨ Settings menus

**Visual Improvements:**
- ✨ More animation effects
- ✨ Customizable themes
- ✨ Swipe gestures
- ✨ Haptic feedback (if hardware added)
- ✨ Night mode (auto-dim)

**Data Features:**
- ✨ Export logs to SD card
- ✨ Daily/weekly reports
- ✨ Water usage statistics
- ✨ Predictive analytics

### **Possible V5.0 Features**

- 🔮 Cloud connectivity
- 🔮 Weather integration
- 🔮 Mobile app (iOS/Android)
- 🔮 Voice control
- 🔮 Multi-controller sync
- 🔮 AI-powered optimization

---

## 📖 Comparison: GIGA vs Mega

### **Why Upgrade to GIGA?**

| Feature | Mega 2560 | GIGA R1 WiFi |
|---------|-----------|--------------|
| **Display** | Small OLED | 480×320 TFT |
| **Touch** | No | Yes! |
| **Interface** | Buttons | Touchscreen |
| **WiFi** | Module needed | Built-in |
| **Speed** | 16MHz | 480MHz |
| **Zones** | 16 | 14* |
| **Graphics** | Text only | Full color |
| **Price** | ~$45 | ~$90 |

*GIGA limitation: only 14 ADC inputs vs 16 on Mega

### **When to Choose GIGA**

✅ **Choose GIGA if:**
- You want modern touch interface
- Visual monitoring important
- Professional appearance desired
- Built-in WiFi preferred
- Budget allows (~$90 + shield)

✅ **Choose Mega if:**
- Need 16 zones
- Budget limited (~$45)
- Don't need touch interface
- Smaller display acceptable
- Prefer simple OLED

**Both versions** have full functionality for irrigation control!

---

## 🎯 Tips & Best Practices

### **Touch Interface**

1. **Keep screen clean** - Fingerprints affect touch
2. **Use gentle taps** - No need to press hard
3. **Avoid sharp objects** - Use finger, not stylus
4. **Update regularly** - Check for new versions
5. **Calibrate sensors** - Accurate graphs need good data

### **Display Care**

1. **Avoid direct sunlight** - Can damage LCD
2. **Keep dry** - Not weatherproof
3. **Clean with microfiber** - No harsh chemicals
4. **Screen protector** - Consider adding one
5. **Ventilation** - Ensure good airflow

### **Configuration**

1. **Use web for setup** - Easier than touch (for now)
2. **Document settings** - Screenshot configurations
3. **Test before deploying** - Verify everything works
4. **Start with few zones** - Add gradually
5. **Monitor first week** - Fine-tune thresholds

---

## 📞 Getting Help

### **Check These First:**

1. **Serial Monitor** (115200 baud)
   - Boot messages
   - Error reports
   - WiFi status
   - System logs

2. **Network Screen**
   - WiFi status
   - IP address
   - Signal strength

3. **System Screen**
   - Configuration summary
   - Active settings

### **Common Questions**

**Q: Why only 14 zones vs 16 on Mega?**
A: GIGA R1 has only 14 ADC inputs (A0-A13)

**Q: Can I use old Mega code?**
A: No - display and touch libraries different

**Q: Does web interface still work?**
A: Yes! Full V3.1 web features included

**Q: Can I add more zones?**
A: Not easily - ADC limit is hardware

**Q: Is touchscreen required?**
A: Yes for V4.0 - use V3.1 for button interface

**Q: What about weatherproofing?**
A: Display is indoor only - use web for outdoor access

---

## 🌟 Conclusion

**V4.0 brings professional-grade touch control** to irrigation management with the GIGA R1 WiFi and Display Shield combo.

**Perfect for:**
- Greenhouse installations
- Indoor grow operations
- Hydroponic systems
- Display-accessible locations
- Users wanting modern interface

**The beautiful touchscreen interface** makes monitoring and control a pleasure, while the powerful GIGA R1 handles complex operations effortlessly.

**Welcome to the future of irrigation control!** 🌱💧📱

---

**Questions? Issues? Improvements?**

Check the GitHub repository or Arduino forums for:
- Latest updates
- Bug fixes
- Feature requests
- Community support

**Happy irrigating with your GIGA!** 🎉
