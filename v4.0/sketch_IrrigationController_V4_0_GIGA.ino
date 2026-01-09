/*
  Advanced Soil Moisture Irrigation Controller
  Version 4.0 - Arduino GIGA R1 WiFi Edition
  January 2026
  
  *** FIXED VERSION ***
  Fixes applied:
  1. Added missing handleButtonPress() function prototype
  2. Fixed GigaDisplayTouch namespace (GDTpoint_t instead of GigaDisplayTouch::Contact)
  3. Moved calibration button handling from drawCalibrateScreen() to handleButtonPress()
     (was referencing undefined 'id' variable - critical bug)
  4. Fixed min() template ambiguity with explicit int casts
  5. Fixed relayOn initialization to use proper boolean 'false'
  6. Replaced strlcpy with portable strncpy implementation
  7. Added ArduinoJson version requirement note
  
  Optimized for Arduino GIGA R1 WiFi with GIGA Display Shield
  
  Hardware:
  - Arduino GIGA R1 WiFi (STM32H747XI dual core)
  - GIGA Display Shield (480x320 TFT touchscreen)
  - AM2315C Temperature/Humidity sensor
  - Up to 14 zones (GIGA has 14 ADC inputs)
  - 14-channel relay board
  
  New in V4.0:
  - Touch-enabled graphical interface
  - 480x320 color TFT display
  - Beautiful modern UI design
  - Touch controls (no physical buttons needed!)
  - Built-in WiFi (no external module)
  - More processing power (dual core M7+M4)
  - Enhanced graphics and animations
  
  Features:
  - Full touchscreen menu system
  - Web interface for remote control
  - Multiple watering modes (Moisture, Time, Hybrid, Manual)
  - Advanced scheduling with day-of-week selection
  - NTP time synchronization
  - Real-time monitoring with graphs
  - All settings save to EEPROM
*/

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WiFiUdp.h>
#include <TimeLib.h>
#include <ArduinoJson.h>  // REQUIRES ArduinoJson v6.21.x (NOT v7.x!)
#include <EEPROM.h>
#include <Wire.h>
#include "AM2315C.h"

// GIGA Display Shield includes
#include "Arduino_GigaDisplay_GFX.h"
#include "Arduino_GigaDisplayTouch.h"

#define VERSION "4.0"
#define VERSION_DATE "Jan 2026"

// ============================================================================
// HARDWARE DEFINITIONS - GIGA R1 SPECIFIC
// ============================================================================

// GIGA Display Shield (480x320)
GigaDisplay_GFX display;
Arduino_GigaDisplayTouch touchDetector;

// Temperature sensor
AM2315C DHT;

// GIGA R1 WiFi has 14 ADC inputs (A0-A13)
// Note: GIGA uses different analog pin numbering
const int MAX_ZONES = 14;
int analogPins[14] = {A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13};

// Digital outputs for relays (using pins 2-15 for 14 zones)
const int relayPinStart = 2;

// ============================================================================
// DISPLAY CONFIGURATION
// ============================================================================

#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 320

// Color palette - Modern dark theme with vibrant accents
#define COLOR_BG_PRIMARY    0x0B1A    // Dark blue-gray
#define COLOR_BG_SECONDARY  0x1C47    // Medium blue-gray
#define COLOR_BG_CARD       0x2965    // Card background
#define COLOR_ACCENT        0x04DF    // Bright cyan
#define COLOR_SUCCESS       0x07E0    // Green
#define COLOR_WARNING       0xFD20    // Orange
#define COLOR_DANGER        0xF800    // Red
#define COLOR_TEXT          0xFFFF    // White
#define COLOR_TEXT_DIM      0x8410    // Gray
#define COLOR_BORDER        0x4208    // Dark gray

// UI dimensions
#define HEADER_HEIGHT 45
#define FOOTER_HEIGHT 40
#define CARD_MARGIN 8
#define CARD_RADIUS 12
#define BUTTON_HEIGHT 50
#define BUTTON_RADIUS 8

// ============================================================================
// NETWORK CONFIGURATION
// ============================================================================

// WiFi credentials - CHANGE THESE!
const char* ssid = "YourWiFiSSID";
const char* password = "YourWiFiPassword";

WiFiUDP udp;
WebServer server(80);

const char* ntpServer = "pool.ntp.org";
const int timeZoneOffset = -5;  // EST = -5
const int ntpPort = 123;
unsigned long lastNTPSync = 0;
const unsigned long NTP_SYNC_INTERVAL = 3600000;

// ============================================================================
// DATA STRUCTURES
// ============================================================================

enum WateringMode {
  MODE_MOISTURE_ONLY = 0,
  MODE_TIME_ONLY = 1,
  MODE_HYBRID = 2,
  MODE_MANUAL = 3
};

const char* modeNames[] = {"Moisture", "Time", "Hybrid", "Manual"};
const char* modeNamesLong[] = {
  "Moisture Only", 
  "Time Based", 
  "Hybrid Mode", 
  "Manual Control"
};

enum Screen {
  SCREEN_HOME,
  SCREEN_ZONES,
  SCREEN_ZONE_DETAIL,
  SCREEN_SCHEDULES,
  SCREEN_SCHEDULE_EDIT,
  SCREEN_SYSTEM,
  SCREEN_NETWORK,
  SCREEN_CALIBRATE,
  SCREEN_GRAPHS
};

struct Schedule {
  bool enabled;
  int startHour;
  int startMinute;
  int duration;
  bool days[7];
};

struct ZoneConfig {
  bool enabled;
  char name[17];
  int dryCal;
  int wetCal;
  int moistureLow;
  int moistureHigh;
  WateringMode mode;
  Schedule schedule;
  int maxDuration;
  unsigned long lastWatered;
  unsigned long wateringStart;
  int totalWateringToday;
  int moistureHistory[60];  // Last 60 readings for graph
  int historyIndex;
};

struct SystemConfig {
  int zones;
  bool tempDisplayF;
  int tempSwitch;
  int tempAdjLow;
  int tempAdjHi;
  bool relayOn;
  bool networkEnabled;
  bool ntpEnabled;
  int timeZoneOffset;
  bool use24Hour;
  bool loggingEnabled;
  int logInterval;
  int maxDailyWatering;
  bool freezeProtect;
  int freezeTemp;
  int sensorReadInterval;
  int screenTimeout;
  bool screenSaver;
};

struct TouchButton {
  int x, y, w, h;
  const char* label;
  uint16_t color;
  bool visible;
  int id;
};

// Global instances
ZoneConfig zones[14];
SystemConfig sysConfig;

// Runtime variables
long soilMoisture[14] = {0};
float temp = 0;
float humidity = 0;
bool relayStatus[14] = {false};
unsigned long lastSensorRead = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastLogEntry = 0;
unsigned long lastTouchTime = 0;
bool screenActive = true;

// UI state
Screen currentScreen = SCREEN_HOME;
int selectedZone = 0;
int scrollOffset = 0;
bool needsRedraw = true;
TouchButton buttons[20];
int buttonCount = 0;

#define EEPROM_ZONE_BASE 0
#define EEPROM_SYSTEM_CONFIG 2000

// ============================================================================
// FUNCTION PROTOTYPES
// ============================================================================

// Configuration
void loadConfiguration();
void saveConfiguration();
void loadZoneConfig(int zoneNum);
void saveZoneConfig(int zoneNum);
void loadSystemConfig();
void saveSystemConfig();
void resetToDefaults();

// Network
void setupNetwork();
void setupWebServer();
void handleRoot();
void handleGetStatus();
void handleGetZones();
void handleSetZone();
void handleGetSystem();
void handleSetSystem();
void handleControlZone();
void syncNTP();
time_t getNTPTime();

// Display
void initDisplay();
void drawScreen();
void drawHomeScreen();
void drawZonesScreen();
void drawZoneDetailScreen();
void drawSchedulesScreen();
void drawSystemScreen();
void drawNetworkScreen();
void drawCalibrateScreen();
void drawGraphsScreen();
void drawHeader();
void drawFooter();
void drawCard(int x, int y, int w, int h, const char* title);
void drawButton(int x, int y, int w, int h, const char* label, uint16_t color, int id);
void drawProgressBar(int x, int y, int w, int h, int value, int max, uint16_t color);
void drawZoneCard(int x, int y, int w, int h, int zoneId);
void drawGraph(int x, int y, int w, int h, int* data, int dataSize, int max);
void drawStatusBadge(int x, int y, const char* text, uint16_t color);
void clearButtons();
void addButton(int x, int y, int w, int h, const char* label, uint16_t color, int id);

// Touch handling
void handleTouch();
int getTouchedButton(int x, int y);
void handleButtonPress(int id);
void vibrate(int duration);

// Irrigation
void updateIrrigation();
void checkMoistureBased(int zone);
void checkTimeBased(int zone);
void checkHybridMode(int zone);
void startWatering(int zone);
void stopWatering(int zone);
bool isInScheduledWindow(int zone);
void checkSafetyLimits(int zone);

// Sensors
void readSensors();
void updateMoistureHistory(int zone);

// Utilities
const char* getTimeString();
const char* getDateString();
void logData();

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println(F("\n========================================"));
  Serial.print(F("Irrigation Controller V"));
  Serial.println(VERSION);
  Serial.println(F("Arduino GIGA R1 WiFi Edition"));
  Serial.print(F("Build: "));
  Serial.println(VERSION_DATE);
  Serial.println(F("========================================"));
  
  // Initialize display
  Serial.println(F("Initializing display..."));
  initDisplay();
  
  // Show splash screen
  display.fillScreen(COLOR_BG_PRIMARY);
  display.setTextColor(COLOR_ACCENT);
  display.setTextSize(3);
  display.setCursor(100, 100);
  display.print(F("Irrigation Control"));
  display.setTextSize(2);
  display.setTextColor(COLOR_TEXT);
  display.setCursor(150, 150);
  display.print(F("Version "));
  display.print(VERSION);
  display.setTextSize(1);
  display.setCursor(140, 200);
  display.print(F("Arduino GIGA R1 WiFi Edition"));
  display.setCursor(180, 220);
  display.print(F("Initializing..."));
  delay(2000);
  
  // Initialize I2C
  Wire.begin();
  
  // Initialize temperature sensor
  Serial.println(F("Initializing temp sensor..."));
  DHT.begin();
  delay(500);
  
  // Initialize analog inputs
  for (int i = 0; i < MAX_ZONES; i++) {
    pinMode(analogPins[i], INPUT);
  }
  
  // Load configuration
  Serial.println(F("Loading configuration..."));
  display.setCursor(180, 240);
  display.print(F("Loading config..."));
  loadConfiguration();
  delay(500);
  
  // Initialize relay outputs
  for (int i = 0; i < sysConfig.zones; i++) {
    pinMode(relayPinStart + i, OUTPUT);
    digitalWrite(relayPinStart + i, !sysConfig.relayOn);
    relayStatus[i] = false;
  }
  
  // Setup network
  if (sysConfig.networkEnabled) {
    Serial.println(F("Setting up WiFi..."));
    display.setCursor(180, 260);
    display.print(F("Connecting WiFi..."));
    setupNetwork();
    setupWebServer();
    delay(1000);
  }
  
  // Initial sensor read
  readSensors();
  
  Serial.println(F("Setup complete!"));
  Serial.print(F("Active zones: "));
  Serial.println(sysConfig.zones);
  
  if (sysConfig.networkEnabled && WiFi.status() == WL_CONNECTED) {
    Serial.print(F("Web interface: http://"));
    Serial.println(WiFi.localIP());
  }
  
  // Initialize display state
  currentScreen = SCREEN_HOME;
  needsRedraw = true;
  lastTouchTime = millis();
  screenActive = true;
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
  unsigned long currentTime = millis();
  
  // Handle touch input
  handleTouch();
  
  // Read sensors
  if (currentTime - lastSensorRead >= sysConfig.sensorReadInterval) {
    readSensors();
    lastSensorRead = currentTime;
    needsRedraw = true;
  }
  
  // Update irrigation logic
  updateIrrigation();
  
  // Update display
  if (needsRedraw && screenActive) {
    drawScreen();
    needsRedraw = false;
  }
  
  // Screen timeout
  if (sysConfig.screenSaver && 
      (currentTime - lastTouchTime > sysConfig.screenTimeout * 1000)) {
    if (screenActive) {
      display.fillScreen(COLOR_BG_PRIMARY);
      screenActive = false;
    }
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
  
  // Handle web requests
  if (sysConfig.networkEnabled) {
    server.handleClient();
  }
  
  delay(10);
}

// ============================================================================
// DISPLAY INITIALIZATION & CORE DRAWING
// ============================================================================

void initDisplay() {
  display.begin();
  touchDetector.begin();
  
  display.fillScreen(COLOR_BG_PRIMARY);
  display.setRotation(1);  // Landscape mode
  display.setTextWrap(false);
}

void drawScreen() {
  display.fillScreen(COLOR_BG_PRIMARY);
  clearButtons();
  
  switch (currentScreen) {
    case SCREEN_HOME: drawHomeScreen(); break;
    case SCREEN_ZONES: drawZonesScreen(); break;
    case SCREEN_ZONE_DETAIL: drawZoneDetailScreen(); break;
    case SCREEN_SCHEDULES: drawSchedulesScreen(); break;
    case SCREEN_SYSTEM: drawSystemScreen(); break;
    case SCREEN_NETWORK: drawNetworkScreen(); break;
    case SCREEN_CALIBRATE: drawCalibrateScreen(); break;
    case SCREEN_GRAPHS: drawGraphsScreen(); break;
  }
  
  drawHeader();
  drawFooter();
}

void drawHeader() {
  // Header background with gradient effect
  for (int i = 0; i < HEADER_HEIGHT; i++) {
    uint16_t color = display.color565(
      11 + (i * 10 / HEADER_HEIGHT),
      26 + (i * 20 / HEADER_HEIGHT),
      26 + (i * 20 / HEADER_HEIGHT)
    );
    display.drawFastHLine(0, i, SCREEN_WIDTH, color);
  }
  
  // Time
  display.setTextColor(COLOR_TEXT);
  display.setTextSize(2);
  display.setCursor(10, 15);
  display.print(getTimeString());
  
  // Temperature & Humidity
  display.setTextSize(1);
  display.setTextColor(COLOR_TEXT_DIM);
  display.setCursor(150, 15);
  display.print(F("Temp: "));
  display.setTextColor(COLOR_ACCENT);
  display.print((int)temp);
  display.print(sysConfig.tempDisplayF ? "F" : "C");
  
  display.setTextColor(COLOR_TEXT_DIM);
  display.setCursor(150, 28);
  display.print(F("Humidity: "));
  display.setTextColor(COLOR_ACCENT);
  display.print((int)humidity);
  display.print(F("%"));
  
  // Active zones indicator
  int activeCount = 0;
  for (int i = 0; i < sysConfig.zones; i++) {
    if (relayStatus[i]) activeCount++;
  }
  
  display.setTextColor(COLOR_TEXT_DIM);
  display.setCursor(280, 15);
  display.print(F("Active: "));
  display.setTextColor(activeCount > 0 ? COLOR_SUCCESS : COLOR_TEXT_DIM);
  display.print(activeCount);
  display.print(F("/"));
  display.print(sysConfig.zones);
  
  // WiFi status
  if (sysConfig.networkEnabled) {
    display.setCursor(280, 28);
    if (WiFi.status() == WL_CONNECTED) {
      display.setTextColor(COLOR_SUCCESS);
      display.print(F("WiFi: "));
      display.print(WiFi.localIP());
    } else {
      display.setTextColor(COLOR_DANGER);
      display.print(F("WiFi: Disconnected"));
    }
  }
  
  // Draw separator line
  display.drawFastHLine(0, HEADER_HEIGHT, SCREEN_WIDTH, COLOR_BORDER);
}

void drawFooter() {
  int footerY = SCREEN_HEIGHT - FOOTER_HEIGHT;
  
  // Footer background
  display.fillRect(0, footerY, SCREEN_WIDTH, FOOTER_HEIGHT, COLOR_BG_SECONDARY);
  display.drawFastHLine(0, footerY, SCREEN_WIDTH, COLOR_BORDER);
  
  // Navigation buttons
  int btnWidth = (SCREEN_WIDTH - 40) / 5;
  int btnY = footerY + 5;
  int btnH = FOOTER_HEIGHT - 10;
  
  drawButton(5, btnY, btnWidth, btnH, "Home", 
    currentScreen == SCREEN_HOME ? COLOR_ACCENT : COLOR_BG_CARD, 100);
  drawButton(5 + btnWidth + 5, btnY, btnWidth, btnH, "Zones",
    currentScreen == SCREEN_ZONES ? COLOR_ACCENT : COLOR_BG_CARD, 101);
  drawButton(5 + (btnWidth + 5) * 2, btnY, btnWidth, btnH, "Schedule",
    currentScreen == SCREEN_SCHEDULES ? COLOR_ACCENT : COLOR_BG_CARD, 102);
  drawButton(5 + (btnWidth + 5) * 3, btnY, btnWidth, btnH, "System",
    currentScreen == SCREEN_SYSTEM ? COLOR_ACCENT : COLOR_BG_CARD, 103);
  drawButton(5 + (btnWidth + 5) * 4, btnY, btnWidth, btnH, "Network",
    currentScreen == SCREEN_NETWORK ? COLOR_ACCENT : COLOR_BG_CARD, 104);
}

// ============================================================================
// HOME SCREEN
// ============================================================================

void drawHomeScreen() {
  int contentY = HEADER_HEIGHT + 10;
  int contentH = SCREEN_HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT - 20;
  
  // Welcome message
  display.setTextSize(2);
  display.setTextColor(COLOR_TEXT);
  display.setCursor(20, contentY);
  display.print(F("Irrigation System Status"));
  
  // System overview cards
  int cardY = contentY + 35;
  int cardW = (SCREEN_WIDTH - 30) / 2;
  int cardH = 80;
  
  // Active zones card
  drawCard(10, cardY, cardW, cardH, "Active Zones");
  display.setTextSize(4);
  display.setTextColor(COLOR_ACCENT);
  int activeCount = 0;
  for (int i = 0; i < sysConfig.zones; i++) {
    if (relayStatus[i]) activeCount++;
  }
  display.setCursor(cardW / 2 - 30, cardY + 35);
  display.print(activeCount);
  display.setTextSize(2);
  display.setTextColor(COLOR_TEXT_DIM);
  display.print(F(" / "));
  display.print(sysConfig.zones);
  
  // Today's total watering card
  drawCard(20 + cardW, cardY, cardW, cardH, "Today's Watering");
  int totalToday = 0;
  for (int i = 0; i < sysConfig.zones; i++) {
    totalToday += zones[i].totalWateringToday;
  }
  display.setTextSize(3);
  display.setTextColor(COLOR_ACCENT);
  display.setCursor(20 + cardW + (cardW / 2) - 40, cardY + 35);
  display.print(totalToday);
  display.setTextSize(2);
  display.setTextColor(COLOR_TEXT_DIM);
  display.print(F(" min"));
  
  // Zone status list
  int listY = cardY + cardH + 15;
  display.setTextSize(1);
  display.setTextColor(COLOR_TEXT);
  display.setCursor(20, listY);
  display.print(F("Zone Status:"));
  
  listY += 20;
  int zoneH = 25;
  int maxZonesVisible = (SCREEN_HEIGHT - listY - FOOTER_HEIGHT - 10) / zoneH;
  
  for (int i = 0; i < min((int)sysConfig.zones, maxZonesVisible); i++) {
    int y = listY + (i * zoneH);
    
    // Zone number and name
    display.setTextColor(COLOR_TEXT_DIM);
    display.setCursor(20, y + 5);
    display.print(i + 1);
    display.print(F(": "));
    display.setTextColor(COLOR_TEXT);
    display.print(zones[i].name);
    
    // Moisture bar
    int barX = 150;
    int barW = 150;
    drawProgressBar(barX, y + 2, barW, 20, soilMoisture[i], 100,
      soilMoisture[i] < zones[i].moistureLow ? COLOR_DANGER :
      soilMoisture[i] > zones[i].moistureHigh ? COLOR_SUCCESS : COLOR_WARNING);
    
    // Moisture percentage
    display.setTextColor(COLOR_TEXT);
    display.setCursor(barX + barW + 10, y + 5);
    display.print(soilMoisture[i]);
    display.print(F("%"));
    
    // Status indicator
    if (relayStatus[i]) {
      drawStatusBadge(barX + barW + 60, y + 2, "ON", COLOR_SUCCESS);
      // Show runtime
      unsigned long runtime = (millis() - zones[i].wateringStart) / 60000;
      display.setTextColor(COLOR_TEXT_DIM);
      display.setCursor(barX + barW + 110, y + 5);
      display.print(runtime);
      display.print(F("m"));
    } else {
      drawStatusBadge(barX + barW + 60, y + 2, "OFF", COLOR_TEXT_DIM);
    }
  }
  
  // Quick action button
  if (sysConfig.zones > maxZonesVisible) {
    drawButton(20, SCREEN_HEIGHT - FOOTER_HEIGHT - 45, 120, 35,
      "View All Zones", COLOR_ACCENT, 110);
  }
}

// ============================================================================
// ZONES SCREEN
// ============================================================================

void drawZonesScreen() {
  int contentY = HEADER_HEIGHT + 10;
  
  display.setTextSize(2);
  display.setTextColor(COLOR_TEXT);
  display.setCursor(20, contentY);
  display.print(F("All Zones"));
  
  // Zone cards in grid
  int cardW = 145;
  int cardH = 110;
  int startY = contentY + 35;
  int cols = 3;
  
  int visibleZones = min((int)sysConfig.zones, 6);  // Show 6 zones (2 rows of 3)
  
  for (int i = 0; i < visibleZones; i++) {
    int row = i / cols;
    int col = i % cols;
    int x = 10 + col * (cardW + 10);
    int y = startY + row * (cardH + 10);
    
    drawZoneCard(x, y, cardW, cardH, i);
  }
  
  // Scroll buttons if more zones
  if (sysConfig.zones > 6) {
    drawButton(360, startY + cardH + 20, 100, 30, "More...", COLOR_ACCENT, 120);
  }
}

void drawZoneCard(int x, int y, int w, int h, int zoneId) {
  // Card background with border
  display.fillRoundRect(x, y, w, h, CARD_RADIUS, COLOR_BG_CARD);
  display.drawRoundRect(x, y, w, h, CARD_RADIUS,
    relayStatus[zoneId] ? COLOR_SUCCESS : COLOR_BORDER);
  
  // Zone number badge
  display.fillCircle(x + 20, y + 20, 15,
    zones[zoneId].enabled ? COLOR_ACCENT : COLOR_TEXT_DIM);
  display.setTextColor(COLOR_BG_PRIMARY);
  display.setTextSize(2);
  display.setCursor(x + (zoneId < 9 ? 16 : 13), y + 13);
  display.print(zoneId + 1);
  
  // Zone name
  display.setTextSize(1);
  display.setTextColor(COLOR_TEXT);
  display.setCursor(x + 40, y + 15);
  display.print(zones[zoneId].name);
  
  // Moisture display
  display.setTextSize(3);
  display.setTextColor(COLOR_ACCENT);
  display.setCursor(x + 20, y + 40);
  display.print(soilMoisture[zoneId]);
  display.setTextSize(1);
  display.setTextColor(COLOR_TEXT_DIM);
  display.print(F("%"));
  
  // Progress bar
  drawProgressBar(x + 10, y + 70, w - 20, 8, soilMoisture[zoneId], 100,
    soilMoisture[zoneId] < zones[zoneId].moistureLow ? COLOR_DANGER :
    soilMoisture[zoneId] > zones[zoneId].moistureHigh ? COLOR_SUCCESS : COLOR_WARNING);
  
  // Status and mode
  display.setTextSize(1);
  display.setTextColor(COLOR_TEXT_DIM);
  display.setCursor(x + 10, y + 85);
  display.print(modeNames[zones[zoneId].mode]);
  
  if (relayStatus[zoneId]) {
    display.setTextColor(COLOR_SUCCESS);
    display.setCursor(x + 80, y + 85);
    display.print(F("WATERING"));
  }
  
  // Make card touchable
  addButton(x, y, w, h, "", COLOR_BG_CARD, 200 + zoneId);
}

// ============================================================================
// ZONE DETAIL SCREEN
// ============================================================================

void drawZoneDetailScreen() {
  int contentY = HEADER_HEIGHT + 10;
  
  display.setTextSize(2);
  display.setTextColor(COLOR_TEXT);
  display.setCursor(20, contentY);
  display.print(F("Zone "));
  display.print(selectedZone + 1);
  display.print(F(": "));
  display.print(zones[selectedZone].name);
  
  // Back button
  drawButton(380, contentY - 5, 80, 30, "< Back", COLOR_BG_SECONDARY, 300);
  
  int detailY = contentY + 35;
  
  // Large moisture display
  display.setTextSize(5);
  display.setTextColor(COLOR_ACCENT);
  display.setCursor(30, detailY);
  display.print(soilMoisture[selectedZone]);
  display.setTextSize(2);
  display.setTextColor(COLOR_TEXT_DIM);
  display.print(F("%"));
  
  // Status badge
  if (relayStatus[selectedZone]) {
    drawStatusBadge(30, detailY + 60, "WATERING", COLOR_SUCCESS);
    unsigned long runtime = (millis() - zones[selectedZone].wateringStart) / 60000;
    display.setTextSize(1);
    display.setTextColor(COLOR_TEXT);
    display.setCursor(120, detailY + 65);
    display.print(F("Running: "));
    display.print(runtime);
    display.print(F(" minutes"));
  } else {
    drawStatusBadge(30, detailY + 60, "OFF", COLOR_TEXT_DIM);
  }
  
  // Graph of moisture history
  drawGraph(200, detailY, 260, 80, zones[selectedZone].moistureHistory, 60, 100);
  
  // Configuration section
  int configY = detailY + 95;
  
  display.setTextSize(1);
  display.setTextColor(COLOR_TEXT);
  display.setCursor(30, configY);
  display.print(F("Mode: "));
  display.setTextColor(COLOR_ACCENT);
  display.print(modeNamesLong[zones[selectedZone].mode]);
  
  display.setTextColor(COLOR_TEXT);
  display.setCursor(30, configY + 20);
  display.print(F("Thresholds: "));
  display.setTextColor(COLOR_ACCENT);
  display.print(zones[selectedZone].moistureLow);
  display.print(F("% - "));
  display.print(zones[selectedZone].moistureHigh);
  display.print(F("%"));
  
  display.setTextColor(COLOR_TEXT);
  display.setCursor(30, configY + 40);
  display.print(F("Today: "));
  display.setTextColor(COLOR_ACCENT);
  display.print(zones[selectedZone].totalWateringToday);
  display.print(F(" minutes"));
  
  display.setTextColor(COLOR_TEXT);
  display.setCursor(30, configY + 60);
  display.print(F("Max Duration: "));
  display.setTextColor(COLOR_ACCENT);
  display.print(zones[selectedZone].maxDuration);
  display.print(F(" min"));
  
  // Control buttons
  int btnY = SCREEN_HEIGHT - FOOTER_HEIGHT - 50;
  
  if (relayStatus[selectedZone]) {
    drawButton(30, btnY, 120, 40, "Turn OFF", COLOR_DANGER, 310);
  } else {
    drawButton(30, btnY, 120, 40, "Turn ON", COLOR_SUCCESS, 310);
  }
  
  drawButton(160, btnY, 120, 40, "Configure", COLOR_ACCENT, 311);
  drawButton(290, btnY, 120, 40, "Calibrate", COLOR_WARNING, 312);
}

// ============================================================================
// UTILITY DRAWING FUNCTIONS
// ============================================================================

void drawCard(int x, int y, int w, int h, const char* title) {
  display.fillRoundRect(x, y, w, h, CARD_RADIUS, COLOR_BG_CARD);
  display.drawRoundRect(x, y, w, h, CARD_RADIUS, COLOR_BORDER);
  
  display.setTextSize(1);
  display.setTextColor(COLOR_TEXT_DIM);
  display.setCursor(x + 10, y + 8);
  display.print(title);
}

void drawButton(int x, int y, int w, int h, const char* label, uint16_t color, int id) {
  display.fillRoundRect(x, y, w, h, BUTTON_RADIUS, color);
  
  // Text centering
  display.setTextSize(1);
  int16_t x1, y1;
  uint16_t tw, th;
  display.getTextBounds(label, 0, 0, &x1, &y1, &tw, &th);
  
  display.setTextColor(COLOR_TEXT);
  display.setCursor(x + (w - tw) / 2, y + (h - th) / 2 + 2);
  display.print(label);
  
  addButton(x, y, w, h, label, color, id);
}

void drawProgressBar(int x, int y, int w, int h, int value, int max, uint16_t color) {
  // Background
  display.fillRoundRect(x, y, w, h, h / 2, COLOR_BG_PRIMARY);
  
  // Progress fill
  int fillW = (w * value) / max;
  if (fillW > 0) {
    display.fillRoundRect(x, y, fillW, h, h / 2, color);
  }
  
  // Border
  display.drawRoundRect(x, y, w, h, h / 2, COLOR_BORDER);
}

void drawStatusBadge(int x, int y, const char* text, uint16_t color) {
  int w = strlen(text) * 6 + 12;
  int h = 18;
  
  display.fillRoundRect(x, y, w, h, 4, color);
  display.setTextSize(1);
  display.setTextColor(COLOR_BG_PRIMARY);
  display.setCursor(x + 6, y + 5);
  display.print(text);
}

void drawGraph(int x, int y, int w, int h, int* data, int dataSize, int max) {
  // Graph background
  display.fillRect(x, y, w, h, COLOR_BG_PRIMARY);
  display.drawRect(x, y, w, h, COLOR_BORDER);
  
  // Grid lines
  display.setTextColor(COLOR_TEXT_DIM);
  for (int i = 0; i <= 4; i++) {
    int gridY = y + (h * i) / 4;
    display.drawFastHLine(x, gridY, w, COLOR_BORDER);
    
    display.setTextSize(1);
    display.setCursor(x + 5, gridY + 2);
    display.print(max - (max * i) / 4);
  }
  
  // Plot data
  if (dataSize > 1) {
    int stepX = w / (dataSize - 1);
    for (int i = 0; i < dataSize - 1; i++) {
      int x1 = x + (i * w) / (dataSize - 1);
      int y1 = y + h - ((data[i] * h) / max);
      int x2 = x + ((i + 1) * w) / (dataSize - 1);
      int y2 = y + h - ((data[i + 1] * h) / max);
      
      display.drawLine(x1, y1, x2, y2, COLOR_ACCENT);
    }
  }
  
  // Label
  display.setTextSize(1);
  display.setTextColor(COLOR_TEXT_DIM);
  display.setCursor(x + 5, y + h - 10);
  display.print(F("History (60 readings)"));
}

// ============================================================================
// BUTTON MANAGEMENT
// ============================================================================

void clearButtons() {
  buttonCount = 0;
}

void addButton(int x, int y, int w, int h, const char* label, uint16_t color, int id) {
  if (buttonCount < 20) {
    buttons[buttonCount].x = x;
    buttons[buttonCount].y = y;
    buttons[buttonCount].w = w;
    buttons[buttonCount].h = h;
    buttons[buttonCount].label = label;
    buttons[buttonCount].color = color;
    buttons[buttonCount].visible = true;
    buttons[buttonCount].id = id;
    buttonCount++;
  }
}

// ============================================================================
// TOUCH HANDLING
// ============================================================================

void handleTouch() {
  uint8_t contacts;
  GDTpoint_t points[5];
  
  contacts = touchDetector.getTouchPoints(points);
  
  if (contacts > 0) {
    int touchX = points[0].x;
    int touchY = points[0].y;
    
    // Wake screen if sleeping
    if (!screenActive) {
      screenActive = true;
      needsRedraw = true;
      lastTouchTime = millis();
      return;
    }
    
    lastTouchTime = millis();
    
    int buttonId = getTouchedButton(touchX, touchY);
    
    if (buttonId >= 0) {
      // Handle button press
      handleButtonPress(buttonId);
      needsRedraw = true;
      delay(200);  // Simple debounce
    }
  }
}

int getTouchedButton(int x, int y) {
  for (int i = 0; i < buttonCount; i++) {
    if (buttons[i].visible &&
        x >= buttons[i].x && x <= buttons[i].x + buttons[i].w &&
        y >= buttons[i].y && y <= buttons[i].y + buttons[i].h) {
      return buttons[i].id;
    }
  }
  return -1;
}

void handleButtonPress(int id) {
  // Navigation buttons (100-104)
  if (id == 100) {
    currentScreen = SCREEN_HOME;
  } else if (id == 101) {
    currentScreen = SCREEN_ZONES;
  } else if (id == 102) {
    currentScreen = SCREEN_SCHEDULES;
  } else if (id == 103) {
    currentScreen = SCREEN_SYSTEM;
  } else if (id == 104) {
    currentScreen = SCREEN_NETWORK;
  }
  
  // Zone card selections (200-213)
  else if (id >= 200 && id < 200 + MAX_ZONES) {
    selectedZone = id - 200;
    currentScreen = SCREEN_ZONE_DETAIL;
  }
  
  // Zone detail actions (310-312)
  else if (id == 300) {  // Back button
    currentScreen = SCREEN_ZONES;
  }
  else if (id == 310) {  // Toggle zone
    if (relayStatus[selectedZone]) {
      stopWatering(selectedZone);
    } else {
      startWatering(selectedZone);
    }
  }
  else if (id == 311) {  // Configure
    // TODO: Implement configuration screen
  }
  else if (id == 312) {  // Calibrate
    currentScreen = SCREEN_CALIBRATE;
  }
  
  // Calibration button handlers (320-321)
  else if (id == 320) {  // Set dry calibration value
    zones[selectedZone].dryCal = analogRead(analogPins[selectedZone]);
    saveZoneConfig(selectedZone);
  }
  else if (id == 321) {  // Set wet calibration value
    zones[selectedZone].wetCal = analogRead(analogPins[selectedZone]);
    saveZoneConfig(selectedZone);
  }
}

// ============================================================================
// PLACEHOLDER SCREENS (To be implemented)
// ============================================================================

void drawSchedulesScreen() {
  int contentY = HEADER_HEIGHT + 10;
  display.setTextSize(2);
  display.setTextColor(COLOR_TEXT);
  display.setCursor(20, contentY);
  display.print(F("Schedules"));
  
  display.setTextSize(1);
  display.setTextColor(COLOR_TEXT_DIM);
  display.setCursor(20, contentY + 40);
  display.print(F("Schedule configuration coming soon..."));
}

void drawSystemScreen() {
  int contentY = HEADER_HEIGHT + 10;
  display.setTextSize(2);
  display.setTextColor(COLOR_TEXT);
  display.setCursor(20, contentY);
  display.print(F("System Settings"));
  
  int y = contentY + 40;
  display.setTextSize(1);
  
  display.setTextColor(COLOR_TEXT);
  display.setCursor(20, y);
  display.print(F("Number of Zones: "));
  display.setTextColor(COLOR_ACCENT);
  display.print(sysConfig.zones);
  
  y += 25;
  display.setTextColor(COLOR_TEXT);
  display.setCursor(20, y);
  display.print(F("Temperature Display: "));
  display.setTextColor(COLOR_ACCENT);
  display.print(sysConfig.tempDisplayF ? "Fahrenheit" : "Celsius");
  
  y += 25;
  display.setTextColor(COLOR_TEXT);
  display.setCursor(20, y);
  display.print(F("Freeze Protection: "));
  display.setTextColor(sysConfig.freezeProtect ? COLOR_SUCCESS : COLOR_DANGER);
  display.print(sysConfig.freezeProtect ? "Enabled" : "Disabled");
  
  if (sysConfig.freezeProtect) {
    display.setTextColor(COLOR_TEXT);
    display.setCursor(20, y + 15);
    display.print(F("  Freeze Temp: "));
    display.setTextColor(COLOR_ACCENT);
    display.print(sysConfig.freezeTemp);
    display.print(sysConfig.tempDisplayF ? "F" : "C");
    y += 15;
  }
  
  y += 25;
  display.setTextColor(COLOR_TEXT);
  display.setCursor(20, y);
  display.print(F("Max Daily Watering: "));
  display.setTextColor(COLOR_ACCENT);
  display.print(sysConfig.maxDailyWatering);
  display.print(F(" minutes"));
}

void drawNetworkScreen() {
  int contentY = HEADER_HEIGHT + 10;
  display.setTextSize(2);
  display.setTextColor(COLOR_TEXT);
  display.setCursor(20, contentY);
  display.print(F("Network Status"));
  
  int y = contentY + 40;
  display.setTextSize(1);
  
  display.setTextColor(COLOR_TEXT);
  display.setCursor(20, y);
  display.print(F("WiFi Status: "));
  
  if (WiFi.status() == WL_CONNECTED) {
    display.setTextColor(COLOR_SUCCESS);
    display.print(F("Connected"));
    
    y += 25;
    display.setTextColor(COLOR_TEXT);
    display.setCursor(20, y);
    display.print(F("IP Address: "));
    display.setTextColor(COLOR_ACCENT);
    display.print(WiFi.localIP());
    
    y += 25;
    display.setTextColor(COLOR_TEXT);
    display.setCursor(20, y);
    display.print(F("Signal Strength: "));
    display.setTextColor(COLOR_ACCENT);
    display.print(WiFi.RSSI());
    display.print(F(" dBm"));
  } else {
    display.setTextColor(COLOR_DANGER);
    display.print(F("Disconnected"));
  }
  
  y += 40;
  display.setTextColor(COLOR_TEXT);
  display.setCursor(20, y);
  display.print(F("Web Interface: "));
  if (WiFi.status() == WL_CONNECTED) {
    display.setTextColor(COLOR_ACCENT);
    display.print(F("http://"));
    display.print(WiFi.localIP());
  } else {
    display.setTextColor(COLOR_DANGER);
    display.print(F("Unavailable"));
  }
}

void drawCalibrateScreen() {
  int contentY = HEADER_HEIGHT + 10;
  display.setTextSize(2);
  display.setTextColor(COLOR_TEXT);
  display.setCursor(20, contentY);
  display.print(F("Calibrate Zone "));
  display.print(selectedZone + 1);
  
  drawButton(350, contentY - 5, 100, 30, "< Back", COLOR_BG_SECONDARY, 300);
  
  int y = contentY + 50;
  display.setTextSize(1);
  display.setTextColor(COLOR_TEXT);
  display.setCursor(20, y);
  display.print(F("Current raw reading: "));
  display.setTextColor(COLOR_ACCENT);
  display.print(analogRead(analogPins[selectedZone]));
  
  y += 30;
  display.setTextColor(COLOR_TEXT);
  display.setCursor(20, y);
  display.print(F("1. Remove sensor from soil and let dry"));
  
  y += 20;
  display.setCursor(20, y);
  display.print(F("2. Press 'Set Dry' when reading stabilizes"));
  
  y += 30;
  drawButton(20, y, 150, 40, "Set Dry Value", COLOR_WARNING, 320);
  
  y += 50;
  display.setTextColor(COLOR_TEXT);
  display.setCursor(20, y);
  display.print(F("Dry calibration: "));
  display.setTextColor(COLOR_ACCENT);
  display.print(zones[selectedZone].dryCal);
  
  y += 40;
  display.setTextColor(COLOR_TEXT);
  display.setCursor(20, y);
  display.print(F("3. Submerge sensor in water"));
  
  y += 20;
  display.setCursor(20, y);
  display.print(F("4. Press 'Set Wet' when reading stabilizes"));
  
  y += 30;
  drawButton(20, y, 150, 40, "Set Wet Value", COLOR_SUCCESS, 321);
  
  display.setTextColor(COLOR_TEXT);
  display.setCursor(20, y + 50);
  display.print(F("Wet calibration: "));
  display.setTextColor(COLOR_ACCENT);
  display.print(zones[selectedZone].wetCal);
}

void drawGraphsScreen() {
  int contentY = HEADER_HEIGHT + 10;
  display.setTextSize(2);
  display.setTextColor(COLOR_TEXT);
  display.setCursor(20, contentY);
  display.print(F("Moisture Graphs"));
  
  // Draw graphs for first 4 zones
  int graphW = 220;
  int graphH = 90;
  int y = contentY + 35;
  
  for (int i = 0; i < min(4, (int)sysConfig.zones); i++) {
    int x = (i % 2) * (graphW + 20) + 20;
    int gy = y + (i / 2) * (graphH + 20);
    
    display.setTextSize(1);
    display.setTextColor(COLOR_TEXT);
    display.setCursor(x, gy);
    display.print(F("Zone "));
    display.print(i + 1);
    display.print(F(": "));
    display.print(zones[i].name);
    
    drawGraph(x, gy + 15, graphW, graphH, zones[i].moistureHistory, 60, 100);
  }
}

// ============================================================================
// IRRIGATION LOGIC (Same as V3.1)
// ============================================================================

void updateIrrigation() {
  if (sysConfig.freezeProtect && temp < sysConfig.freezeTemp) {
    for (int i = 0; i < sysConfig.zones; i++) {
      if (relayStatus[i]) stopWatering(i);
    }
    return;
  }
  
  for (int i = 0; i < sysConfig.zones; i++) {
    if (!zones[i].enabled) continue;
    checkSafetyLimits(i);
    
    switch (zones[i].mode) {
      case MODE_MOISTURE_ONLY: checkMoistureBased(i); break;
      case MODE_TIME_ONLY: checkTimeBased(i); break;
      case MODE_HYBRID: checkHybridMode(i); break;
      case MODE_MANUAL: break;
    }
  }
}

void checkMoistureBased(int zone) {
  int effectiveLow = zones[zone].moistureLow;
  int effectiveHigh = zones[zone].moistureHigh;
  
  if (temp >= sysConfig.tempSwitch) {
    effectiveLow += sysConfig.tempAdjLow;
    effectiveHigh += sysConfig.tempAdjHi;
  }
  
  if (soilMoisture[zone] <= effectiveLow && !relayStatus[zone]) {
    startWatering(zone);
  }
  if (soilMoisture[zone] >= effectiveHigh && relayStatus[zone]) {
    stopWatering(zone);
  }
}

void checkTimeBased(int zone) {
  if (!zones[zone].schedule.enabled) return;
  if (isInScheduledWindow(zone) && !relayStatus[zone]) {
    if (day() != day(zones[zone].lastWatered)) {
      startWatering(zone);
    }
  }
  if (relayStatus[zone]) {
    unsigned long runTime = (millis() - zones[zone].wateringStart) / 60000;
    if (runTime >= zones[zone].schedule.duration) {
      stopWatering(zone);
    }
  }
}

void checkHybridMode(int zone) {
  if (!zones[zone].schedule.enabled) {
    checkMoistureBased(zone);
    return;
  }
  if (isInScheduledWindow(zone)) {
    checkMoistureBased(zone);
  } else {
    if (relayStatus[zone]) stopWatering(zone);
  }
}

bool isInScheduledWindow(int zone) {
  if (!zones[zone].schedule.enabled) return false;
  if (timeStatus() != timeSet) return false;
  
  int today = weekday() - 1;
  if (!zones[zone].schedule.days[today]) return false;
  
  int currentMinutes = hour() * 60 + minute();
  int startMinutes = zones[zone].schedule.startHour * 60 + zones[zone].schedule.startMinute;
  int endMinutes = startMinutes + zones[zone].schedule.duration;
  
  return (currentMinutes >= startMinutes && currentMinutes < endMinutes);
}

void checkSafetyLimits(int zone) {
  if (relayStatus[zone]) {
    unsigned long runTime = (millis() - zones[zone].wateringStart) / 60000;
    if (runTime >= zones[zone].maxDuration) {
      stopWatering(zone);
    }
    if (zones[zone].totalWateringToday >= sysConfig.maxDailyWatering) {
      stopWatering(zone);
    }
  }
}

void startWatering(int zone) {
  digitalWrite(relayPinStart + zone, sysConfig.relayOn);
  relayStatus[zone] = true;
  zones[zone].wateringStart = millis();
  Serial.print(F("Zone "));
  Serial.print(zone + 1);
  Serial.println(F(" started"));
}

void stopWatering(int zone) {
  digitalWrite(relayPinStart + zone, !sysConfig.relayOn);
  relayStatus[zone] = false;
  
  if (zones[zone].wateringStart > 0) {
    unsigned long duration = (millis() - zones[zone].wateringStart) / 60000;
    zones[zone].totalWateringToday += duration;
    zones[zone].lastWatered = now();
    zones[zone].wateringStart = 0;
  }
}

// ============================================================================
// SENSOR READING
// ============================================================================

void readSensors() {
  // Read temperature and humidity
  int status = DHT.read();
  if (status == AM2315C_OK) {
    temp = DHT.getTemperature() * 9.0 / 5.0 + 32.0;
    humidity = DHT.getHumidity();
  }
  
  // Read soil moisture sensors
  for (int i = 0; i < sysConfig.zones; i++) {
    if (!zones[i].enabled) continue;
    
    long reading = analogRead(analogPins[i]);
    if (reading < zones[i].dryCal) reading = zones[i].dryCal;
    if (reading > zones[i].wetCal) reading = zones[i].wetCal;
    
    soilMoisture[i] = map(reading, zones[i].dryCal, zones[i].wetCal, 0, 100);
    
    // Update history for graphs
    updateMoistureHistory(i);
  }
}

void updateMoistureHistory(int zone) {
  zones[zone].moistureHistory[zones[zone].historyIndex] = soilMoisture[zone];
  zones[zone].historyIndex = (zones[zone].historyIndex + 1) % 60;
}

// ============================================================================
// NETWORK FUNCTIONS (Simplified for GIGA R1 WiFi)
// ============================================================================

void setupNetwork() {
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
    Serial.print(F("WiFi connected: "));
    Serial.println(WiFi.localIP());
    if (sysConfig.ntpEnabled) syncNTP();
  } else {
    Serial.println(F(" Failed!"));
    sysConfig.networkEnabled = false;
  }
}

void setupWebServer() {
  server.on("/", handleRoot);
  server.on("/api/status", HTTP_GET, handleGetStatus);
  server.on("/api/zones", HTTP_GET, handleGetZones);
  server.on("/api/zone", HTTP_POST, handleSetZone);
  server.on("/api/system", HTTP_GET, handleGetSystem);
  server.on("/api/system", HTTP_POST, handleSetSystem);
  server.on("/api/control", HTTP_POST, handleControlZone);
  
  server.begin();
  Serial.println(F("Web server started"));
}

// Web handlers - simplified versions from V3.1
void handleRoot() {
  server.send(200, "text/html", "<html><body><h1>Irrigation Controller</h1><p>API available at /api/</p></body></html>");
}

void handleGetStatus() {
  StaticJsonDocument<200> doc;
  doc["temperature"] = temp;
  doc["humidity"] = humidity;
  doc["time"] = getTimeString();
  
  int activeCount = 0;
  for (int i = 0; i < sysConfig.zones; i++) {
    if (relayStatus[i]) activeCount++;
  }
  doc["activeZones"] = activeCount;
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleGetZones() {
  DynamicJsonDocument doc(4096);
  JsonArray array = doc.to<JsonArray>();
  
  for (int i = 0; i < sysConfig.zones; i++) {
    JsonObject zone = array.createNestedObject();
    zone["id"] = i;
    zone["name"] = zones[i].name;
    zone["enabled"] = zones[i].enabled;
    zone["active"] = relayStatus[i];
    zone["moisture"] = soilMoisture[i];
    zone["mode"] = zones[i].mode;
    zone["low"] = zones[i].moistureLow;
    zone["high"] = zones[i].moistureHigh;
  }
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleSetZone() {
  if (server.hasArg("plain")) {
    StaticJsonDocument<512> doc;
    deserializeJson(doc, server.arg("plain"));
    
    int zoneId = doc["id"];
    if (zoneId >= 0 && zoneId < sysConfig.zones) {
      if (doc.containsKey("name")) {
        strncpy(zones[zoneId].name, doc["name"] | "", sizeof(zones[zoneId].name) - 1);
        zones[zoneId].name[sizeof(zones[zoneId].name) - 1] = '\0';
      }
      if (doc.containsKey("enabled")) zones[zoneId].enabled = doc["enabled"];
      if (doc.containsKey("mode")) zones[zoneId].mode = (WateringMode)(int)doc["mode"];
      if (doc.containsKey("low")) zones[zoneId].moistureLow = doc["low"];
      if (doc.containsKey("high")) zones[zoneId].moistureHigh = doc["high"];
      
      saveZoneConfig(zoneId);
      server.send(200, "application/json", "{\"success\":true}");
      return;
    }
  }
  server.send(400, "application/json", "{\"error\":\"Invalid data\"}");
}

void handleGetSystem() {
  StaticJsonDocument<512> doc;
  doc["zones"] = sysConfig.zones;
  doc["tempDisplayF"] = sysConfig.tempDisplayF;
  doc["freezeProtect"] = sysConfig.freezeProtect;
  doc["freezeTemp"] = sysConfig.freezeTemp;
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleSetSystem() {
  if (server.hasArg("plain")) {
    StaticJsonDocument<512> doc;
    deserializeJson(doc, server.arg("plain"));
    
    if (doc.containsKey("zones")) sysConfig.zones = doc["zones"];
    if (doc.containsKey("tempDisplayF")) sysConfig.tempDisplayF = doc["tempDisplayF"];
    
    saveSystemConfig();
    server.send(200, "application/json", "{\"success\":true}");
    return;
  }
  server.send(400, "application/json", "{\"error\":\"Invalid data\"}");
}

void handleControlZone() {
  if (server.hasArg("plain")) {
    StaticJsonDocument<128> doc;
    deserializeJson(doc, server.arg("plain"));
    
    int zoneId = doc["zone"];
    const char* action = doc["action"];
    
    if (strcmp(action, "toggle") == 0) {
      if (relayStatus[zoneId]) {
        stopWatering(zoneId);
      } else {
        startWatering(zoneId);
      }
    }
    
    server.send(200, "application/json", "{\"success\":true}");
  }
}

void syncNTP() {
  time_t ntpTime = getNTPTime();
  if (ntpTime > 0) {
    setTime(ntpTime + (sysConfig.timeZoneOffset * 3600));
  }
}

time_t getNTPTime() {
  const int NTP_PACKET_SIZE = 48;
  byte packetBuffer[NTP_PACKET_SIZE];
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011;
  
  udp.begin(ntpPort);
  udp.beginPacket(ntpServer, 123);
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
  
  delay(1000);
  
  if (udp.parsePacket()) {
    udp.read(packetBuffer, NTP_PACKET_SIZE);
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    udp.stop();
    return secsSince1900 - 2208988800UL;
  }
  udp.stop();
  return 0;
}

// ============================================================================
// UTILITIES
// ============================================================================

const char* getTimeString() {
  static char timeStr[9];
  sprintf(timeStr, "%02d:%02d:%02d", hour(), minute(), second());
  return timeStr;
}

const char* getDateString() {
  static char dateStr[11];
  sprintf(dateStr, "%02d/%02d/%04d", month(), day(), year());
  return dateStr;
}

void logData() {
  Serial.println(F("=== DATA LOG ==="));
  Serial.print(F("Time: "));
  Serial.println(getTimeString());
  Serial.print(F("Temp: "));
  Serial.print(temp);
  Serial.print(F("F, RH: "));
  Serial.print(humidity);
  Serial.println(F("%"));
  
  for (int i = 0; i < sysConfig.zones; i++) {
    if (!zones[i].enabled) continue;
    Serial.print(F("Zone "));
    Serial.print(i + 1);
    Serial.print(F(": "));
    Serial.print(soilMoisture[i]);
    Serial.print(F("% - "));
    Serial.println(relayStatus[i] ? "ON" : "OFF");
  }
}

// ============================================================================
// EEPROM CONFIGURATION
// ============================================================================

void loadConfiguration() {
  EEPROM.get(EEPROM_SYSTEM_CONFIG, sysConfig);
  if (sysConfig.zones < 1 || sysConfig.zones > MAX_ZONES) {
    resetToDefaults();
    saveConfiguration();
  } else {
    for (int i = 0; i < MAX_ZONES; i++) {
      loadZoneConfig(i);
    }
  }
}

void saveConfiguration() {
  saveSystemConfig();
  for (int i = 0; i < sysConfig.zones; i++) {
    saveZoneConfig(i);
  }
}

void loadZoneConfig(int zoneNum) {
  if (zoneNum < 0 || zoneNum >= MAX_ZONES) return;
  int addr = EEPROM_ZONE_BASE + (zoneNum * 150);
  EEPROM.get(addr, zones[zoneNum]);
}

void saveZoneConfig(int zoneNum) {
  if (zoneNum < 0 || zoneNum >= MAX_ZONES) return;
  int addr = EEPROM_ZONE_BASE + (zoneNum * 150);
  EEPROM.put(addr, zones[zoneNum]);
}

void loadSystemConfig() {
  EEPROM.get(EEPROM_SYSTEM_CONFIG, sysConfig);
}

void saveSystemConfig() {
  EEPROM.put(EEPROM_SYSTEM_CONFIG, sysConfig);
}

void resetToDefaults() {
  sysConfig.zones = 6;
  sysConfig.tempDisplayF = true;
  sysConfig.tempSwitch = 92;
  sysConfig.tempAdjLow = 6;
  sysConfig.tempAdjHi = 4;
  sysConfig.relayOn = false;  // false = LOW activates relay (NC configuration)
  sysConfig.networkEnabled = true;
  sysConfig.ntpEnabled = true;
  sysConfig.timeZoneOffset = -5;
  sysConfig.use24Hour = false;
  sysConfig.loggingEnabled = true;
  sysConfig.logInterval = 60;
  sysConfig.maxDailyWatering = 180;
  sysConfig.freezeProtect = true;
  sysConfig.freezeTemp = 35;
  sysConfig.sensorReadInterval = 5000;
  sysConfig.screenTimeout = 300;
  sysConfig.screenSaver = true;
  
  for (int i = 0; i < MAX_ZONES; i++) {
    zones[i].enabled = (i < sysConfig.zones);
    sprintf(zones[i].name, "Zone %d", i + 1);
    zones[i].dryCal = 125;
    zones[i].wetCal = 550;
    zones[i].moistureLow = 40;
    zones[i].moistureHigh = 70;
    zones[i].mode = MODE_MOISTURE_ONLY;
    zones[i].maxDuration = 60;
    zones[i].lastWatered = 0;
    zones[i].wateringStart = 0;
    zones[i].totalWateringToday = 0;
    zones[i].historyIndex = 0;
    
    for (int j = 0; j < 60; j++) {
      zones[i].moistureHistory[j] = 0;
    }
    
    zones[i].schedule.enabled = false;
    zones[i].schedule.startHour = 6;
    zones[i].schedule.startMinute = 0;
    zones[i].schedule.duration = 30;
    for (int j = 0; j < 7; j++) {
      zones[i].schedule.days[j] = false;
    }
  }
}
