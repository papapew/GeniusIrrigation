# Web Interface Guide - Irrigation Controller V3.1

## 🌐 New in Version 3.1

Version 3.1 adds a **beautiful, modern web interface** that runs directly on your Arduino! Configure everything from your phone, tablet, or computer - no serial monitor or button navigation needed.

### Key Features

✨ **Sleek Dark Theme Interface**
- Modern, responsive design works on any device
- Real-time updates without page refreshes
- Smooth animations and visual feedback
- Mobile-optimized for on-the-go control

💾 **Persistent Configuration**
- All settings automatically save to EEPROM
- Survives power loss
- No data loss between sessions

🎛️ **Complete Control**
- Configure all 16 zones
- Set watering modes and schedules
- Adjust thresholds and calibration
- System-wide settings
- Manual zone control

📊 **Real-Time Monitoring**
- Live moisture readings
- Current zone status
- Temperature and humidity
- Active watering time
- Today's watering totals

---

## 🚀 Quick Start

### 1. Setup Network Connection

Edit these lines in the sketch:

```cpp
// Line 26: Choose WiFi or Ethernet
#define USE_WIFI  // For WiFi
// #define USE_ETHERNET  // For Ethernet

// Lines 74-75: WiFi credentials
const char* ssid = "YourWiFiSSID";          // Change this
const char* password = "YourWiFiPassword";   // Change this
```

### 2. Upload Sketch

- Install required library: **ArduinoJson** (via Library Manager)
- Upload V3.1 sketch to your Arduino Mega
- Open Serial Monitor (115200 baud)
- Note the IP address displayed

### 3. Access Web Interface

Open your browser and navigate to the IP address shown:
```
http://192.168.1.100  (example - use YOUR IP)
```

**Bookmark it!** The IP won't change (unless you restart your router or the Arduino requests a new DHCP address).

---

## 📱 Web Interface Tour

### Header Section

```
┌─────────────────────────────────────────┐
│  🌱 Irrigation Control System          │
│                                         │
│  Temperature: 72°F   Humidity: 65%     │
│  Time: 09:30:45      Active Zones: 2   │
└─────────────────────────────────────────┘
```

Shows real-time system status. Updates every 5 seconds automatically.

### Navigation Tabs

**Zones** → Monitor and control individual zones  
**System Settings** → Basic configuration  
**Schedules** → Time-based watering schedules  
**Advanced** → Network, logging, and advanced features

---

## 🎯 Using the Zones Tab

### Zone Card Layout

Each zone displays as a card showing:

```
┌──────────────────────────────┐
│ Zone 1: Front Lawn    ⚫ OFF │  ← Name and Status
│                              │
│         45%                  │  ← Current Moisture
│ ▓▓▓▓▓░░░░░░░░░░░            │  ← Visual bar
│                              │
│ Mode: Moisture   Today: 12m │  ← Info
│ Threshold: 40%-70%  Enabled │
│                              │
│ [Configure] [Turn On]       │  ← Actions
└──────────────────────────────┘
```

### Zone Card Features

**Status Indicator**
- ⚫ **OFF** (gray) - Zone is not watering
- 🟢 **ON** (green) - Zone actively watering
- Pulsing dot when active

**Moisture Display**
- Large percentage (0-100%)
- Color-coded bar:
  - Red (0-33%): Dry
  - Yellow (34-66%): Moderate
  - Green (67-100%): Wet

**Quick Info**
- Operating mode (Moisture/Time/Hybrid/Manual)
- Total watering time today
- On/Off thresholds
- Enabled/Disabled status

**Action Buttons**
- **Configure**: Opens detailed settings
- **Turn On/Off**: Manual override control

### Zone Configuration Modal

Click **Configure** on any zone to open detailed settings:

#### Basic Settings

**Zone Name** (up to 16 characters)
- Default: "Zone 1", "Zone 2", etc.
- Examples: "Front Lawn", "Tomatoes", "Roses"
- Helpful for identifying zones

**Enabled** (Toggle)
- ON: Zone participates in automatic watering
- OFF: Zone completely disabled (safety)

**Watering Mode**
- **Moisture Only**: Water based on soil moisture
- **Time Only**: Water on schedule regardless of moisture
- **Hybrid**: Schedule + moisture control
- **Manual**: No automatic operation

#### Threshold Settings

**Turn On Threshold** (0-100%)
- Moisture level that triggers watering
- Lower = drier before watering
- Typical: 30-50%
- Example: 40% = water when soil is 40% moist

**Turn Off Threshold** (0-100%)
- Moisture level that stops watering
- Higher = wetter before stopping
- Typical: 60-80%
- Example: 70% = stop when soil reaches 70% moist
- **Must be higher than "Turn On"**

**Max Duration** (minutes)
- Emergency shutoff timer
- Prevents flooding if sensor fails
- Typical: 30-60 minutes
- Example: 60 = automatically shut off after 1 hour

#### Calibration Values

**Dry Calibration Value**
- Raw sensor reading when completely dry
- Obtained from calibration procedure
- Typical: 100-200
- See Calibration Guide below

**Wet Calibration Value**
- Raw sensor reading when saturated
- Obtained from calibration procedure
- Typical: 400-600
- See Calibration Guide below

### Saving Zone Configuration

Click **"Save Zone Configuration"** button at bottom of modal.

You'll see:
```
✓ Zone configuration saved to EEPROM!
```

Settings are **permanently saved** and will survive power loss.

---

## ⚙️ System Settings Tab

### Number of Zones
- Set how many physical zones you have (1-16)
- Only configured zones will be monitored
- Default: 6

### Temperature Units
- Fahrenheit (°F) or Celsius (°C)
- Affects display only, not functionality
- Default: Fahrenheit

### Freeze Protection Temperature
- Temperature below which ALL watering stops
- Prevents damage to frozen pipes
- Default: 35°F (2°C)
- Recommended: Keep at 35°F

### Max Daily Watering
- Maximum minutes per zone per day
- Safety limit to prevent flooding
- Resets at midnight
- Default: 180 minutes (3 hours)
- Recommended: 60-240 minutes depending on zone

**💡 Pro Tip**: Set this lower than you think you need. It's a safety net.

### Saving System Settings

Click **"Save System Settings"**

All system configuration saves to EEPROM permanently.

---

## 📅 Schedules Tab

Configure time-based watering schedules for each zone.

### Schedule Configuration

For each zone, you can set:

**Enable Schedule** (Toggle)
- ON: Schedule is active
- OFF: Schedule ignored

**Start Time**
- Hour (0-23 for 24-hour, or 1-12 for 12-hour)
- Minute (0-59)
- Example: 06:00 for 6:00 AM

**Duration** (minutes)
- How long to water
- Example: 20 = run for 20 minutes

**Days of Week**
- Select which days to water
- Checkboxes for each day
- Example: Monday, Wednesday, Friday

### How Schedules Work by Mode

**Time Only Mode**
- Waters ONLY during scheduled time
- Ignores moisture sensors
- Runs for exact duration
- Example: Waters lawn at 6 AM for 20 minutes every M/W/F

**Hybrid Mode**
- Only operates during scheduled window
- Uses moisture sensors to start/stop
- Won't water if already wet
- Will stop early if moisture threshold reached
- Example: Window is 6-7 AM, but only waters if dry

**Moisture Only Mode**
- Schedules are ignored
- Waters anytime moisture drops below threshold

---

## 🔧 Advanced Tab

### Network Settings

**NTP Sync** (Toggle)
- Automatically synchronize time from internet
- Requires working network connection
- Updates hourly
- Keeps time accurate without RTC

### Logging Settings

**Data Logging** (Toggle)
- Enable/disable serial logging
- Logs to Serial Monitor
- Future: Will support SD card

**Log Interval** (minutes)
- How often to log data
- Default: 60 (every hour)
- Range: 1-1440 (24 hours)

### Danger Zone

**Winterize System** (Button)
- Runs winterization procedure
- Pulses each valve for 5 seconds
- Clears water from lines
- **Use before freezing weather**
- Confirmation required

### Saving Advanced Settings

Click **"Save Advanced Settings"**

All advanced configuration saves to EEPROM.

---

## 🎨 Understanding the UI Design

### Color Coding

**Blue** (#0ea5e9)
- Primary actions
- Active elements
- Headers and titles

**Green** (#10b981)
- Zone is watering
- Success messages
- Healthy moisture levels

**Yellow/Orange** (#f59e0b)
- Warnings
- Moderate moisture
- Attention needed

**Red** (#ef4444)
- Danger actions (winterize)
- Critical errors
- Dry moisture levels

### Visual Feedback

**Animations**
- Smooth transitions on all actions
- Pulsing dots for active zones
- Hover effects on interactive elements

**Responsive Design**
- Automatically adjusts to screen size
- Mobile-friendly interface
- Touch-optimized buttons

**Real-Time Updates**
- Status bar: every 5 seconds
- Zone data: every 10 seconds
- No page refresh needed!

---

## 📲 Mobile Usage Tips

### Landscape vs Portrait

**Portrait Mode** (Phone vertical)
- Zone cards stack vertically
- Easier scrolling
- Best for monitoring

**Landscape Mode** (Phone horizontal)
- Zone cards in grid
- More information visible
- Best for quick overview

### Touch Interactions

- **Tap** buttons to activate
- **Tap** zone card to select
- **Swipe** to scroll through zones
- **Pinch** to zoom (if needed)

### Adding to Home Screen

**iOS (iPhone/iPad)**
1. Open in Safari
2. Tap Share button
3. "Add to Home Screen"
4. Name it "Irrigation"

**Android**
1. Open in Chrome
2. Tap menu (⋮)
3. "Add to Home screen"
4. Name it "Irrigation"

Creates app-like icon for quick access!

---

## 🔍 Real-Time Monitoring

### Status Bar Updates

Every 5 seconds, the header updates with:
- Current temperature
- Current humidity
- Current time
- Number of active zones

**No page refresh needed** - uses AJAX for seamless updates.

### Zone Card Updates

Every 10 seconds, each zone card refreshes:
- Current moisture percentage
- ON/OFF status
- Today's watering time
- Visual moisture bar

Watch your zones in real-time as moisture changes!

---

## 💡 Common Usage Scenarios

### Scenario 1: Quick Zone Check

**Need**: See if front lawn is watering

1. Open web interface on phone
2. Look at Zones tab
3. Find "Front Lawn" card
4. Check status indicator
   - Green ON dot = watering now
   - Gray OFF dot = not watering
5. See moisture percentage and time running

**Time**: 5 seconds

### Scenario 2: Adjust Threshold

**Need**: Zone watering too often

1. Find zone card
2. Click "Configure"
3. Lower "Turn On Threshold"
   - Was 50% → Change to 40%
4. Click "Save Zone Configuration"
5. Done - waits until drier before watering

**Time**: 30 seconds

### Scenario 3: Emergency Shutoff

**Need**: Stop all watering immediately

**Option A**: Manual control per zone
1. Go to Zones tab
2. Click "Turn Off" on each active zone

**Option B**: Disable all zones
1. Go to each zone config
2. Toggle "Enabled" to OFF
3. Save each zone

**Option C**: Physical
- Remove power from relay board
- Fastest emergency option

### Scenario 4: Setup New Schedule

**Need**: Water roses every Monday/Wednesday at 7 AM for 15 minutes

1. Find "Roses" zone card
2. Click "Configure"
3. Set "Watering Mode" to "Time Only"
4. Save and close
5. Go to "Schedules" tab
6. Find Roses zone
7. Enable schedule
8. Set start time: 07:00
9. Set duration: 15
10. Check Monday and Wednesday
11. Save schedule

**Time**: 2 minutes

### Scenario 5: Calibrate New Sensor

**Need**: Just installed new sensor in Zone 5

1. Remove sensor from soil, let dry
2. Wait 10 minutes for reading to stabilize
3. Open Zone 5 config
4. Note current reading as "Dry Calibration"
5. Submerge sensor in water
6. Wait 2 minutes
7. Note reading as "Wet Calibration"
8. Enter both values in form
9. Save zone configuration

**Time**: 15 minutes (mostly waiting)

---

## 🔐 Security Considerations

### Network Access

**Current**: Open access on local network
- Anyone on your WiFi can access
- No password protection
- Suitable for home networks

**Best Practices**:
- Use secure WiFi with WPA2/WPA3
- Don't expose to internet without firewall
- Keep on isolated IoT network if possible

### Future Enhancements

Possible additions for security:
- Login/password protection
- HTTPS encryption
- API key authentication
- User access levels

For now: **Keep on trusted network only**

---

## 🐛 Web Interface Troubleshooting

### Can't Connect to IP Address

**Check**:
1. Is Arduino powered on?
2. Is network cable connected (Ethernet)?
3. Is WiFi connected? (check Serial Monitor)
4. Are you on same network?
5. Try http:// not https://
6. Is IP address correct?

**Fix**:
- Restart Arduino
- Check Serial Monitor for IP
- Verify network settings
- Try different device (phone vs laptop)

### Page Loads But No Data

**Symptoms**: 
- Blank zone cards
- All values show "--"
- No status updates

**Check**:
- Serial Monitor for errors
- Network connectivity
- Arduino isn't frozen

**Fix**:
- Refresh page (F5)
- Clear browser cache
- Restart Arduino
- Check API endpoints in browser:
  - http://IP/api/status
  - http://IP/api/zones

### Settings Don't Save

**Symptoms**:
- Click "Save" but values revert
- EEPROM errors in Serial Monitor

**Check**:
- EEPROM not corrupted
- Values are in valid ranges
- Arduino has enough memory

**Fix**:
- Reset to defaults (reload sketch with reset code uncommented)
- Check Serial Monitor for specific error
- Verify EEPROM addresses aren't overlapping

### Web Interface is Slow

**Causes**:
- Too many zones (16 is max)
- Network congestion
- Arduino processing load

**Fix**:
- Reduce number of active zones
- Increase update intervals
- Use wired Ethernet instead of WiFi
- Ensure adequate power supply

### Page Doesn't Update

**Symptoms**:
- Values never change
- Status stuck

**Check**:
- JavaScript errors (F12 in browser)
- Network connectivity
- Arduino still running

**Fix**:
- Hard refresh: Ctrl+Shift+R (Windows) or Cmd+Shift+R (Mac)
- Check browser console for errors
- Try different browser
- Restart Arduino

### Mobile Display Issues

**Problems**:
- Text too small
- Buttons hard to press
- Layout broken

**Fix**:
- Zoom in using pinch gesture
- Rotate to landscape
- Update browser app
- Clear browser cache
- Try different mobile browser

---

## 🎓 Advanced Web Features

### Direct API Access

You can interact with the controller using HTTP requests:

**Get System Status**
```bash
curl http://192.168.1.100/api/status
```

Returns:
```json
{
  "temperature": 72.5,
  "humidity": 65.0,
  "time": "09:30:45",
  "activeZones": 2
}
```

**Get All Zones**
```bash
curl http://192.168.1.100/api/zones
```

**Get Single Zone**
```bash
curl http://192.168.1.100/api/zone?id=0
```

**Update Zone**
```bash
curl -X POST http://192.168.1.100/api/zone \
  -H "Content-Type: application/json" \
  -d '{"id":0,"low":45,"high":75}'
```

**Toggle Zone**
```bash
curl -X POST http://192.168.1.100/api/control \
  -H "Content-Type: application/json" \
  -d '{"zone":0,"action":"toggle"}'
```

### Integration Possibilities

With the REST API, you can:

**Home Automation**
- Integrate with Home Assistant
- Connect to SmartThings
- Link with Alexa/Google Home (via bridge)

**Monitoring**
- Log data to external database
- Create custom dashboards
- Send alerts/notifications

**Automation**
- Python scripts for complex logic
- Integrate with weather APIs
- Schedule changes remotely

**Example Python Script**:
```python
import requests
import json

# Get current status
response = requests.get('http://192.168.1.100/api/status')
status = response.json()

print(f"Temperature: {status['temperature']}°F")
print(f"Active Zones: {status['activeZones']}")

# Turn on zone 3
requests.post(
    'http://192.168.1.100/api/control',
    json={'zone': 2, 'action': 'toggle'}
)
```

---

## 📊 Understanding Data Flow

### How Web Updates Work

```
Browser                Arduino
   │                      │
   │──── GET /api/zones ──>│
   │                      │
   │                   [Read sensors]
   │                   [Check EEPROM]
   │                   [Build JSON]
   │                      │
   │<──── JSON Response ──│
   │                      │
   [Update display]      │
   [Refresh cards]       │
```

**Every 10 seconds**, browser requests fresh data.  
**Arduino responds** with current readings.  
**Browser updates** cards without reload.

### How Settings Save

```
Browser                Arduino
   │                      │
   │── POST /api/zone ────>│
   │    {config data}     │
   │                      │
   │                   [Validate data]
   │                   [Update memory]
   │                   [Write EEPROM]
   │                      │
   │<──── {"success":true}│
   │                      │
   [Show confirmation]   │
```

**Settings instantly save to EEPROM** when you click Save.  
**Power loss won't lose your changes**.

---

## 🎯 Best Practices

### Configuration Workflow

1. **Start Simple**
   - Configure 1-2 zones first
   - Test moisture control
   - Verify thresholds work
   - Then add more zones

2. **Document Settings**
   - Take screenshots of working configs
   - Note which values work best
   - Keep backup of key thresholds

3. **Test Before Deploying**
   - Manually toggle zones
   - Watch moisture response
   - Verify schedules trigger correctly
   - Check freeze protection

4. **Monitor Initially**
   - Check web interface daily for first week
   - Look for unexpected behavior
   - Fine-tune thresholds as needed
   - Review daily watering totals

### Performance Tips

**Reduce Load**
- Disable unused zones
- Increase log interval (60+ minutes)
- Use Ethernet instead of WiFi (if possible)

**Maintain Reliability**
- Bookmark the IP address
- Keep Arduino powered continuously
- Use quality power supply
- Keep network stable

**Optimize Settings**
- Set realistic max durations
- Don't make thresholds too sensitive
- Space out schedules (not all at 6 AM)
- Monitor daily totals

---

## 🔄 Update Intervals

### Automatic Updates

**Status Bar**: 5 seconds
- Temperature
- Humidity  
- Time
- Active zone count

**Zone Cards**: 10 seconds
- Moisture readings
- ON/OFF status
- Today's totals
- Visual bars

**Configuration**: On demand
- Only updates when you open modal
- Saves immediately when you click Save

**Why these intervals?**
- Balance responsiveness vs load
- Moisture doesn't change that fast
- Reduces network traffic
- Keeps Arduino responsive

### Manual Refresh

Want fresh data immediately?
- **Refresh page**: F5 or pull-down-to-refresh
- **Re-open zone modal**: Gets latest calibration
- **Switch tabs**: Triggers update

---

## 📱 Comparing Web vs Physical Interface

### When to Use Web Interface

✅ **Best For**:
- Initial setup and configuration
- Detailed adjustments
- Monitoring from anywhere on network
- Changing schedules
- Batch configuration changes
- Checking status remotely
- Documentation (screenshots)

### When to Use Physical Buttons/Display

✅ **Best For**:
- Quick status check at controller
- When network is down
- Emergency manual override
- Winterization (can be done on web too)
- No phone/computer available
- Faster than pulling out phone

### Recommendation

**Setup**: Use web interface  
**Daily monitoring**: Either works  
**Emergency**: Physical buttons  
**Adjustments**: Web interface  
**Status check**: Physical display

---

## 🌟 Coming Soon (Potential V3.2 Features)

Based on user feedback, future web interface could add:

- **Live Charts**: Moisture graphs over time
- **Weather Integration**: Adjust based on forecast
- **Mobile App**: Native iOS/Android apps
- **Push Notifications**: Alerts when zones need attention
- **User Accounts**: Multiple users with permissions
- **Dark/Light Theme Toggle**: User preference
- **Advanced Scheduling**: Multiple schedules per zone
- **Data Export**: CSV download of logs
- **Batch Operations**: Change all zones at once
- **Zone Groups**: Control multiple zones together

---

**Enjoy your modern, web-based irrigation system! 🌱💧**

Questions? Check the main README or WIRING_GUIDE for more details.
