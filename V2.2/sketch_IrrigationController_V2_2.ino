/*
  Soil Moisture Irrigation Controller
  Version 2.2 CONFIGURABLE
  
  Based on original by Jack Mamiye & Anna Sweeney
  Menu system implementation added
  
  *** FULLY CONFIGURABLE VERSION ***
  All settings accessible via 4-button menu system:
  - Number of zones (1-16)
  - Temperature display (C/F)
  - Relay type (NO/NC)
  - Per-zone calibration (dry/wet)
  - Per-zone thresholds (low/high %)
  - High temperature adjustment
  - Winterize function
  
  Hardware: Arduino Mega 2560, 128x64 OLED (SSD1309), AM2315C sensor
  Buttons: Menu(22), Up(24), Select(26), Down(28)
*/

#include "AM2315C.h"
#include <Arduino.h>
#include <U8g2lib.h>
#include <EEPROM.h>
#include <SPI.h>
#include <Wire.h>

// Hardware
AM2315C DHT;
U8X8_SSD1309_128X64_NONAME0_4W_HW_SPI u8x8(47, 49, 48);

// Button pins
const int bPinMenu = 22;
const int bPinUp = 24;
const int bPinSelect = 26;
const int bPinDown = 28;

#define DEBOUNCE_MS 150
#define LONG_PRESS_MS 1500

// Analog pins for sensors
const int analogPins[16] = {A0,A1,A2,A3,A4,A5,A6,A7,A8,A9,A10,A11,A12,A13,A14,A15};

// ============================================================================
// CONFIGURATION STRUCTURE
// ============================================================================

struct Config {
  uint8_t zones;        // 1-16
  uint8_t tempDispF;    // 0=Celsius, 1=Fahrenheit
  uint8_t relayOn;      // 0=LOW activates, 1=HIGH activates
  uint8_t tempSwitch;   // Temperature threshold for adjustment
  uint8_t tempAdjLow;   // Add to low threshold when hot
  uint8_t tempAdjHi;    // Add to high threshold when hot
  uint16_t checksum;    // For validation
};

struct ZoneData {
  uint16_t dryCal;      // ADC value for 0% moisture
  uint16_t wetCal;      // ADC value for 100% moisture
  uint8_t lowThresh;    // Turn ON below this %
  uint8_t highThresh;   // Turn OFF above this %
};

Config config;
ZoneData zoneData[16];

#define EEPROM_CONFIG_ADDR 0
#define EEPROM_ZONES_ADDR 32

// ============================================================================
// RUNTIME VARIABLES
// ============================================================================

int soilMoisture[16];
bool relayStatus[16];
float tempC = 20.0, tempF = 68.0, humidity = 50.0;

// Menu system
enum MenuState {
  MENU_RUN,           // Normal operation display
  MENU_MAIN,          // Main menu
  MENU_ZONES,         // Number of zones
  MENU_TEMP_UNIT,     // C or F
  MENU_RELAY_TYPE,    // NO or NC
  MENU_ZONE_SELECT,   // Select zone to configure
  MENU_ZONE_CONFIG,   // Configure selected zone
  MENU_ZONE_CAL_DRY,  // Calibrate dry
  MENU_ZONE_CAL_WET,  // Calibrate wet
  MENU_ZONE_LOW,      // Low threshold
  MENU_ZONE_HIGH,     // High threshold
  MENU_TEMP_SWITCH,   // High temp threshold
  MENU_TEMP_ADJ,      // Temperature adjustments
  MENU_WINTERIZE,     // Winterize confirmation
  MENU_SAVE           // Save confirmation
};

MenuState menuState = MENU_RUN;
uint8_t menuCursor = 0;
uint8_t selectedZone = 0;
uint8_t editValue = 0;
bool editing = false;

unsigned long lastButtonTime = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastSensorRead = 0;

// ============================================================================
// FUNCTION PROTOTYPES
// ============================================================================

void loadConfig();
void saveConfig();
void setDefaults();
uint16_t calcChecksum();
void readSensors();
void updateIrrigation();
void handleButtons();
void updateDisplay();
void drawRunScreen();
void drawMainMenu();
void drawZoneSelect();
void drawZoneConfig();
void drawValueEdit(const char* title, int value, const char* unit, int minVal, int maxVal);
void winterize();

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  Serial.begin(115200);
  Serial.println(F("\n=== Irrigation Controller V2.2 ==="));
  
  // Initialize display
  u8x8.begin();
  u8x8.setPowerSave(0);
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.clear();
  u8x8.print(F("Irrigation V2.2"));
  u8x8.setCursor(0, 2);
  u8x8.print(F("Initializing..."));
  
  // Initialize I2C and sensor
  Wire.begin();
  DHT.begin();
  
  // Initialize buttons
  pinMode(bPinMenu, INPUT_PULLUP);
  pinMode(bPinUp, INPUT_PULLUP);
  pinMode(bPinSelect, INPUT_PULLUP);
  pinMode(bPinDown, INPUT_PULLUP);
  
  // Initialize analog inputs
  for (int i = 0; i < 16; i++) {
    pinMode(analogPins[i], INPUT);
  }
  
  // Load configuration
  loadConfig();
  
  // Initialize relay outputs
  for (int i = 0; i < config.zones; i++) {
    pinMode(i + 2, OUTPUT);
    digitalWrite(i + 2, !config.relayOn);  // All OFF
    relayStatus[i] = false;
  }
  
  // Initial sensor read
  readSensors();
  
  u8x8.clear();
  u8x8.print(F("Ready!"));
  u8x8.setCursor(0, 2);
  u8x8.print(F("MENU for config"));
  delay(2000);
  
  Serial.print(F("Zones: ")); Serial.println(config.zones);
  Serial.print(F("Relay type: ")); Serial.println(config.relayOn ? "NO" : "NC");
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
  unsigned long now = millis();
  
  // Read sensors every 2 seconds
  if (now - lastSensorRead >= 2000) {
    readSensors();
    lastSensorRead = now;
    
    // Only run irrigation logic in RUN mode
    if (menuState == MENU_RUN) {
      updateIrrigation();
    }
  }
  
  // Handle button input
  handleButtons();
  
  // Update display every 500ms
  if (now - lastDisplayUpdate >= 500) {
    updateDisplay();
    lastDisplayUpdate = now;
  }
}

// ============================================================================
// CONFIGURATION MANAGEMENT
// ============================================================================

void loadConfig() {
  EEPROM.get(EEPROM_CONFIG_ADDR, config);
  
  // Validate checksum
  uint16_t stored = config.checksum;
  config.checksum = 0;
  if (calcChecksum() != stored || config.zones == 0 || config.zones > 16) {
    Serial.println(F("Invalid config - loading defaults"));
    setDefaults();
    saveConfig();
  } else {
    // Load zone data
    for (int i = 0; i < 16; i++) {
      EEPROM.get(EEPROM_ZONES_ADDR + i * sizeof(ZoneData), zoneData[i]);
    }
    Serial.println(F("Config loaded from EEPROM"));
  }
}

void saveConfig() {
  config.checksum = 0;
  config.checksum = calcChecksum();
  EEPROM.put(EEPROM_CONFIG_ADDR, config);
  
  for (int i = 0; i < 16; i++) {
    EEPROM.put(EEPROM_ZONES_ADDR + i * sizeof(ZoneData), zoneData[i]);
  }
  Serial.println(F("Config saved to EEPROM"));
}

void setDefaults() {
  config.zones = 6;
  config.tempDispF = 1;
  config.relayOn = 0;
  config.tempSwitch = 92;
  config.tempAdjLow = 6;
  config.tempAdjHi = 4;
  
  for (int i = 0; i < 16; i++) {
    zoneData[i].dryCal = 125;
    zoneData[i].wetCal = 550;
    zoneData[i].lowThresh = 40;
    zoneData[i].highThresh = 70;
  }
}

uint16_t calcChecksum() {
  uint16_t sum = 0;
  uint8_t* ptr = (uint8_t*)&config;
  for (size_t i = 0; i < sizeof(Config); i++) {
    sum += ptr[i];
  }
  return sum ^ 0xA5A5;
}

// ============================================================================
// SENSOR READING
// ============================================================================

void readSensors() {
  // Read temperature/humidity
  if (DHT.read() == AM2315C_OK) {
    tempC = DHT.getTemperature();
    tempF = tempC * 9.0 / 5.0 + 32.0;
    humidity = DHT.getHumidity();
  }
  
  // Read soil moisture for each zone
  for (int i = 0; i < config.zones; i++) {
    long raw = analogRead(analogPins[i]);
    
    // Constrain to calibration range
    if (raw < zoneData[i].dryCal) raw = zoneData[i].dryCal;
    if (raw > zoneData[i].wetCal) raw = zoneData[i].wetCal;
    
    // Map to percentage
    soilMoisture[i] = map(raw, zoneData[i].dryCal, zoneData[i].wetCal, 0, 100);
  }
}

// ============================================================================
// IRRIGATION CONTROL
// ============================================================================

void updateIrrigation() {
  float currentTemp = config.tempDispF ? tempF : tempC;
  bool isHot = (currentTemp >= config.tempSwitch);
  
  for (int i = 0; i < config.zones; i++) {
    int effectiveLow = zoneData[i].lowThresh;
    int effectiveHigh = zoneData[i].highThresh;
    
    // Adjust thresholds for high temperature
    if (isHot) {
      effectiveLow += config.tempAdjLow;
      effectiveHigh += config.tempAdjHi;
    }
    
    // Control logic
    if (soilMoisture[i] <= effectiveLow && !relayStatus[i]) {
      // Turn ON
      digitalWrite(i + 2, config.relayOn);
      relayStatus[i] = true;
      Serial.print(F("Zone ")); Serial.print(i+1); Serial.println(F(" ON"));
    }
    else if (soilMoisture[i] >= effectiveHigh && relayStatus[i]) {
      // Turn OFF
      digitalWrite(i + 2, !config.relayOn);
      relayStatus[i] = false;
      Serial.print(F("Zone ")); Serial.print(i+1); Serial.println(F(" OFF"));
    }
  }
}

// ============================================================================
// BUTTON HANDLING
// ============================================================================

void handleButtons() {
  if (millis() - lastButtonTime < DEBOUNCE_MS) return;
  
  bool menuBtn = !digitalRead(bPinMenu);
  bool upBtn = !digitalRead(bPinUp);
  bool downBtn = !digitalRead(bPinDown);
  bool selectBtn = !digitalRead(bPinSelect);
  
  if (!menuBtn && !upBtn && !downBtn && !selectBtn) return;
  
  lastButtonTime = millis();
  
  // MENU button - back/exit
  if (menuBtn) {
    if (editing) {
      editing = false;  // Cancel edit
    } else if (menuState == MENU_RUN) {
      menuState = MENU_MAIN;
      menuCursor = 0;
    } else if (menuState == MENU_MAIN) {
      menuState = MENU_RUN;
    } else if (menuState == MENU_ZONE_CONFIG) {
      menuState = MENU_ZONE_SELECT;
    } else {
      menuState = MENU_MAIN;
      menuCursor = 0;
    }
    return;
  }
  
  // UP button
  if (upBtn) {
    if (editing) {
      editValue++;
    } else if (menuCursor > 0) {
      menuCursor--;
    }
    return;
  }
  
  // DOWN button
  if (downBtn) {
    if (editing) {
      if (editValue > 0) editValue--;
    } else {
      menuCursor++;
    }
    return;
  }
  
  // SELECT button
  if (selectBtn) {
    handleSelect();
  }
}

void handleSelect() {
  switch (menuState) {
    case MENU_RUN:
      menuState = MENU_MAIN;
      menuCursor = 0;
      break;
      
    case MENU_MAIN:
      switch (menuCursor) {
        case 0: menuState = MENU_ZONES; editValue = config.zones; editing = true; break;
        case 1: menuState = MENU_TEMP_UNIT; editValue = config.tempDispF; editing = true; break;
        case 2: menuState = MENU_RELAY_TYPE; editValue = config.relayOn; editing = true; break;
        case 3: menuState = MENU_ZONE_SELECT; menuCursor = 0; break;
        case 4: menuState = MENU_TEMP_SWITCH; editValue = config.tempSwitch; editing = true; break;
        case 5: menuState = MENU_TEMP_ADJ; menuCursor = 0; break;
        case 6: menuState = MENU_WINTERIZE; break;
        case 7: menuState = MENU_SAVE; break;
      }
      break;
      
    case MENU_ZONES:
      if (editing) {
        if (editValue >= 1 && editValue <= 16) config.zones = editValue;
        editing = false;
        menuState = MENU_MAIN;
      }
      break;
      
    case MENU_TEMP_UNIT:
      if (editing) {
        config.tempDispF = editValue & 1;
        editing = false;
        menuState = MENU_MAIN;
      }
      break;
      
    case MENU_RELAY_TYPE:
      if (editing) {
        config.relayOn = editValue & 1;
        editing = false;
        menuState = MENU_MAIN;
      }
      break;
      
    case MENU_ZONE_SELECT:
      if (menuCursor < config.zones) {
        selectedZone = menuCursor;
        menuState = MENU_ZONE_CONFIG;
        menuCursor = 0;
      }
      break;
      
    case MENU_ZONE_CONFIG:
      switch (menuCursor) {
        case 0:  // Calibrate Dry
          zoneData[selectedZone].dryCal = analogRead(analogPins[selectedZone]);
          Serial.print(F("Zone ")); Serial.print(selectedZone+1);
          Serial.print(F(" DryCal: ")); Serial.println(zoneData[selectedZone].dryCal);
          break;
        case 1:  // Calibrate Wet
          zoneData[selectedZone].wetCal = analogRead(analogPins[selectedZone]);
          Serial.print(F("Zone ")); Serial.print(selectedZone+1);
          Serial.print(F(" WetCal: ")); Serial.println(zoneData[selectedZone].wetCal);
          break;
        case 2:  // Low threshold
          menuState = MENU_ZONE_LOW;
          editValue = zoneData[selectedZone].lowThresh;
          editing = true;
          break;
        case 3:  // High threshold
          menuState = MENU_ZONE_HIGH;
          editValue = zoneData[selectedZone].highThresh;
          editing = true;
          break;
      }
      break;
      
    case MENU_ZONE_LOW:
      if (editing) {
        if (editValue <= 100) zoneData[selectedZone].lowThresh = editValue;
        editing = false;
        menuState = MENU_ZONE_CONFIG;
      }
      break;
      
    case MENU_ZONE_HIGH:
      if (editing) {
        if (editValue <= 100) zoneData[selectedZone].highThresh = editValue;
        editing = false;
        menuState = MENU_ZONE_CONFIG;
      }
      break;
      
    case MENU_TEMP_SWITCH:
      if (editing) {
        config.tempSwitch = editValue;
        editing = false;
        menuState = MENU_MAIN;
      }
      break;
      
    case MENU_TEMP_ADJ:
      if (menuCursor == 0) {
        editValue = config.tempAdjLow;
        editing = true;
        // Inline edit for simplicity
      } else if (menuCursor == 1) {
        editValue = config.tempAdjHi;
        editing = true;
      }
      if (editing && selectBtn) {
        if (menuCursor == 0) config.tempAdjLow = editValue;
        else config.tempAdjHi = editValue;
        editing = false;
      }
      break;
      
    case MENU_WINTERIZE:
      winterize();
      menuState = MENU_MAIN;
      break;
      
    case MENU_SAVE:
      saveConfig();
      menuState = MENU_RUN;
      break;
  }
}

// ============================================================================
// DISPLAY
// ============================================================================

void updateDisplay() {
  switch (menuState) {
    case MENU_RUN: drawRunScreen(); break;
    case MENU_MAIN: drawMainMenu(); break;
    case MENU_ZONES: drawValueEdit("Num Zones", editValue, "", 1, 16); break;
    case MENU_TEMP_UNIT: drawValueEdit("Temp Unit", editValue, editValue ? "F" : "C", 0, 1); break;
    case MENU_RELAY_TYPE: drawValueEdit("Relay Type", editValue, editValue ? "NO" : "NC", 0, 1); break;
    case MENU_ZONE_SELECT: drawZoneSelect(); break;
    case MENU_ZONE_CONFIG: drawZoneConfig(); break;
    case MENU_ZONE_LOW: drawValueEdit("Low Thresh", editValue, "%", 0, 100); break;
    case MENU_ZONE_HIGH: drawValueEdit("High Thresh", editValue, "%", 0, 100); break;
    case MENU_TEMP_SWITCH: drawValueEdit("Temp Switch", editValue, config.tempDispF ? "F" : "C", 0, 150); break;
    case MENU_TEMP_ADJ: 
      u8x8.clear();
      u8x8.print(F("Temp Adjust"));
      u8x8.setCursor(0, 2);
      u8x8.print(menuCursor == 0 ? ">" : " ");
      u8x8.print(F("Low +"));
      u8x8.print(config.tempAdjLow);
      u8x8.print("%");
      u8x8.setCursor(0, 3);
      u8x8.print(menuCursor == 1 ? ">" : " ");
      u8x8.print(F("High +"));
      u8x8.print(config.tempAdjHi);
      u8x8.print("%");
      break;
    case MENU_WINTERIZE:
      u8x8.clear();
      u8x8.print(F("WINTERIZE?"));
      u8x8.setCursor(0, 2);
      u8x8.print(F("Press SELECT"));
      u8x8.setCursor(0, 3);
      u8x8.print(F("to confirm"));
      u8x8.setCursor(0, 5);
      u8x8.print(F("MENU to cancel"));
      break;
    case MENU_SAVE:
      u8x8.clear();
      u8x8.print(F("SAVE CONFIG?"));
      u8x8.setCursor(0, 2);
      u8x8.print(F("Press SELECT"));
      u8x8.setCursor(0, 3);
      u8x8.print(F("to save EEPROM"));
      break;
  }
}

void drawRunScreen() {
  u8x8.clear();
  
  // Header with temp/humidity
  u8x8.setCursor(0, 0);
  u8x8.print(F("T:"));
  u8x8.print(config.tempDispF ? (int)tempF : (int)tempC);
  u8x8.print(config.tempDispF ? "F" : "C");
  u8x8.print(F(" RH:"));
  u8x8.print((int)humidity);
  u8x8.print("%");
  
  // Zone status (up to 6 visible)
  int visible = min((int)config.zones, 6);
  for (int i = 0; i < visible; i++) {
    u8x8.setCursor(0, i + 1);
    u8x8.print(i + 1);
    u8x8.print(":");
    if (soilMoisture[i] < 10) u8x8.print(" ");
    if (soilMoisture[i] < 100) u8x8.print(" ");
    u8x8.print(soilMoisture[i]);
    u8x8.print("% ");
    u8x8.print(relayStatus[i] ? "ON " : "OFF");
  }
  
  u8x8.setCursor(0, 7);
  u8x8.print(F("MENU=Config"));
}

void drawMainMenu() {
  const char* items[] = {
    "Num Zones",
    "Temp Unit",
    "Relay Type",
    "Zone Config",
    "Temp Switch",
    "Temp Adjust",
    "Winterize",
    "Save & Exit"
  };
  const int numItems = 8;
  
  if (menuCursor >= numItems) menuCursor = numItems - 1;
  
  u8x8.clear();
  u8x8.print(F("=== CONFIG ==="));
  
  int start = (menuCursor > 4) ? menuCursor - 4 : 0;
  for (int i = 0; i < 6 && (start + i) < numItems; i++) {
    u8x8.setCursor(0, i + 1);
    u8x8.print((start + i) == menuCursor ? ">" : " ");
    u8x8.print(items[start + i]);
  }
  
  u8x8.setCursor(0, 7);
  u8x8.print(F("UP/DN SEL MENU"));
}

void drawZoneSelect() {
  if (menuCursor >= config.zones) menuCursor = config.zones - 1;
  
  u8x8.clear();
  u8x8.print(F("Select Zone:"));
  
  int start = (menuCursor > 4) ? menuCursor - 4 : 0;
  for (int i = 0; i < 6 && (start + i) < config.zones; i++) {
    u8x8.setCursor(0, i + 1);
    u8x8.print((start + i) == menuCursor ? ">" : " ");
    u8x8.print(F("Zone "));
    u8x8.print(start + i + 1);
    u8x8.print(F(" ["));
    u8x8.print(soilMoisture[start + i]);
    u8x8.print(F("%]"));
  }
}

void drawZoneConfig() {
  const char* items[] = {"Set Dry Cal", "Set Wet Cal", "Low Thresh", "High Thresh"};
  if (menuCursor > 3) menuCursor = 3;
  
  u8x8.clear();
  u8x8.print(F("Zone "));
  u8x8.print(selectedZone + 1);
  u8x8.print(F(" Config"));
  
  u8x8.setCursor(0, 1);
  u8x8.print(F("Dry:"));
  u8x8.print(zoneData[selectedZone].dryCal);
  u8x8.print(F(" Wet:"));
  u8x8.print(zoneData[selectedZone].wetCal);
  
  for (int i = 0; i < 4; i++) {
    u8x8.setCursor(0, i + 2);
    u8x8.print(i == menuCursor ? ">" : " ");
    u8x8.print(items[i]);
    if (i == 2) { u8x8.print(":"); u8x8.print(zoneData[selectedZone].lowThresh); u8x8.print("%"); }
    if (i == 3) { u8x8.print(":"); u8x8.print(zoneData[selectedZone].highThresh); u8x8.print("%"); }
  }
  
  u8x8.setCursor(0, 7);
  u8x8.print(F("Now:"));
  u8x8.print(analogRead(analogPins[selectedZone]));
}

void drawValueEdit(const char* title, int value, const char* unit, int minVal, int maxVal) {
  // Constrain edit value
  if (editValue < minVal) editValue = minVal;
  if (editValue > maxVal) editValue = maxVal;
  
  u8x8.clear();
  u8x8.print(title);
  
  u8x8.setCursor(0, 3);
  u8x8.print(F("  < "));
  u8x8.print(value);
  u8x8.print(" ");
  u8x8.print(unit);
  u8x8.print(F(" >"));
  
  u8x8.setCursor(0, 5);
  u8x8.print(F("UP/DN to change"));
  u8x8.setCursor(0, 6);
  u8x8.print(F("SELECT to confirm"));
  u8x8.setCursor(0, 7);
  u8x8.print(F("MENU to cancel"));
}

// ============================================================================
// WINTERIZE
// ============================================================================

void winterize() {
  u8x8.clear();
  u8x8.print(F("WINTERIZING..."));
  Serial.println(F("=== WINTERIZE ==="));
  
  for (int i = 0; i < config.zones; i++) {
    u8x8.setCursor(0, 2);
    u8x8.print(F("Zone "));
    u8x8.print(i + 1);
    u8x8.print(F(" ON  "));
    
    digitalWrite(i + 2, config.relayOn);
    delay(5000);
    digitalWrite(i + 2, !config.relayOn);
    
    u8x8.setCursor(0, 2);
    u8x8.print(F("Zone "));
    u8x8.print(i + 1);
    u8x8.print(F(" Done"));
    delay(1000);
  }
  
  u8x8.clear();
  u8x8.print(F("WINTERIZE"));
  u8x8.setCursor(0, 2);
  u8x8.print(F("COMPLETE!"));
  delay(3000);
}
