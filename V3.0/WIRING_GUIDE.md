# Wiring Diagram & Pin Configuration
## Irrigation Controller V3.0

---

## Complete Pin Assignment Table

| Function | Arduino Pin | Connected To | Notes |
|----------|------------|--------------|-------|
| **Display (SPI)** ||||
| Chip Select | 47 | OLED CS | |
| Data/Command | 49 | OLED DC | |
| Reset | 48 | OLED RST | |
| MOSI | 51 | OLED MOSI | Hardware SPI |
| SCK | 52 | OLED SCK | Hardware SPI |
| **I2C Devices** ||||
| SDA | 20 | Temp Sensor + RTC | |
| SCL | 21 | Temp Sensor + RTC | |
| **Buttons** ||||
| Menu Button | 22 | Pushbutton + 10kΩ pullup | INPUT_PULLUP |
| Up Button | 24 | Pushbutton + 10kΩ pullup | INPUT_PULLUP |
| Select Button | 26 | Pushbutton + 10kΩ pullup | INPUT_PULLUP |
| Down Button | 28 | Pushbutton + 10kΩ pullup | INPUT_PULLUP |
| **Soil Moisture Sensors** ||||
| Zone 1 Sensor | A0 | Capacitive sensor signal | |
| Zone 2 Sensor | A1 | Capacitive sensor signal | |
| Zone 3 Sensor | A2 | Capacitive sensor signal | |
| Zone 4 Sensor | A3 | Capacitive sensor signal | |
| Zone 5 Sensor | A4 | Capacitive sensor signal | |
| Zone 6 Sensor | A5 | Capacitive sensor signal | |
| Zone 7 Sensor | A6 | Capacitive sensor signal | |
| Zone 8 Sensor | A7 | Capacitive sensor signal | |
| Zone 9 Sensor | A8 | Capacitive sensor signal | |
| Zone 10 Sensor | A9 | Capacitive sensor signal | |
| Zone 11 Sensor | A10 | Capacitive sensor signal | |
| Zone 12 Sensor | A11 | Capacitive sensor signal | |
| Zone 13 Sensor | A12 | Capacitive sensor signal | |
| Zone 14 Sensor | A13 | Capacitive sensor signal | |
| Zone 15 Sensor | A14 | Capacitive sensor signal | |
| Zone 16 Sensor | A15 | Capacitive sensor signal | |
| **Relay Outputs** ||||
| Zone 1 Relay | 2 | Relay board IN1 | Controls valve 1 |
| Zone 2 Relay | 3 | Relay board IN2 | Controls valve 2 |
| Zone 3 Relay | 4 | Relay board IN3 | Controls valve 3 |
| Zone 4 Relay | 5 | Relay board IN4 | Controls valve 4 |
| Zone 5 Relay | 6 | Relay board IN5 | Controls valve 5 |
| Zone 6 Relay | 7 | Relay board IN6 | Controls valve 6 |
| Zone 7 Relay | 8 | Relay board IN7 | Controls valve 7 |
| Zone 8 Relay | 9 | Relay board IN8 | Controls valve 8 |
| Zone 9 Relay | 10 | Relay board IN9 | Controls valve 9 |
| Zone 10 Relay | 11 | Relay board IN10 | Controls valve 10 |
| Zone 11 Relay | 12 | Relay board IN11 | Controls valve 11 |
| Zone 12 Relay | 13 | Relay board IN12 | Controls valve 12 |
| Zone 13 Relay | 14 | Relay board IN13 | Controls valve 13 |
| Zone 14 Relay | 15 | Relay board IN14 | Controls valve 14 |
| Zone 15 Relay | 16 | Relay board IN15 | Controls valve 15 |
| Zone 16 Relay | 17 | Relay board IN16 | Controls valve 16 |
| **Network** ||||
| Ethernet CS | 53 | Ethernet shield | If using Ethernet |
| WiFi RX/TX | Varies | ESP module | If using WiFi - see notes |

---

## Detailed Component Connections

### 1. OLED Display (128x64 SSD1309)

**SPI Connection:**
```
Display Pin    →    Arduino Mega Pin
─────────────────────────────────────
VCC            →    5V
GND            →    GND
CS (Chip Sel)  →    47
DC (Data/Cmd)  →    49
RST (Reset)    →    48
MOSI (SDA)     →    51 (hardware SPI)
SCK (CLK)      →    52 (hardware SPI)
```

**Notes:**
- Make sure display is 5V tolerant or use level shifters
- Some displays may have slightly different labels (D0=SCK, D1=MOSI)
- Verify your specific display pinout

---

### 2. Temperature/Humidity Sensor (AM2315C)

**I2C Connection:**
```
Sensor Pin     →    Arduino Mega Pin
─────────────────────────────────────
VCC            →    5V
GND            →    GND
SDA            →    20 (I2C Data)
SCL            →    21 (I2C Clock)
```

**I2C Address:** 0x38 (default for AM2315C)

**Notes:**
- No pullup resistors needed (already on board)
- Can share I2C bus with RTC

---

### 3. RTC Module (DS3231) - Optional but Recommended

**I2C Connection:**
```
RTC Pin        →    Arduino Mega Pin
─────────────────────────────────────
VCC            →    5V
GND            →    GND
SDA            →    20 (shared with temp sensor)
SCL            →    21 (shared with temp sensor)
```

**I2C Address:** 0x68 (DS3231 default)

**Notes:**
- Install CR2032 battery for time backup
- Can share I2C bus with temperature sensor
- Not required if using NTP over network

---

### 4. Pushbuttons (4 required)

**Standard Button Wiring (with internal pullup):**
```
Each Button:
─────────────────────────────────────
One side       →    GND
Other side     →    Designated pin (22, 24, 26, or 28)

Button         →    Arduino Pin
─────────────────────────────────────
Menu           →    22
Up             →    24
Select         →    26
Down           →    28
```

**Alternative with External Pullups:**
```
Button         →    Pin → 10kΩ resistor → 5V
Other side     →    GND
```

**Notes:**
- Code uses INPUT_PULLUP mode (internal resistors)
- Use momentary pushbuttons
- Add 0.1µF capacitor across button for debouncing (optional)

---

### 5. Soil Moisture Sensors (up to 16)

**Recommended:** Capacitive sensors (more durable than resistive)

**Per Sensor Wiring:**
```
Sensor Pin     →    Connection
─────────────────────────────────────
VCC            →    5V (or 3.3V if specified)
GND            →    GND
AOUT           →    Analog pin (A0-A15)
```

**Zone Mapping:**
```
Zone 1  → A0     Zone 9  → A8
Zone 2  → A1     Zone 10 → A9
Zone 3  → A2     Zone 11 → A10
Zone 4  → A3     Zone 12 → A11
Zone 5  → A4     Zone 13 → A12
Zone 6  → A5     Zone 14 → A13
Zone 7  → A6     Zone 15 → A14
Zone 8  → A7     Zone 16 → A15
```

**Important Notes:**
- Keep sensor cables under 3 meters (10 feet) to avoid noise
- Use shielded cable for longer runs
- Each sensor needs independent power and ground
- Calibrate EVERY sensor individually

**Recommended Sensors:**
- Capacitive soil moisture sensor v1.2
- Gravity analog capacitive sensor
- DFRobot SEN0193

**Avoid:**
- Cheap resistive fork sensors (corrode quickly)

---

### 6. Relay Board (16-channel)

**Per Relay Connection:**
```
Relay Board Pin →   Connection
─────────────────────────────────────
VCC             →   12V DC (separate supply!)
GND             →   Common ground with Arduino
INx (IN1-IN16)  →   Arduino pins 2-17

Zone 1  → Pin 2      Zone 9  → Pin 10
Zone 2  → Pin 3      Zone 10 → Pin 11
Zone 3  → Pin 4      Zone 11 → Pin 12
Zone 4  → Pin 5      Zone 12 → Pin 13
Zone 5  → Pin 6      Zone 13 → Pin 14
Zone 6  → Pin 7      Zone 14 → Pin 15
Zone 7  → Pin 8      Zone 15 → Pin 16
Zone 8  → Pin 9      Zone 16 → Pin 17
```

**Relay to Valve Wiring:**
```
For NC (Normally Closed) Relays:
─────────────────────────────────────
Common (COM)   →    24VAC from transformer
NO (Normally Open) → (not used)
NC (Normally Closed) → Valve solenoid wire 1
Valve wire 2   →    24VAC common return
```

**Critical Notes:**
- ALWAYS use separate power for relay board
- Connect Arduino GND to relay board GND
- Use adequate gauge wire for valve current
- Most sprinkler valves need 24VAC, 0.5A each
- Never power relays from Arduino 5V pin!

**Power Supply Sizing:**
- 16 relays × 70mA = 1.12A minimum
- Recommend: 12V 2A power supply
- Use barrel jack or screw terminals

**Optocoupler Isolation:**
Most relay boards have optocouplers for protection. If yours doesn't:
- Add 1kΩ resistor between Arduino pin and relay IN
- Consider buying optocoupled relay board

---

### 7. Network Connection

#### Option A: WiFi (ESP32 or ESP8266)

**ESP32 Wiring:**
```
ESP32 Pin      →    Arduino Mega Pin
─────────────────────────────────────
TX             →    RX1 (19)
RX             →    TX1 (18)
GND            →    GND
VCC (3.3V!)    →    3.3V
EN             →    3.3V (via 10kΩ)
```

**Important WiFi Notes:**
- ESP32/ESP8266 are 3.3V devices!
- Use level shifters for RX/TX if needed
- Or use ESP32 as main controller (code modifications needed)
- Alternative: Use ESP-01 with AT commands

#### Option B: Ethernet Shield

**Ethernet Shield Connections:**
```
Shield Pin     →    Arduino Mega Pin
─────────────────────────────────────
CS             →    53 (default)
MOSI           →    51
MISO           →    50
SCK            →    52
```

**Notes:**
- Shield plugs directly into Mega
- Verify shield doesn't conflict with other pins
- Some shields use pin 10 for CS (change in code)
- Connect Ethernet cable to RJ45 jack

---

## Power Supply Design

### Option 1: Single Supply System
```
12V 3A DC Power Supply
├── Buck converter → 7V → Arduino Mega VIN
└── Direct → Relay board VCC
```

### Option 2: Dual Supply System (Recommended)
```
12V 2A DC Supply #1 → Relay board VCC
9V 1A DC Supply #2  → Arduino Mega VIN
```

### Option 3: With Valve Transformer
```
24VAC Transformer → Sprinkler valves
12V DC Supply → Arduino + Relays
```

**Ground Connections:**
- Connect all GND together (Arduino, relay, sensors)
- Relay board GND must connect to Arduino GND
- AC circuit ground is separate (for valves)

---

## Enclosure Layout Recommendations

### Weatherproof Enclosure
```
┌─────────────────────────────────┐
│  [Display Window]               │
│                                 │
│  [Button Panel]                 │
│  ┌────────────────────────────┐ │
│  │                            │ │
│  │  Arduino Mega              │ │
│  │                            │ │
│  └────────────────────────────┘ │
│                                 │
│  ┌────────────────────────────┐ │
│  │  16-Channel Relay Board    │ │
│  └────────────────────────────┘ │
│                                 │
│  [Power Supplies]               │
│                                 │
│  [Terminal Blocks - Sensors]   │
│  [Terminal Blocks - Valves]    │
└─────────────────────────────────┘

Cable Entries:
├── Top: Antenna (if WiFi)
├── Bottom Left: Sensor cables
├── Bottom Right: Valve wires
└── Bottom Center: Power input
```

**Mounting Tips:**
- Use DIN rail for Arduino and relays
- Add cooling fan if >10 zones or hot climate
- Include desiccant pack to prevent moisture
- Use weatherproof cable glands
- Mount away from direct sun
- Keep above flood level

---

## Cable Management

### Sensor Cables
- Use CAT5e ethernet cable (cheap, 8 conductors)
- Each cable can run 2 sensors (VCC, GND, Signal, Signal)
- Label clearly at both ends
- Use different colors for different zones

### Valve Wires
- Standard 18AWG irrigation wire
- Bundle together in conduit
- Waterproof connectors at valve end
- Terminal blocks at control box

### Wire Colors (Suggested Standard)
```
Power:
- Red: +12V
- Black: GND
- Yellow: +5V

Sensors:
- Red: VCC
- Black: GND
- White: Signal

Valves:
- Use numbered/colored irrigation wire
- Keep wire map documented
```

---

## Common Wiring Mistakes to Avoid

❌ **DON'T:**
1. Power relays from Arduino 5V pin (will damage board)
2. Forget to connect grounds between modules
3. Use resistive moisture sensors (they corrode)
4. Run sensor cables next to AC power lines
5. Connect 5V to 3.3V devices (ESP32/ESP8266)
6. Skip fuses on power supplies
7. Mount outdoor without weatherproof enclosure

✅ **DO:**
1. Use separate power supply for relays
2. Connect all grounds together
3. Use capacitive moisture sensors
4. Keep sensor cables away from power
5. Use level shifters where needed
6. Install fuses/breakers on all power
7. Use NEMA 4 rated enclosure outdoors

---

## Testing Checklist

### Before First Power-On:
- [ ] Check all power supply polarities
- [ ] Verify no shorts between power and ground
- [ ] Confirm relay board has separate power
- [ ] Double-check all pin connections
- [ ] Verify display wiring (CS=47, DC=49, RST=48)
- [ ] Test each button with multimeter (continuity)

### Initial Power-On:
- [ ] Arduino powers up (LED on)
- [ ] Display shows startup screen
- [ ] Buttons respond when pressed
- [ ] Temperature sensor reads reasonable value
- [ ] All moisture sensors show readings

### Functional Testing:
- [ ] Each relay clicks when activated
- [ ] Each valve opens when relay activates
- [ ] No valves leak when off
- [ ] Moisture readings change when wet/dry
- [ ] Menu system navigates properly
- [ ] Network connects (if enabled)
- [ ] Time displays correctly

---

## Expansion Options

### Adding More Than 16 Zones

**Option 1: I2C GPIO Expander**
```
Use MCP23017 (16 GPIO over I2C)
Can cascade up to 8 chips = 128 zones!
Requires code modification
```

**Option 2: Shift Registers**
```
74HC595 (8-bit shift register)
Cascade for 32, 64+ zones
Uses only 3 Arduino pins
Requires code modification
```

**Option 3: Multiple Controllers**
```
Deploy multiple Mega units
Each handles 16 zones
Can network together
```

### Future Sensor Additions

**Available Pins for Expansion:**
- Digital: 30-49 (if not using all relays)
- Analog: A0-A15 (shared with moisture sensors)
- I2C: Unlimited devices via address (SDA/SCL)

**Suggested Additions:**
- Rain sensor (digital input)
- Flow meter (pulse counter)
- Light sensor (analog input)
- pH sensor (analog input)
- Water level sensor (analog input)

---

## Troubleshooting Wiring Issues

| Symptom | Likely Cause | Check |
|---------|-------------|-------|
| No display | Wrong SPI pins | Verify CS=47, DC=49, RST=48 |
| Garbled display | Bad MOSI/SCK | Check pins 51, 52 |
| No temp reading | I2C wiring | Verify SDA=20, SCL=21 |
| Buttons don't work | No pullup | Code has INPUT_PULLUP |
| All moisture reads 0 | No sensor power | Check 5V to sensors |
| Random relay activation | Ground issue | Connect relay GND to Arduino GND |
| Arduino resets | Power insufficient | Use proper power supply |
| WiFi won't connect | 3.3V power issue | ESP32 needs 3.3V |

---

## Safety Warnings

⚠️ **ELECTRICAL SAFETY:**
- Never work on wiring while powered
- Use GFCI protection for outdoor installations
- Keep 24VAC valve wiring separate from control wiring
- Install proper fuses/breakers
- Follow local electrical codes

⚠️ **WATER DAMAGE:**
- Use weatherproof enclosure (NEMA 4 minimum)
- Keep electronics above potential flood level
- Install drainage in enclosure
- Protect cable entries with grommets

⚠️ **FIRE HAZARD:**
- Never exceed relay/wire current ratings
- Use proper gauge wire for current
- Don't bundle too many power wires together
- Install in well-ventilated enclosure
- Consider adding heat sensors/alarms

---

## Recommended Components List

### Essential:
- Arduino Mega 2560
- SSD1309 128x64 OLED (SPI)
- AM2315C temperature sensor
- 4x tactile pushbuttons
- 16x capacitive soil moisture sensors
- 16-channel relay board (12V)
- 12V 2A power supply (relays)
- 9V 1A power supply (Arduino)

### Recommended:
- DS3231 RTC module
- Ethernet shield OR ESP32
- 24VAC 40VA transformer (for valves)
- Weatherproof NEMA enclosure
- Terminal blocks
- DIN rail mounting
- Cooling fan (12V)

### Optional:
- Rain sensor
- Flow meters
- Additional power monitoring
- UPS/battery backup
- Remote alarm/notification system

---

**End of Wiring Guide**

Always test components individually before final assembly!
Document your specific wiring with photos for future reference!
