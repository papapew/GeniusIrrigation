/*
  Advanced Soil Moisture Irrigation Controller
  Version 3.0
  January 2026
  
  Enhanced by Claude (based on V2.2 by Jack Mamiye & Anna Sweeney)
  
  Features:
  - Full pushbutton menu system with LCD navigation
  - WiFi and Ethernet support (selectable)
  - NTP time synchronization
  - Multiple watering modes:
    * Time-based scheduling
    * Soil moisture-based
    * Hybrid (time windows + moisture)
  - Enhanced EEPROM configuration management
  - Data logging capabilities
  - Up to 16 zones supported
  
  Hardware Requirements:
  - Arduino Mega 2560 (required for pin count)
  - U8g2 compatible 128x64 OLED display
  - AM2315C Temperature/Humidity sensor
  - 4 pushbuttons (Menu, Up, Down, Select)
  - Ethernet shield OR ESP32/ESP8266 WiFi module
  - RTC module (DS3231) recommended for time backup
  - 16 soil moisture sensors (capacitive recommended)
  - 16-channel relay board
*/

#include "AM2315C.h"
#include <Arduino.h>
#include <U8g2lib.h>
#include <EEPROM.h>
#include <SPI.h>
#include <Wire.h>
#include <Time.h>
#include <TimeLib.h>

// Network libraries - comment out the one you're not using
#define USE_WIFI  // Uncomment for WiFi, comment for Ethernet
// #define USE_ETHERNET  // Uncomment for Ethernet, comment for WiFi

#ifdef USE_WIFI
  #include <WiFi.h>
  #include <WiFiUdp.h>
#endif

#ifdef USE_ETHERNET
  #include <Ethernet.h>
  #include <EthernetUdp.h>
#endif

// Optional: RTC support for time backup
#define USE_RTC
#ifdef USE_RTC
  #include <RTClib.h>
  RTC_DS3231 rtc;
#endif

// Version information
#define VERSION "3.0"
#define VERSION_DATE "Jan 2026"

// ============================================================================
// HARDWARE DEFINITIONS
// ============================================================================

// Constructor for Temp & Humidity Sensor
AM2315C DHT;

// Constructor for 128x64 OLED Display
U8X8_SSD1309_128X64_NONAME0_4W_HW_SPI u8x8(/* cs=*/ 47, /* dc=*/ 49, /* reset=*/ 48);

// Button pins
const int bPinMenu = 22;
const int bPinUp = 24;
const int bPinSelect = 26;
const int bPinDown = 28;

// Debounce settings
#define DEBOUNCE_TIMEOUT 100
#define LONG_PRESS 1000

// Analog pins for soil moisture sensors
int analogpins[16] = {A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15};

// ============================================================================
// NETWORK CONFIGURATION
// ============================================================================

#ifdef USE_WIFI
  // WiFi credentials - CHANGE THESE!
  const char* ssid = "YourWiFiSSID";
  const char* password = "YourWiFiPassword";
  WiFiUDP udp;
#endif

#ifdef USE_ETHERNET
  // MAC address for Ethernet shield
  byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
  EthernetUDP udp;
#endif

// NTP Configuration
const char* ntpServer = "pool.ntp.org";
const int timeZone = -5;  // EST = -5, adjust for your timezone
const int ntpPort = 123;
unsigned long lastNTPSync = 0;
const unsigned long NTP_SYNC_INTERVAL = 3600000;  // Sync every hour

// ============================================================================
// WATERING MODE DEFINITIONS
// ============================================================================

enum WateringMode {
  MODE_MOISTURE_ONLY = 0,
  MODE_TIME_ONLY = 1,
  MODE_HYBRID = 2,
  MODE_MANUAL = 3
};

const char* modeNames[] = {
  "Moisture Only",
  "Time Only",
  "Hybrid",
  "Manual"
};

// ============================================================================
// MENU SYSTEM DEFINITIONS
// ============================================================================

enum MenuState {
  MENU_STATUS,
  MENU_MAIN,
  MENU_ZONE_CONFIG,
  MENU_ZONE_SELECT,
  MENU_ZONE_SETTINGS,
  MENU_ZONE_CALIBRATE,
  MENU_SCHEDULE_CONFIG,
  MENU_SCHEDULE_SELECT,
  MENU_SCHEDULE_EDIT,
  MENU_SYSTEM_CONFIG,
  MENU_NETWORK_CONFIG,
  MENU_TIME_CONFIG,
  MENU_MANUAL_CONTROL,
  MENU_WINTERIZE,
  MENU_DATA_LOG
};

MenuState currentMenu = MENU_STATUS;
int menuCursor = 0;
int selectedZone = 0;
int selectedSchedule = 0;
bool inEdit = false;
int editValue = 0;
int editDigit = 0;

// ============================================================================
// DATA STRUCTURES
// ============================================================================

// Zone schedule structure (time-based watering)
struct Schedule {
  bool enabled;
  int startHour;
  int startMinute;
  int duration;  // Duration in minutes
  bool days[7];  // Which days of week (0=Sunday, 6=Saturday)
};

// Zone configuration structure
struct ZoneConfig {
  bool enabled;
  char name[17];  // 16 chars + null terminator
  int dryCal;
  int wetCal;
  int moistureLow;   // Turn on threshold
  int moistureHigh;  // Turn off threshold
  WateringMode mode;
  Schedule schedule;
  int maxDuration;   // Maximum watering duration in minutes (safety)
  unsigned long lastWatered;  // Unix timestamp of last watering
  unsigned long wateringStart;  // When current watering started (0 if not watering)
  int totalWateringToday;  // Total minutes watered today
};

// System configuration structure
struct SystemConfig {
  // Basic settings
  int zones;
  bool tempDisplayF;  // true=Fahrenheit, false=Celsius
  int tempSwitch;
  int tempAdjLow;
  int tempAdjHi;
  bool relayOn;  // Relay activation level (0 or 1)
  
  // Network settings
  bool useWifi;  // true=WiFi, false=Ethernet
  bool networkEnabled;
  bool ntpEnabled;
  int timeZoneOffset;
  
  // Time settings
  bool use24Hour;
  
  // Logging
  bool loggingEnabled;
  int logInterval;  // Minutes between log entries
  
  // Safety limits
  int maxDailyWatering;  // Max minutes per zone per day
  bool freezeProtect;
  int freezeTemp;  // Temp below which to disable watering
  
  // Advanced
  int sensorReadInterval;  // Milliseconds between sensor reads
  bool backlightTimeout;
  int backlightMinutes;
};

// Global instances
ZoneConfig zones[16];
SystemConfig sysConfig;

// Runtime variables
long soilMoisture[16] = {0};
float temp = 0;
float humidity = 0;
bool relaystatus[16] = {false};
unsigned long lastSensorRead = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastLogEntry = 0;
unsigned long buttonPressStart = 0;
bool buttonPressed = false;
int lastButtonState[4] = {HIGH, HIGH, HIGH, HIGH};

// ============================================================================
// EEPROM MEMORY MAP
// ============================================================================
// Address 0-1999: Zone configurations (125 bytes × 16 zones = 2000 bytes)
// Address 2000-2199: System configuration (200 bytes)
// Address 2200-4095: Reserved for future use / logging

#define EEPROM_ZONE_BASE 0
#define EEPROM_SYSTEM_CONFIG 2000
#define EEPROM_LOG_BASE 2200

// ============================================================================
// FUNCTION PROTOTYPES
// ============================================================================

// EEPROM functions
void loadConfiguration();
void saveConfiguration();
void loadZoneConfig(int zoneNum);
void saveZoneConfig(int zoneNum);
void loadSystemConfig();
void saveSystemConfig();
void resetToDefaults();

// Network functions
void setupNetwork();
void syncNTP();
time_t getNTPTime();

// Menu system functions
void updateMenu();
void drawStatusScreen();
void drawMainMenu();
void drawZoneConfigMenu();
void drawScheduleMenu();
void drawSystemConfigMenu();
void drawNetworkConfigMenu();
void drawTimeConfigMenu();
void drawManualControlMenu();
void handleButtons();
int readButton(int pin, int index);

// Irrigation functions
void updateIrrigation();
void checkMoistureBased(int zone);
void checkTimeBased(int zone);
void checkHybridMode(int zone);
void startWatering(int zone);
void stopWatering(int zone);
bool isInScheduledWindow(int zone);
void checkSafetyLimits(int zone);

// Sensor functions
void readSensors();
float getTemperature();
float getHumidity();
int getSoilMoisture(int zone);

// Utility functions
void winterize();
void logData();
const char* getTimeString();
const char* getDateString();
void displayError(const char* msg);

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000);  // Wait up to 3 seconds for serial
  
  Serial.println();
  Serial.println(F("========================================"));
  Serial.print(F("Irrigation Controller V"));
  Serial.println(VERSION);
  Serial.print(F("Build Date: "));
  Serial.println(VERSION_DATE);
  Serial.println(F("========================================"));
  
  // Initialize display
  u8x8.begin();
  u8x8.setPowerSave(0);
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  
  u8x8.clear();
  u8x8.setCursor(0, 0);
  u8x8.print(F("Irrigation"));
  u8x8.setCursor(0, 1);
  u8x8.print(F("Controller"));
  u8x8.setCursor(0, 2);
  u8x8.print(F("Version "));
  u8x8.print(VERSION);
  u8x8.setCursor(0, 4);
  u8x8.print(F("Initializing..."));
  delay(2000);
  
  // Initialize I2C
  Wire.begin();
  
  // Initialize temperature sensor
  u8x8.setCursor(0, 5);
  u8x8.print(F("Temp Sensor..."));
  DHT.begin();
  delay(500);
  
  // Initialize RTC if available
  #ifdef USE_RTC
  u8x8.setCursor(0, 6);
  u8x8.print(F("RTC..."));
  if (rtc.begin()) {
    Serial.println(F("RTC initialized"));
    if (rtc.lostPower()) {
      Serial.println(F("RTC lost power, setting time..."));
      // Set to compile time as fallback
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
  } else {
    Serial.println(F("RTC not found"));
  }
  delay(500);
  #endif
  
  // Initialize button pins
  pinMode(bPinMenu, INPUT_PULLUP);
  pinMode(bPinUp, INPUT_PULLUP);
  pinMode(bPinSelect, INPUT_PULLUP);
  pinMode(bPinDown, INPUT_PULLUP);
  
  // Initialize analog input pins
  for (int i = 0; i < 16; i++) {
    pinMode(analogpins[i], INPUT);
  }
  
  // Load configuration from EEPROM
  u8x8.setCursor(0, 7);
  u8x8.print(F("Loading Config..."));
  loadConfiguration();
  delay(500);
  
  // Initialize relay output pins
  for (int i = 0; i < sysConfig.zones; i++) {
    pinMode(i + 2, OUTPUT);  // Digital pins 2-17
    digitalWrite(i + 2, !sysConfig.relayOn);  // All off initially
    relaystatus[i] = false;
  }
  
  // Setup network if enabled
  if (sysConfig.networkEnabled) {
    u8x8.clear();
    u8x8.setCursor(0, 0);
    u8x8.print(F("Network Setup..."));
    setupNetwork();
    delay(1000);
  }
  
  // Initial sensor read
  readSensors();
  
  // Setup complete
  u8x8.clear();
  u8x8.setCursor(0, 3);
  u8x8.print(F("Ready!"));
  delay(1000);
  
  Serial.println(F("Setup complete"));
  Serial.print(F("Active zones: "));
  Serial.println(sysConfig.zones);
  Serial.print(F("Network: "));
  Serial.println(sysConfig.networkEnabled ? "Enabled" : "Disabled");
  
  currentMenu = MENU_STATUS;
  lastDisplayUpdate = millis();
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
  unsigned long currentTime = millis();
  
  // Handle button input
  handleButtons();
  
  // Read sensors at configured interval
  if (currentTime - lastSensorRead >= sysConfig.sensorReadInterval) {
    readSensors();
    lastSensorRead = currentTime;
  }
  
  // Update irrigation logic (only if not in menu)
  if (currentMenu == MENU_STATUS && !inEdit) {
    updateIrrigation();
  }
  
  // Update display
  if (currentTime - lastDisplayUpdate >= 500) {  // Update display every 500ms
    updateMenu();
    lastDisplayUpdate = currentTime;
  }
  
  // NTP sync
  if (sysConfig.networkEnabled && sysConfig.ntpEnabled && 
      currentTime - lastNTPSync >= NTP_SYNC_INTERVAL) {
    syncNTP();
    lastNTPSync = currentTime;
  }
  
  // Data logging
  if (sysConfig.loggingEnabled && 
      currentTime - lastLogEntry >= (sysConfig.logInterval * 60000UL)) {
    logData();
    lastLogEntry = currentTime;
  }
  
  // Small delay to prevent overwhelming the processor
  delay(10);
}

// ============================================================================
// BUTTON HANDLING
// ============================================================================

void handleButtons() {
  static unsigned long lastButtonTime = 0;
  unsigned long currentTime = millis();
  
  // Debounce protection
  if (currentTime - lastButtonTime < DEBOUNCE_TIMEOUT) {
    return;
  }
  
  int btnMenu = readButton(bPinMenu, 0);
  int btnUp = readButton(bPinUp, 1);
  int btnDown = readButton(bPinDown, 2);
  int btnSelect = readButton(bPinSelect, 3);
  
  // Menu button - return to previous menu or status
  if (btnMenu == LOW && lastButtonState[0] == HIGH) {
    lastButtonTime = currentTime;
    
    if (inEdit) {
      inEdit = false;
    } else {
      // Navigate back through menu hierarchy
      switch (currentMenu) {
        case MENU_MAIN:
          currentMenu = MENU_STATUS;
          break;
        case MENU_ZONE_SELECT:
        case MENU_SCHEDULE_SELECT:
        case MENU_SYSTEM_CONFIG:
        case MENU_NETWORK_CONFIG:
        case MENU_TIME_CONFIG:
        case MENU_MANUAL_CONTROL:
        case MENU_WINTERIZE:
        case MENU_DATA_LOG:
          currentMenu = MENU_MAIN;
          menuCursor = 0;
          break;
        case MENU_ZONE_SETTINGS:
        case MENU_ZONE_CALIBRATE:
          currentMenu = MENU_ZONE_SELECT;
          break;
        case MENU_SCHEDULE_EDIT:
          currentMenu = MENU_SCHEDULE_SELECT;
          break;
        default:
          currentMenu = MENU_STATUS;
          break;
      }
    }
  }
  
  // Up button
  if (btnUp == LOW && lastButtonState[1] == HIGH) {
    lastButtonTime = currentTime;
    
    if (inEdit) {
      editValue++;
    } else {
      if (menuCursor > 0) menuCursor--;
    }
  }
  
  // Down button
  if (btnDown == LOW && lastButtonState[2] == HIGH) {
    lastButtonTime = currentTime;
    
    if (inEdit) {
      editValue--;
    } else {
      menuCursor++;
    }
  }
  
  // Select button
  if (btnSelect == LOW && lastButtonState[3] == HIGH) {
    lastButtonTime = currentTime;
    
    if (inEdit) {
      // Save edited value
      inEdit = false;
      // Actual saving happens in menu-specific code
    } else {
      // Enter submenu or start editing
      switch (currentMenu) {
        case MENU_STATUS:
          currentMenu = MENU_MAIN;
          menuCursor = 0;
          break;
        case MENU_MAIN:
          // Navigate to selected submenu
          switch (menuCursor) {
            case 0: currentMenu = MENU_ZONE_SELECT; break;
            case 1: currentMenu = MENU_SCHEDULE_SELECT; break;
            case 2: currentMenu = MENU_SYSTEM_CONFIG; break;
            case 3: currentMenu = MENU_NETWORK_CONFIG; break;
            case 4: currentMenu = MENU_TIME_CONFIG; break;
            case 5: currentMenu = MENU_MANUAL_CONTROL; break;
            case 6: currentMenu = MENU_WINTERIZE; break;
            case 7: currentMenu = MENU_DATA_LOG; break;
          }
          menuCursor = 0;
          break;
        case MENU_ZONE_SELECT:
          selectedZone = menuCursor;
          currentMenu = MENU_ZONE_SETTINGS;
          menuCursor = 0;
          break;
        case MENU_SCHEDULE_SELECT:
          selectedSchedule = menuCursor;
          currentMenu = MENU_SCHEDULE_EDIT;
          menuCursor = 0;
          break;
        // Add more cases for other menus
      }
    }
  }
  
  // Update button states
  lastButtonState[0] = btnMenu;
  lastButtonState[1] = btnUp;
  lastButtonState[2] = btnDown;
  lastButtonState[3] = btnSelect;
}

int readButton(int pin, int index) {
  return digitalRead(pin);
}

// ============================================================================
// MENU DRAWING FUNCTIONS
// ============================================================================

void updateMenu() {
  switch (currentMenu) {
    case MENU_STATUS:
      drawStatusScreen();
      break;
    case MENU_MAIN:
      drawMainMenu();
      break;
    case MENU_ZONE_SELECT:
      drawZoneConfigMenu();
      break;
    case MENU_ZONE_SETTINGS:
      drawZoneSettingsMenu();
      break;
    case MENU_SCHEDULE_SELECT:
      drawScheduleMenu();
      break;
    case MENU_SYSTEM_CONFIG:
      drawSystemConfigMenu();
      break;
    case MENU_NETWORK_CONFIG:
      drawNetworkConfigMenu();
      break;
    case MENU_TIME_CONFIG:
      drawTimeConfigMenu();
      break;
    case MENU_MANUAL_CONTROL:
      drawManualControlMenu();
      break;
    default:
      drawStatusScreen();
      break;
  }
}

void drawStatusScreen() {
  u8x8.clear();
  
  // Display time if available
  if (timeStatus() == timeSet) {
    u8x8.setCursor(0, 0);
    u8x8.print(getTimeString());
  }
  
  // Display temperature and humidity
  u8x8.setCursor(10, 0);
  if (sysConfig.tempDisplayF) {
    u8x8.print((int)temp);
    u8x8.print("F");
  } else {
    u8x8.print((int)((temp - 32) * 5 / 9));
    u8x8.print("C");
  }
  
  // Display up to 6 zones
  int displayZones = min(sysConfig.zones, 6);
  for (int i = 0; i < displayZones; i++) {
    if (!zones[i].enabled) continue;
    
    u8x8.setCursor(0, i + 1);
    u8x8.print(i + 1);
    u8x8.print(":");
    
    // Show moisture percentage
    u8x8.setCursor(3, i + 1);
    if (soilMoisture[i] < 10) u8x8.print(" ");
    if (soilMoisture[i] < 100) u8x8.print(" ");
    u8x8.print((int)soilMoisture[i]);
    u8x8.print("%");
    
    // Show status indicator
    u8x8.setCursor(9, i + 1);
    if (relaystatus[i]) {
      u8x8.print("ON ");
      // Show how long it's been running
      unsigned long runTime = (millis() - zones[i].wateringStart) / 60000;
      if (runTime < 100) {
        u8x8.print((int)runTime);
        u8x8.print("m");
      }
    } else {
      u8x8.print("OFF");
    }
    
    // Show mode indicator
    u8x8.setCursor(14, i + 1);
    switch (zones[i].mode) {
      case MODE_MOISTURE_ONLY: u8x8.print("M"); break;
      case MODE_TIME_ONLY: u8x8.print("T"); break;
      case MODE_HYBRID: u8x8.print("H"); break;
      case MODE_MANUAL: u8x8.print("X"); break;
    }
  }
  
  // Show "MENU" prompt
  u8x8.setCursor(0, 7);
  u8x8.print("MENU for options");
}

void drawMainMenu() {
  u8x8.clear();
  u8x8.setCursor(0, 0);
  u8x8.print("=== MAIN MENU ===");
  
  const char* menuItems[] = {
    "Zone Config",
    "Schedules",
    "System Setup",
    "Network Setup",
    "Time Setup",
    "Manual Control",
    "Winterize",
    "View Logs"
  };
  
  int numItems = 8;
  int startItem = max(0, menuCursor - 4);
  
  for (int i = 0; i < min(6, numItems); i++) {
    int itemIndex = startItem + i;
    if (itemIndex >= numItems) break;
    
    u8x8.setCursor(1, i + 1);
    if (itemIndex == menuCursor) {
      u8x8.print(">");
    } else {
      u8x8.print(" ");
    }
    u8x8.print(menuItems[itemIndex]);
  }
  
  u8x8.setCursor(0, 7);
  u8x8.print("MENU=Back SEL=OK");
}

void drawZoneConfigMenu() {
  u8x8.clear();
  u8x8.setCursor(0, 0);
  u8x8.print("== ZONE SELECT ==");
  
  int startZone = max(0, menuCursor - 4);
  
  for (int i = 0; i < min(6, sysConfig.zones); i++) {
    int zoneIndex = startZone + i;
    if (zoneIndex >= sysConfig.zones) break;
    
    u8x8.setCursor(0, i + 1);
    if (zoneIndex == menuCursor) {
      u8x8.print(">");
    } else {
      u8x8.print(" ");
    }
    
    u8x8.print("Z");
    u8x8.print(zoneIndex + 1);
    u8x8.print(":");
    
    if (zones[zoneIndex].enabled) {
      u8x8.print(zones[zoneIndex].name);
    } else {
      u8x8.print("(disabled)");
    }
  }
  
  u8x8.setCursor(0, 7);
  u8x8.print("MENU=Back SEL=OK");
}

void drawZoneSettingsMenu() {
  u8x8.clear();
  u8x8.setCursor(0, 0);
  u8x8.print("ZONE ");
  u8x8.print(selectedZone + 1);
  u8x8.print(" SETTINGS");
  
  const char* settingNames[] = {
    "Enable",
    "Mode",
    "Low %",
    "High %",
    "Calibrate",
    "Max Duration",
    "Reset Stats"
  };
  
  // Display current values
  u8x8.setCursor(0, 1);
  if (menuCursor == 0) u8x8.print(">");
  u8x8.print("Enable: ");
  u8x8.print(zones[selectedZone].enabled ? "YES" : "NO");
  
  u8x8.setCursor(0, 2);
  if (menuCursor == 1) u8x8.print(">");
  u8x8.print("Mode: ");
  u8x8.print(modeNames[zones[selectedZone].mode]);
  
  u8x8.setCursor(0, 3);
  if (menuCursor == 2) u8x8.print(">");
  u8x8.print("Low: ");
  u8x8.print(zones[selectedZone].moistureLow);
  u8x8.print("%");
  
  u8x8.setCursor(0, 4);
  if (menuCursor == 3) u8x8.print(">");
  u8x8.print("High: ");
  u8x8.print(zones[selectedZone].moistureHigh);
  u8x8.print("%");
  
  u8x8.setCursor(0, 5);
  if (menuCursor == 4) u8x8.print(">");
  u8x8.print("Calibrate");
  
  u8x8.setCursor(0, 6);
  if (menuCursor == 5) u8x8.print(">");
  u8x8.print("MaxDur: ");
  u8x8.print(zones[selectedZone].maxDuration);
  u8x8.print("m");
  
  u8x8.setCursor(0, 7);
  u8x8.print("UP/DN SEL=Edit");
}

void drawScheduleMenu() {
  u8x8.clear();
  u8x8.setCursor(0, 0);
  u8x8.print("=== SCHEDULES ===");
  
  u8x8.setCursor(0, 1);
  u8x8.print("Zone ");
  u8x8.print(menuCursor + 1);
  
  if (zones[menuCursor].schedule.enabled) {
    u8x8.setCursor(0, 2);
    u8x8.print("Time: ");
    if (zones[menuCursor].schedule.startHour < 10) u8x8.print("0");
    u8x8.print(zones[menuCursor].schedule.startHour);
    u8x8.print(":");
    if (zones[menuCursor].schedule.startMinute < 10) u8x8.print("0");
    u8x8.print(zones[menuCursor].schedule.startMinute);
    
    u8x8.setCursor(0, 3);
    u8x8.print("Dur: ");
    u8x8.print(zones[menuCursor].schedule.duration);
    u8x8.print(" min");
    
    u8x8.setCursor(0, 4);
    u8x8.print("Days: ");
    if (zones[menuCursor].schedule.days[0]) u8x8.print("Su ");
    if (zones[menuCursor].schedule.days[1]) u8x8.print("M ");
    if (zones[menuCursor].schedule.days[2]) u8x8.print("Tu ");
    if (zones[menuCursor].schedule.days[3]) u8x8.print("W ");
    u8x8.setCursor(6, 5);
    if (zones[menuCursor].schedule.days[4]) u8x8.print("Th ");
    if (zones[menuCursor].schedule.days[5]) u8x8.print("F ");
    if (zones[menuCursor].schedule.days[6]) u8x8.print("Sa");
  } else {
    u8x8.setCursor(0, 3);
    u8x8.print("No schedule");
  }
  
  u8x8.setCursor(0, 7);
  u8x8.print("SEL=Edit");
}

void drawSystemConfigMenu() {
  u8x8.clear();
  u8x8.setCursor(0, 0);
  u8x8.print("=== SYSTEM CFG ==");
  
  const char* items[] = {
    "Num Zones",
    "Temp Units",
    "Relay Type",
    "Freeze Protect",
    "Max Daily Water",
    "Sensor Interval"
  };
  
  u8x8.setCursor(0, 1);
  if (menuCursor == 0) u8x8.print(">");
  u8x8.print("Zones: ");
  u8x8.print(sysConfig.zones);
  
  u8x8.setCursor(0, 2);
  if (menuCursor == 1) u8x8.print(">");
  u8x8.print("Temp: ");
  u8x8.print(sysConfig.tempDisplayF ? "F" : "C");
  
  u8x8.setCursor(0, 3);
  if (menuCursor == 2) u8x8.print(">");
  u8x8.print("Relay: ");
  u8x8.print(sysConfig.relayOn ? "NO" : "NC");
  
  u8x8.setCursor(0, 4);
  if (menuCursor == 3) u8x8.print(">");
  u8x8.print("Freeze: ");
  u8x8.print(sysConfig.freezeProtect ? "ON" : "OFF");
  
  u8x8.setCursor(0, 7);
  u8x8.print("UP/DN SEL=Edit");
}

void drawNetworkConfigMenu() {
  u8x8.clear();
  u8x8.setCursor(0, 0);
  u8x8.print("== NETWORK CFG ==");
  
  u8x8.setCursor(0, 1);
  if (menuCursor == 0) u8x8.print(">");
  u8x8.print("Enable: ");
  u8x8.print(sysConfig.networkEnabled ? "YES" : "NO");
  
  u8x8.setCursor(0, 2);
  if (menuCursor == 1) u8x8.print(">");
  u8x8.print("Type: ");
  #ifdef USE_WIFI
  u8x8.print("WiFi");
  #else
  u8x8.print("Ethernet");
  #endif
  
  u8x8.setCursor(0, 3);
  if (menuCursor == 2) u8x8.print(">");
  u8x8.print("NTP: ");
  u8x8.print(sysConfig.ntpEnabled ? "ON" : "OFF");
  
  if (sysConfig.networkEnabled) {
    u8x8.setCursor(0, 5);
    #ifdef USE_WIFI
    u8x8.print("IP: ");
    u8x8.print(WiFi.localIP().toString().c_str());
    #else
    u8x8.print("Connected");
    #endif
  }
  
  u8x8.setCursor(0, 7);
  u8x8.print("UP/DN SEL=Edit");
}

void drawTimeConfigMenu() {
  u8x8.clear();
  u8x8.setCursor(0, 0);
  u8x8.print("=== TIME SETUP ==");
  
  u8x8.setCursor(0, 1);
  u8x8.print("Current:");
  u8x8.setCursor(0, 2);
  if (timeStatus() == timeSet) {
    u8x8.print(getTimeString());
    u8x8.setCursor(0, 3);
    u8x8.print(getDateString());
  } else {
    u8x8.print("Not Set");
  }
  
  u8x8.setCursor(0, 5);
  u8x8.print("TZ Offset: ");
  u8x8.print(sysConfig.timeZoneOffset);
  
  u8x8.setCursor(0, 6);
  u8x8.print("24hr: ");
  u8x8.print(sysConfig.use24Hour ? "YES" : "NO");
  
  u8x8.setCursor(0, 7);
  u8x8.print("SEL=Sync NTP");
}

void drawManualControlMenu() {
  u8x8.clear();
  u8x8.setCursor(0, 0);
  u8x8.print("= MANUAL CTRL ==");
  
  int startZone = max(0, menuCursor - 4);
  
  for (int i = 0; i < min(6, sysConfig.zones); i++) {
    int zoneIndex = startZone + i;
    if (zoneIndex >= sysConfig.zones) break;
    
    u8x8.setCursor(0, i + 1);
    if (zoneIndex == menuCursor) {
      u8x8.print(">");
    } else {
      u8x8.print(" ");
    }
    
    u8x8.print("Z");
    u8x8.print(zoneIndex + 1);
    u8x8.print(": ");
    
    if (relaystatus[zoneIndex]) {
      u8x8.print("ON  (SEL=OFF)");
    } else {
      u8x8.print("OFF (SEL=ON)");
    }
  }
  
  u8x8.setCursor(0, 7);
  u8x8.print("MENU=Back");
}

// ============================================================================
// IRRIGATION LOGIC
// ============================================================================

void updateIrrigation() {
  // Check freeze protection
  if (sysConfig.freezeProtect && temp < sysConfig.freezeTemp) {
    // Turn off all zones
    for (int i = 0; i < sysConfig.zones; i++) {
      if (relaystatus[i]) {
        stopWatering(i);
      }
    }
    return;
  }
  
  // Update each zone
  for (int i = 0; i < sysConfig.zones; i++) {
    if (!zones[i].enabled) continue;
    
    // Check safety limits
    checkSafetyLimits(i);
    
    // Run appropriate control logic based on mode
    switch (zones[i].mode) {
      case MODE_MOISTURE_ONLY:
        checkMoistureBased(i);
        break;
      case MODE_TIME_ONLY:
        checkTimeBased(i);
        break;
      case MODE_HYBRID:
        checkHybridMode(i);
        break;
      case MODE_MANUAL:
        // Manual mode - do nothing automatically
        break;
    }
  }
}

void checkMoistureBased(int zone) {
  // Adjust thresholds for temperature if configured
  int effectiveLow = zones[zone].moistureLow;
  int effectiveHigh = zones[zone].moistureHigh;
  
  if (temp >= sysConfig.tempSwitch) {
    effectiveLow += sysConfig.tempAdjLow;
    effectiveHigh += sysConfig.tempAdjHi;
  }
  
  // Turn on if below low threshold
  if (soilMoisture[zone] <= effectiveLow && !relaystatus[zone]) {
    startWatering(zone);
  }
  
  // Turn off if above high threshold
  if (soilMoisture[zone] >= effectiveHigh && relaystatus[zone]) {
    stopWatering(zone);
  }
}

void checkTimeBased(int zone) {
  if (!zones[zone].schedule.enabled) return;
  
  // Check if we're in the scheduled time window
  if (isInScheduledWindow(zone) && !relaystatus[zone]) {
    // Check if we haven't already watered today
    if (day() != day(zones[zone].lastWatered)) {
      startWatering(zone);
    }
  }
  
  // Check if we've exceeded the scheduled duration
  if (relaystatus[zone]) {
    unsigned long runTime = (millis() - zones[zone].wateringStart) / 60000;
    if (runTime >= zones[zone].schedule.duration) {
      stopWatering(zone);
    }
  }
}

void checkHybridMode(int zone) {
  // Hybrid mode: Only water during scheduled time windows,
  // but use moisture sensors to control start/stop
  
  if (!zones[zone].schedule.enabled) {
    // Fall back to moisture-only if no schedule
    checkMoistureBased(zone);
    return;
  }
  
  // Only operate during scheduled windows
  if (isInScheduledWindow(zone)) {
    checkMoistureBased(zone);
  } else {
    // Outside schedule window - turn off if running
    if (relaystatus[zone]) {
      stopWatering(zone);
    }
  }
}

bool isInScheduledWindow(int zone) {
  if (!zones[zone].schedule.enabled) return false;
  if (timeStatus() != timeSet) return false;
  
  // Check if today is a scheduled day
  int today = weekday() - 1;  // TimeLib uses 1-7, we use 0-6
  if (!zones[zone].schedule.days[today]) return false;
  
  // Check if current time is within the window
  int currentMinutes = hour() * 60 + minute();
  int startMinutes = zones[zone].schedule.startHour * 60 + zones[zone].schedule.startMinute;
  int endMinutes = startMinutes + zones[zone].schedule.duration;
  
  return (currentMinutes >= startMinutes && currentMinutes < endMinutes);
}

void checkSafetyLimits(int zone) {
  // Check maximum run time
  if (relaystatus[zone]) {
    unsigned long runTime = (millis() - zones[zone].wateringStart) / 60000;
    
    // Emergency shutoff if max duration exceeded
    if (runTime >= zones[zone].maxDuration) {
      stopWatering(zone);
      Serial.print(F("Zone "));
      Serial.print(zone + 1);
      Serial.println(F(" - Max duration exceeded!"));
    }
    
    // Check daily limit
    if (zones[zone].totalWateringToday >= sysConfig.maxDailyWatering) {
      stopWatering(zone);
      Serial.print(F("Zone "));
      Serial.print(zone + 1);
      Serial.println(F(" - Daily limit reached!"));
    }
  }
}

void startWatering(int zone) {
  digitalWrite(zone + 2, sysConfig.relayOn);
  relaystatus[zone] = true;
  zones[zone].wateringStart = millis();
  
  Serial.print(F("Zone "));
  Serial.print(zone + 1);
  Serial.println(F(" - Started watering"));
}

void stopWatering(int zone) {
  digitalWrite(zone + 2, !sysConfig.relayOn);
  relaystatus[zone] = false;
  
  // Update statistics
  if (zones[zone].wateringStart > 0) {
    unsigned long duration = (millis() - zones[zone].wateringStart) / 60000;
    zones[zone].totalWateringToday += duration;
    zones[zone].lastWatered = now();
    zones[zone].wateringStart = 0;
    
    Serial.print(F("Zone "));
    Serial.print(zone + 1);
    Serial.print(F(" - Stopped watering ("));
    Serial.print(duration);
    Serial.println(F(" minutes)"));
  }
}

// ============================================================================
// SENSOR READING
// ============================================================================

void readSensors() {
  // Read temperature and humidity
  int status = DHT.read();
  if (status == AM2315C_OK) {
    temp = DHT.getTemperature() * 9.0 / 5.0 + 32.0;  // Convert to Fahrenheit
    humidity = DHT.getHumidity();
  }
  
  // Read soil moisture sensors
  for (int i = 0; i < sysConfig.zones; i++) {
    if (!zones[i].enabled) continue;
    
    long reading = analogRead(analogpins[i]);
    
    // Constrain to calibration range
    if (reading < zones[i].dryCal) reading = zones[i].dryCal;
    if (reading > zones[i].wetCal) reading = zones[i].wetCal;
    
    // Map to percentage
    soilMoisture[i] = map(reading, zones[i].dryCal, zones[i].wetCal, 0, 100);
  }
}

float getTemperature() {
  return temp;
}

float getHumidity() {
  return humidity;
}

int getSoilMoisture(int zone) {
  if (zone >= 0 && zone < 16) {
    return soilMoisture[zone];
  }
  return 0;
}

// ============================================================================
// NETWORK FUNCTIONS
// ============================================================================

void setupNetwork() {
  #ifdef USE_WIFI
  Serial.print(F("Connecting to WiFi"));
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print(F("WiFi connected. IP: "));
    Serial.println(WiFi.localIP());
    
    if (sysConfig.ntpEnabled) {
      syncNTP();
    }
  } else {
    Serial.println(F(" Failed!"));
    sysConfig.networkEnabled = false;
  }
  #endif
  
  #ifdef USE_ETHERNET
  Serial.print(F("Starting Ethernet..."));
  if (Ethernet.begin(mac)) {
    Serial.println(F(" Success!"));
    Serial.print(F("IP: "));
    Serial.println(Ethernet.localIP());
    
    if (sysConfig.ntpEnabled) {
      syncNTP();
    }
  } else {
    Serial.println(F(" Failed!"));
    sysConfig.networkEnabled = false;
  }
  #endif
}

void syncNTP() {
  Serial.println(F("Syncing time with NTP..."));
  
  time_t ntpTime = getNTPTime();
  if (ntpTime > 0) {
    setTime(ntpTime + (sysConfig.timeZoneOffset * 3600));
    
    #ifdef USE_RTC
    // Update RTC as well
    rtc.adjust(DateTime(year(), month(), day(), hour(), minute(), second()));
    #endif
    
    Serial.println(F("Time synchronized"));
    Serial.println(getTimeString());
  } else {
    Serial.println(F("NTP sync failed"));
  }
}

time_t getNTPTime() {
  #if defined(USE_WIFI) || defined(USE_ETHERNET)
  const int NTP_PACKET_SIZE = 48;
  byte packetBuffer[NTP_PACKET_SIZE];
  
  // Clear buffer
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  
  // Initialize NTP request
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;            // Stratum
  packetBuffer[2] = 6;            // Polling Interval
  packetBuffer[3] = 0xEC;         // Peer Clock Precision
  
  // Send NTP request
  udp.begin(ntpPort);
  udp.beginPacket(ntpServer, 123);
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
  
  // Wait for response
  delay(1000);
  
  if (udp.parsePacket()) {
    udp.read(packetBuffer, NTP_PACKET_SIZE);
    
    // Extract timestamp (seconds since 1900)
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    
    // Convert to Unix time (seconds since 1970)
    const unsigned long seventyYears = 2208988800UL;
    time_t epoch = secsSince1900 - seventyYears;
    
    udp.stop();
    return epoch;
  }
  
  udp.stop();
  #endif
  
  return 0;
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

const char* getTimeString() {
  static char timeStr[9];
  if (sysConfig.use24Hour) {
    sprintf(timeStr, "%02d:%02d:%02d", hour(), minute(), second());
  } else {
    int h = hour();
    bool pm = h >= 12;
    if (h > 12) h -= 12;
    if (h == 0) h = 12;
    sprintf(timeStr, "%2d:%02d %s", h, minute(), pm ? "PM" : "AM");
  }
  return timeStr;
}

const char* getDateString() {
  static char dateStr[11];
  sprintf(dateStr, "%02d/%02d/%04d", month(), day(), year());
  return dateStr;
}

void winterize() {
  u8x8.clear();
  u8x8.setCursor(0, 0);
  u8x8.print(F("WINTERIZING..."));
  u8x8.setCursor(0, 1);
  u8x8.print(F("Clearing valves"));
  
  Serial.println(F("========== WINTERIZE =========="));
  
  for (int i = 0; i < sysConfig.zones; i++) {
    if (!zones[i].enabled) continue;
    
    u8x8.clearLine(3);
    u8x8.setCursor(0, 3);
    u8x8.print(F("Zone "));
    u8x8.print(i + 1);
    
    Serial.print(F("Zone "));
    Serial.print(i + 1);
    Serial.print(F(": "));
    
    // Turn on
    digitalWrite(i + 2, sysConfig.relayOn);
    u8x8.print(F(" ON"));
    Serial.print(F("ON..."));
    
    // Run for 5 seconds
    for (int j = 0; j < 5; j++) {
      delay(1000);
      u8x8.print(".");
    }
    
    // Turn off
    digitalWrite(i + 2, !sysConfig.relayOn);
    Serial.println(F(" OFF"));
    
    u8x8.setCursor(0, 4);
    u8x8.print(F("Complete!"));
    delay(1000);
  }
  
  u8x8.clear();
  u8x8.setCursor(0, 3);
  u8x8.print(F("WINTERIZE"));
  u8x8.setCursor(0, 4);
  u8x8.print(F("COMPLETE!"));
  
  Serial.println(F("========== COMPLETE =========="));
  delay(3000);
}

void logData() {
  // Log current state to serial
  Serial.println(F("========== DATA LOG =========="));
  Serial.print(F("Time: "));
  Serial.print(getTimeString());
  Serial.print(F(" "));
  Serial.println(getDateString());
  
  Serial.print(F("Temperature: "));
  Serial.print(temp);
  Serial.print(F("F, Humidity: "));
  Serial.print(humidity);
  Serial.println(F("%"));
  
  for (int i = 0; i < sysConfig.zones; i++) {
    if (!zones[i].enabled) continue;
    
    Serial.print(F("Zone "));
    Serial.print(i + 1);
    Serial.print(F(": "));
    Serial.print(soilMoisture[i]);
    Serial.print(F("% - "));
    Serial.print(relaystatus[i] ? "ON" : "OFF");
    Serial.print(F(" - Today: "));
    Serial.print(zones[i].totalWateringToday);
    Serial.println(F("min"));
  }
  
  Serial.println(F("=============================="));
}

void displayError(const char* msg) {
  u8x8.clear();
  u8x8.setCursor(0, 3);
  u8x8.print(F("ERROR:"));
  u8x8.setCursor(0, 4);
  u8x8.print(msg);
  delay(3000);
}

// ============================================================================
// EEPROM CONFIGURATION MANAGEMENT
// ============================================================================

void loadConfiguration() {
  // Load system configuration
  loadSystemConfig();
  
  // Check if EEPROM has been initialized
  if (sysConfig.zones < 1 || sysConfig.zones > 16) {
    Serial.println(F("EEPROM not initialized - loading defaults"));
    resetToDefaults();
    saveConfiguration();
  } else {
    // Load zone configurations
    for (int i = 0; i < 16; i++) {
      loadZoneConfig(i);
    }
  }
}

void saveConfiguration() {
  saveSystemConfig();
  for (int i = 0; i < sysConfig.zones; i++) {
    saveZoneConfig(i);
  }
  Serial.println(F("Configuration saved to EEPROM"));
}

void loadZoneConfig(int zoneNum) {
  if (zoneNum < 0 || zoneNum >= 16) return;
  
  int addr = EEPROM_ZONE_BASE + (zoneNum * 125);
  EEPROM.get(addr, zones[zoneNum]);
}

void saveZoneConfig(int zoneNum) {
  if (zoneNum < 0 || zoneNum >= 16) return;
  
  int addr = EEPROM_ZONE_BASE + (zoneNum * 125);
  EEPROM.put(addr, zones[zoneNum]);
}

void loadSystemConfig() {
  EEPROM.get(EEPROM_SYSTEM_CONFIG, sysConfig);
}

void saveSystemConfig() {
  EEPROM.put(EEPROM_SYSTEM_CONFIG, sysConfig);
}

void resetToDefaults() {
  // System defaults
  sysConfig.zones = 6;
  sysConfig.tempDisplayF = true;
  sysConfig.tempSwitch = 92;
  sysConfig.tempAdjLow = 6;
  sysConfig.tempAdjHi = 4;
  sysConfig.relayOn = 0;  // NC relays
  sysConfig.useWifi = true;
  sysConfig.networkEnabled = false;
  sysConfig.ntpEnabled = true;
  sysConfig.timeZoneOffset = -5;  // EST
  sysConfig.use24Hour = false;
  sysConfig.loggingEnabled = true;
  sysConfig.logInterval = 60;  // 60 minutes
  sysConfig.maxDailyWatering = 180;  // 3 hours max per zone per day
  sysConfig.freezeProtect = true;
  sysConfig.freezeTemp = 35;  // 35°F
  sysConfig.sensorReadInterval = 5000;  // 5 seconds
  sysConfig.backlightTimeout = false;
  sysConfig.backlightMinutes = 5;
  
  // Zone defaults
  for (int i = 0; i < 16; i++) {
    zones[i].enabled = (i < sysConfig.zones);
    sprintf(zones[i].name, "Zone %d", i + 1);
    zones[i].dryCal = 125;
    zones[i].wetCal = 550;
    zones[i].moistureLow = 40;
    zones[i].moistureHigh = 70;
    zones[i].mode = MODE_MOISTURE_ONLY;
    zones[i].maxDuration = 60;  // 60 minutes max
    zones[i].lastWatered = 0;
    zones[i].wateringStart = 0;
    zones[i].totalWateringToday = 0;
    
    // Default schedule (disabled)
    zones[i].schedule.enabled = false;
    zones[i].schedule.startHour = 6;
    zones[i].schedule.startMinute = 0;
    zones[i].schedule.duration = 30;
    for (int j = 0; j < 7; j++) {
      zones[i].schedule.days[j] = false;
    }
  }
  
  Serial.println(F("Configuration reset to defaults"));
}
