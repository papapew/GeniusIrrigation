/*
  Soil Moisture Irrigation Controller with Web Interface
  Version 3.1 LEAN CONFIGURABLE
  
  *** FULLY CONFIGURABLE - NO CODE CHANGES NEEDED ***
  
  Web Interface Features:
  - Real-time zone status monitoring
  - Toggle any zone on/off manually
  - Configure number of zones
  - Per-zone settings (name, thresholds, calibration, mode)
  - System settings (relay type, temp units, freeze protect)
  - Winterize function
  - All settings saved to EEPROM
  
  Memory Optimized:
  - HTML stored in PROGMEM (flash)
  - Streaming responses (no String buffering)
  - Compact data structures
  - ~3KB RAM usage
  
  Hardware: Arduino Mega 2560 + Ethernet Shield (W5100/W5500)
*/

#include "AM2315C.h"
#include <Arduino.h>
#include <U8g2lib.h>
#include <EEPROM.h>
#include <SPI.h>
#include <Wire.h>
#include <TimeLib.h>
#include <ArduinoJson.h>  // v6.21.x required!
#include <Ethernet.h>
#include <EthernetUdp.h>

// Optional RTC
#define USE_RTC
#ifdef USE_RTC
  #include <RTClib.h>
  RTC_DS3231 rtc;
#endif

#define VERSION "3.1LC"

// ============================================================================
// HARDWARE
// ============================================================================

AM2315C DHT;
U8X8_SSD1309_128X64_NONAME0_4W_HW_SPI u8x8(47, 49, 48);

const int analogPins[16] PROGMEM = {A0,A1,A2,A3,A4,A5,A6,A7,A8,A9,A10,A11,A12,A13,A14,A15};

// ============================================================================
// NETWORK
// ============================================================================

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
EthernetServer server(80);
EthernetUDP udp;
unsigned long lastNTPSync = 0;

// ============================================================================
// COMPACT DATA STRUCTURES
// ============================================================================

enum WateringMode : uint8_t { MODE_MOISTURE=0, MODE_TIME=1, MODE_HYBRID=2, MODE_MANUAL=3 };
const char* modeNames[] = {"Moisture", "Time", "Hybrid", "Manual"};

struct ZoneConfig {
  char name[10];
  int16_t dryCal;
  int16_t wetCal;
  uint8_t moistureLow;
  uint8_t moistureHigh;
  uint8_t mode;
  uint8_t enabled;
  uint8_t maxDuration;
};

struct SystemConfig {
  uint8_t zones;
  uint8_t tempDisplayF;
  uint8_t relayOn;
  uint8_t ntpEnabled;
  uint8_t freezeProtect;
  int8_t timeZoneOffset;
  uint8_t freezeTemp;
  uint8_t tempSwitch;
  uint8_t tempAdjLow;
  uint8_t tempAdjHi;
  uint16_t checksum;
};

ZoneConfig zones[16];
SystemConfig sysConfig;

// Runtime
int16_t soilMoisture[16];
uint16_t relayStatus = 0;  // Bitmask
float temp = 0, humidity = 0;
unsigned long lastSensorRead = 0;

#define EEPROM_SYS_ADDR 0
#define EEPROM_ZONE_ADDR 32

// Helpers
inline bool isRelayOn(uint8_t z) { return relayStatus & (1 << z); }
inline void setRelay(uint8_t z, bool on) { 
  if (on) relayStatus |= (1 << z); 
  else relayStatus &= ~(1 << z); 
}

// ============================================================================
// HTML IN PROGMEM - Full configuration interface
// ============================================================================

const char HTML_HEAD[] PROGMEM = R"rawhtml(<!DOCTYPE html>
<html><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Irrigation Controller</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:system-ui,sans-serif;background:#111;color:#eee;padding:15px;max-width:800px;margin:0 auto}
h1,h2{color:#4af;margin:10px 0}h2{font-size:1.1em;border-bottom:1px solid #333;padding-bottom:5px}
.card{background:#1a1a2e;border-radius:8px;padding:15px;margin:15px 0}
.zone{display:flex;justify-content:space-between;align-items:center;padding:10px;margin:5px 0;background:#252540;border-radius:5px}
.on{color:#0f0;font-weight:bold}.off{color:#666}
button{background:#07f;color:#fff;border:none;padding:8px 15px;border-radius:5px;cursor:pointer;margin:3px}
button:hover{background:#05d}button.off{background:#333}button.danger{background:#c33}
input,select{background:#222;color:#fff;border:1px solid #444;padding:8px;border-radius:5px;margin:3px}
input[type=text]{width:100px}input[type=number]{width:70px}
.row{display:flex;flex-wrap:wrap;gap:10px;align-items:center;margin:8px 0}
.label{color:#888;min-width:100px}
.tabs{display:flex;gap:5px;margin-bottom:15px}
.tab{padding:10px 20px;background:#222;border:none;color:#888;cursor:pointer;border-radius:5px 5px 0 0}
.tab.active{background:#1a1a2e;color:#4af}
.panel{display:none}.panel.active{display:block}
.status{font-size:1.5em;color:#4f4}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:10px}
.msg{padding:10px;border-radius:5px;margin:10px 0}.msg.ok{background:#042;color:#0f0}.msg.err{background:#400;color:#f44}
</style></head><body>
<h1>Irrigation Controller</h1>
<div class="card">
<div class="row"><span class="label">Time:</span><span id="time">--:--</span>
<span class="label">Temp:</span><span id="temp">--</span>&deg;<span id="tunit">F</span>
<span class="label">Humidity:</span><span id="rh">--</span>%</div></div>

<div class="tabs">
<button class="tab active" onclick="showTab('zones')">Zones</button>
<button class="tab" onclick="showTab('config')">Zone Config</button>
<button class="tab" onclick="showTab('system')">System</button>
</div>

<div id="zones" class="panel active"><div class="card">
<h2>Zone Status</h2>
<div id="zonelist">Loading...</div>
</div></div>

<div id="config" class="panel"><div class="card">
<h2>Zone Configuration</h2>
<div class="row"><span class="label">Select Zone:</span>
<select id="zsel" onchange="loadZone()"></select></div>
<div id="zconfig"></div>
</div></div>

<div id="system" class="panel"><div class="card">
<h2>System Settings</h2>
<div class="row"><span class="label">Active Zones:</span>
<input type="number" id="numzones" min="1" max="16"></div>
<div class="row"><span class="label">Temp Display:</span>
<select id="tempunit"><option value="1">Fahrenheit</option><option value="0">Celsius</option></select></div>
<div class="row"><span class="label">Relay Type:</span>
<select id="relaytype"><option value="0">NC (Normally Closed)</option><option value="1">NO (Normally Open)</option></select></div>
<div class="row"><span class="label">Freeze Protect:</span>
<input type="checkbox" id="freezeprot"><span class="label">Below</span>
<input type="number" id="freezetemp" style="width:50px">&deg;</div>
<div class="row"><span class="label">Temp Switch:</span>
<input type="number" id="tempswitch" style="width:50px">&deg;
<span class="label">Low Adj:</span><input type="number" id="adjlow" style="width:50px">%
<span class="label">Hi Adj:</span><input type="number" id="adjhi" style="width:50px">%</div>
<div class="row"><span class="label">NTP Sync:</span><input type="checkbox" id="ntp">
<span class="label">Timezone:</span><input type="number" id="tz" min="-12" max="12" style="width:50px"></div>
<div class="row">
<button onclick="saveSys()">Save System Config</button>
<button class="danger" onclick="doWinterize()">Winterize All Zones</button>
</div>
<div id="sysmsg"></div>
</div></div>

<script>)rawhtml";

const char HTML_SCRIPT[] PROGMEM = R"rawhtml(
var zoneData=[];
function G(id){return document.getElementById(id);}
function showTab(t){
document.querySelectorAll('.tab').forEach(function(b){b.classList.remove('active');});
document.querySelectorAll('.panel').forEach(function(p){p.classList.remove('active');});
event.target.classList.add('active');G(t).classList.add('active');
if(t=='config')loadZone();}

function load(){
fetch('/api/status').then(function(r){return r.json();}).then(function(s){
G('time').textContent=s.time;G('temp').textContent=s.temp.toFixed(1);
G('rh').textContent=s.rh.toFixed(1);G('tunit').textContent=s.unit;
});
fetch('/api/zones').then(function(r){return r.json();}).then(function(z){
zoneData=z;var h='';var sel='';
for(var i=0;i<z.length;i++){
var cls=z[i].active?'on':'off';
h+='<div class="zone"><div><b>'+z[i].name+'</b> <span class="'+cls+'">'+z[i].moisture+'%</span>';
h+=' <small>['+z[i].low+'-'+z[i].high+'%]</small></div>';
h+='<div><button class="'+(z[i].active?'':'off')+'" onclick="toggle('+i+')">'+(z[i].active?'ON':'OFF')+'</button></div></div>';
sel+='<option value="'+i+'">'+z[i].name+'</option>';
}
G('zonelist').innerHTML=h;
G('zsel').innerHTML=sel;
});}

function toggle(z){
fetch('/api/control',{method:'POST',headers:{'Content-Type':'application/json'},
body:JSON.stringify({zone:z,action:'toggle'})}).then(function(){load();});}

function loadZone(){
var i=parseInt(G('zsel').value);if(isNaN(i))return;
var z=zoneData[i];if(!z)return;
var h='<div class="row"><span class="label">Name:</span><input type="text" id="zname" value="'+z.name+'" maxlength="9"></div>';
h+='<div class="row"><span class="label">Enabled:</span><input type="checkbox" id="zen" '+(z.enabled?'checked':'')+'></div>';
h+='<div class="row"><span class="label">Mode:</span><select id="zmode">';
h+='<option value="0" '+(z.mode==0?'selected':'')+'>Moisture</option>';
h+='<option value="1" '+(z.mode==1?'selected':'')+'>Time</option>';
h+='<option value="2" '+(z.mode==2?'selected':'')+'>Hybrid</option>';
h+='<option value="3" '+(z.mode==3?'selected':'')+'>Manual</option></select></div>';
h+='<div class="row"><span class="label">Low Thresh:</span><input type="number" id="zlow" value="'+z.low+'" min="0" max="100">%</div>';
h+='<div class="row"><span class="label">High Thresh:</span><input type="number" id="zhi" value="'+z.high+'" min="0" max="100">%</div>';
h+='<div class="row"><span class="label">Dry Cal:</span><input type="number" id="zdry" value="'+z.dryCal+'">';
h+='<button onclick="calDry('+i+')">Set Now</button></div>';
h+='<div class="row"><span class="label">Wet Cal:</span><input type="number" id="zwet" value="'+z.wetCal+'">';
h+='<button onclick="calWet('+i+')">Set Now</button></div>';
h+='<div class="row"><span class="label">Raw ADC:</span><span id="rawadc">--</span>';
h+='<button onclick="readRaw('+i+')">Read</button></div>';
h+='<div class="row"><button onclick="saveZone('+i+')">Save Zone '+(i+1)+'</button></div>';
h+='<div id="zmsg"></div>';
G('zconfig').innerHTML=h;}

function readRaw(z){fetch('/api/raw?z='+z).then(function(r){return r.json();}).then(function(d){G('rawadc').textContent=d.raw;});}
function calDry(z){fetch('/api/raw?z='+z).then(function(r){return r.json();}).then(function(d){G('zdry').value=d.raw;});}
function calWet(z){fetch('/api/raw?z='+z).then(function(r){return r.json();}).then(function(d){G('zwet').value=d.raw;});}

function saveZone(i){
var d={id:i,name:G('zname').value,enabled:G('zen').checked?1:0,mode:parseInt(G('zmode').value),
low:parseInt(G('zlow').value),high:parseInt(G('zhi').value),
dryCal:parseInt(G('zdry').value),wetCal:parseInt(G('zwet').value)};
fetch('/api/zone',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(d)})
.then(function(r){G('zmsg').innerHTML=r.ok?'<div class="msg ok">Saved!</div>':'<div class="msg err">Error!</div>';
setTimeout(function(){G('zmsg').innerHTML='';},3000);load();});}

function loadSys(){
fetch('/api/system').then(function(r){return r.json();}).then(function(s){
G('numzones').value=s.zones;G('tempunit').value=s.tempF;G('relaytype').value=s.relay;
G('freezeprot').checked=s.freeze;G('freezetemp').value=s.freezeT;
G('tempswitch').value=s.tempSw;G('adjlow').value=s.adjL;G('adjhi').value=s.adjH;
G('ntp').checked=s.ntp;G('tz').value=s.tz;});}

function saveSys(){
var d={zones:parseInt(G('numzones').value),tempF:parseInt(G('tempunit').value),
relay:parseInt(G('relaytype').value),freeze:G('freezeprot').checked?1:0,
freezeT:parseInt(G('freezetemp').value),tempSw:parseInt(G('tempswitch').value),
adjL:parseInt(G('adjlow').value),adjH:parseInt(G('adjhi').value),
ntp:G('ntp').checked?1:0,tz:parseInt(G('tz').value)};
fetch('/api/system',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(d)})
.then(function(r){G('sysmsg').innerHTML=r.ok?'<div class="msg ok">System config saved!</div>':'<div class="msg err">Error!</div>';
setTimeout(function(){G('sysmsg').innerHTML='';},3000);load();});}

function doWinterize(){
if(!confirm('Run winterize cycle on all zones?'))return;
G('sysmsg').innerHTML='<div class="msg ok">Winterizing...</div>';
fetch('/api/winterize',{method:'POST'}).then(function(){
G('sysmsg').innerHTML='<div class="msg ok">Winterize complete!</div>';
setTimeout(function(){G('sysmsg').innerHTML='';},5000);});}

load();loadSys();setInterval(load,5000);
</script></body></html>)rawhtml";

// ============================================================================
// FUNCTION PROTOTYPES
// ============================================================================

void loadConfig();
void saveConfig();
void saveZoneConfig(uint8_t z);
void setDefaults();
uint16_t calcChecksum();
void readSensors();
void updateIrrigation();
void startWatering(uint8_t z);
void stopWatering(uint8_t z);
void handleWeb();
void winterize();
void syncNTP();
time_t getNTPTime();
int freeRam();

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  Serial.begin(115200);
  Serial.println(F("\n=== Irrigation V3.1 LEAN CONFIG ==="));
  
  u8x8.begin();
  u8x8.setPowerSave(0);
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.clear();
  u8x8.print(F("Irrigation"));
  u8x8.setCursor(0, 1);
  u8x8.print(VERSION);
  
  Wire.begin();
  DHT.begin();
  
  #ifdef USE_RTC
  if (rtc.begin() && rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  #endif
  
  loadConfig();
  
  // Init relay pins
  for (uint8_t i = 0; i < sysConfig.zones; i++) {
    pinMode(i + 2, OUTPUT);
    digitalWrite(i + 2, !sysConfig.relayOn);
  }
  relayStatus = 0;
  
  // Network
  u8x8.setCursor(0, 3);
  u8x8.print(F("Network..."));
  if (Ethernet.begin(mac)) {
    Serial.print(F("IP: "));
    Serial.println(Ethernet.localIP());
    server.begin();
    if (sysConfig.ntpEnabled) syncNTP();
    u8x8.setCursor(0, 4);
    u8x8.print(Ethernet.localIP());
  } else {
    u8x8.setCursor(0, 4);
    u8x8.print(F("DHCP Failed!"));
  }
  
  readSensors();
  
  Serial.print(F("Free RAM: "));
  Serial.println(freeRam());
  
  delay(2000);
  u8x8.clear();
}

int freeRam() {
  extern int __heap_start, *__brkval;
  int v;
  return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
  unsigned long now = millis();
  
  if (now - lastSensorRead >= 5000) {
    readSensors();
    updateIrrigation();
    lastSensorRead = now;
    
    // Update OLED
    u8x8.setCursor(0, 0);
    u8x8.print(F("T:"));
    u8x8.print(sysConfig.tempDisplayF ? (int)temp : (int)((temp-32)*5/9));
    u8x8.print(sysConfig.tempDisplayF ? "F " : "C ");
    
    for (uint8_t i = 0; i < min((uint8_t)6, sysConfig.zones); i++) {
      u8x8.setCursor(0, i + 1);
      u8x8.print(i + 1);
      u8x8.print(":");
      u8x8.print(soilMoisture[i]);
      u8x8.print("% ");
      u8x8.print(isRelayOn(i) ? "ON " : "OFF");
    }
    u8x8.setCursor(0, 7);
    u8x8.print(Ethernet.localIP());
  }
  
  handleWeb();
  
  // NTP sync hourly
  if (sysConfig.ntpEnabled && now - lastNTPSync >= 3600000UL) {
    syncNTP();
    lastNTPSync = now;
  }
  
  delay(10);
}

// ============================================================================
// WEB SERVER
// ============================================================================

void handleWeb() {
  EthernetClient client = server.available();
  if (!client) return;
  
  char req[80] = {0};
  char body[200] = {0};
  uint8_t reqIdx = 0, bodyIdx = 0;
  bool inBody = false, lineBlank = false;
  unsigned long timeout = millis() + 2000;
  
  while (client.connected() && millis() < timeout) {
    if (client.available()) {
      char c = client.read();
      if (inBody) {
        if (bodyIdx < 199) body[bodyIdx++] = c;
      } else {
        if (c == '\n' && lineBlank) inBody = true;
        else if (reqIdx < 79 && c != '\r' && c != '\n') req[reqIdx++] = c;
        lineBlank = (c == '\n');
      }
    } else if (inBody) break;
  }
  
  // Parse request
  char* method = req;
  char* path = strchr(req, ' ');
  if (path) { *path++ = 0; char* end = strchr(path, ' '); if (end) *end = 0; }
  else path = (char*)"/";
  
  bool isGet = (strcmp(method, "GET") == 0);
  bool isPost = (strcmp(method, "POST") == 0);
  
  // Route
  if (isGet && strcmp(path, "/") == 0) {
    // Serve HTML from PROGMEM
    client.println(F("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n"));
    const char* p = HTML_HEAD;
    char c; while ((c = pgm_read_byte(p++))) client.write(c);
    p = HTML_SCRIPT;
    while ((c = pgm_read_byte(p++))) client.write(c);
  }
  else if (isGet && strcmp(path, "/api/status") == 0) {
    client.println(F("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\n"));
    client.print(F("{\"temp\":")); client.print(temp);
    client.print(F(",\"rh\":")); client.print(humidity);
    client.print(F(",\"unit\":\"")); client.print(sysConfig.tempDisplayF ? "F" : "C");
    client.print(F("\",\"time\":\""));
    client.print(hour()); client.print(':');
    if (minute() < 10) client.print('0'); client.print(minute());
    client.println(F("\"}"));
  }
  else if (isGet && strcmp(path, "/api/zones") == 0) {
    client.println(F("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\n"));
    client.print('[');
    for (uint8_t i = 0; i < sysConfig.zones; i++) {
      if (i) client.print(',');
      client.print(F("{\"name\":\""));  client.print(zones[i].name);
      client.print(F("\",\"enabled\":")); client.print(zones[i].enabled);
      client.print(F(",\"active\":")); client.print(isRelayOn(i) ? "true" : "false");
      client.print(F(",\"moisture\":")); client.print(soilMoisture[i]);
      client.print(F(",\"mode\":")); client.print(zones[i].mode);
      client.print(F(",\"low\":")); client.print(zones[i].moistureLow);
      client.print(F(",\"high\":")); client.print(zones[i].moistureHigh);
      client.print(F(",\"dryCal\":")); client.print(zones[i].dryCal);
      client.print(F(",\"wetCal\":")); client.print(zones[i].wetCal);
      client.print('}');
    }
    client.println(']');
  }
  else if (isGet && strncmp(path, "/api/raw", 8) == 0) {
    // Get raw ADC for zone: /api/raw?z=0
    int z = 0;
    char* zp = strstr(path, "z=");
    if (zp) z = atoi(zp + 2);
    int raw = analogRead(pgm_read_word(&analogPins[z]));
    client.println(F("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\n"));
    client.print(F("{\"raw\":")); client.print(raw); client.println('}');
  }
  else if (isGet && strcmp(path, "/api/system") == 0) {
    client.println(F("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\n"));
    client.print(F("{\"zones\":")); client.print(sysConfig.zones);
    client.print(F(",\"tempF\":")); client.print(sysConfig.tempDisplayF);
    client.print(F(",\"relay\":")); client.print(sysConfig.relayOn);
    client.print(F(",\"freeze\":")); client.print(sysConfig.freezeProtect);
    client.print(F(",\"freezeT\":")); client.print(sysConfig.freezeTemp);
    client.print(F(",\"tempSw\":")); client.print(sysConfig.tempSwitch);
    client.print(F(",\"adjL\":")); client.print(sysConfig.tempAdjLow);
    client.print(F(",\"adjH\":")); client.print(sysConfig.tempAdjHi);
    client.print(F(",\"ntp\":")); client.print(sysConfig.ntpEnabled);
    client.print(F(",\"tz\":")); client.print(sysConfig.timeZoneOffset);
    client.println('}');
  }
  else if (isPost && strcmp(path, "/api/control") == 0) {
    StaticJsonDocument<64> doc;
    if (deserializeJson(doc, body) == DeserializationError::Ok) {
      uint8_t z = doc["zone"];
      if (z < sysConfig.zones) {
        if (isRelayOn(z)) stopWatering(z);
        else startWatering(z);
      }
    }
    client.println(F("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n{\"ok\":true}"));
  }
  else if (isPost && strcmp(path, "/api/zone") == 0) {
    StaticJsonDocument<128> doc;
    if (deserializeJson(doc, body) == DeserializationError::Ok) {
      uint8_t z = doc["id"];
      if (z < 16) {
        if (doc.containsKey("name")) strncpy(zones[z].name, doc["name"] | "", 9);
        if (doc.containsKey("enabled")) zones[z].enabled = doc["enabled"];
        if (doc.containsKey("mode")) zones[z].mode = doc["mode"];
        if (doc.containsKey("low")) zones[z].moistureLow = doc["low"];
        if (doc.containsKey("high")) zones[z].moistureHigh = doc["high"];
        if (doc.containsKey("dryCal")) zones[z].dryCal = doc["dryCal"];
        if (doc.containsKey("wetCal")) zones[z].wetCal = doc["wetCal"];
        saveZoneConfig(z);
      }
    }
    client.println(F("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n{\"ok\":true}"));
  }
  else if (isPost && strcmp(path, "/api/system") == 0) {
    StaticJsonDocument<128> doc;
    if (deserializeJson(doc, body) == DeserializationError::Ok) {
      if (doc.containsKey("zones")) sysConfig.zones = doc["zones"];
      if (doc.containsKey("tempF")) sysConfig.tempDisplayF = doc["tempF"];
      if (doc.containsKey("relay")) sysConfig.relayOn = doc["relay"];
      if (doc.containsKey("freeze")) sysConfig.freezeProtect = doc["freeze"];
      if (doc.containsKey("freezeT")) sysConfig.freezeTemp = doc["freezeT"];
      if (doc.containsKey("tempSw")) sysConfig.tempSwitch = doc["tempSw"];
      if (doc.containsKey("adjL")) sysConfig.tempAdjLow = doc["adjL"];
      if (doc.containsKey("adjH")) sysConfig.tempAdjHi = doc["adjH"];
      if (doc.containsKey("ntp")) sysConfig.ntpEnabled = doc["ntp"];
      if (doc.containsKey("tz")) sysConfig.timeZoneOffset = doc["tz"];
      saveConfig();
    }
    client.println(F("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n{\"ok\":true}"));
  }
  else if (isPost && strcmp(path, "/api/winterize") == 0) {
    winterize();
    client.println(F("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n{\"ok\":true}"));
  }
  else {
    client.println(F("HTTP/1.1 404 Not Found\r\nConnection: close\r\n"));
  }
  
  delay(1);
  client.stop();
}

// ============================================================================
// IRRIGATION
// ============================================================================

void updateIrrigation() {
  // Freeze protection
  if (sysConfig.freezeProtect && temp < sysConfig.freezeTemp) {
    for (uint8_t i = 0; i < sysConfig.zones; i++) {
      if (isRelayOn(i)) stopWatering(i);
    }
    return;
  }
  
  bool isHot = (temp >= sysConfig.tempSwitch);
  
  for (uint8_t i = 0; i < sysConfig.zones; i++) {
    if (!zones[i].enabled || zones[i].mode == MODE_MANUAL) continue;
    
    int effLow = zones[i].moistureLow + (isHot ? sysConfig.tempAdjLow : 0);
    int effHigh = zones[i].moistureHigh + (isHot ? sysConfig.tempAdjHi : 0);
    
    if (soilMoisture[i] <= effLow && !isRelayOn(i)) startWatering(i);
    if (soilMoisture[i] >= effHigh && isRelayOn(i)) stopWatering(i);
  }
}

void startWatering(uint8_t z) {
  digitalWrite(z + 2, sysConfig.relayOn);
  setRelay(z, true);
  Serial.print(F("Zone ")); Serial.print(z+1); Serial.println(F(" ON"));
}

void stopWatering(uint8_t z) {
  digitalWrite(z + 2, !sysConfig.relayOn);
  setRelay(z, false);
  Serial.print(F("Zone ")); Serial.print(z+1); Serial.println(F(" OFF"));
}

// ============================================================================
// SENSORS
// ============================================================================

void readSensors() {
  if (DHT.read() == AM2315C_OK) {
    temp = DHT.getTemperature() * 9.0 / 5.0 + 32.0;
    humidity = DHT.getHumidity();
  }
  
  for (uint8_t i = 0; i < sysConfig.zones; i++) {
    int pin = pgm_read_word(&analogPins[i]);
    long raw = analogRead(pin);
    if (raw < zones[i].dryCal) raw = zones[i].dryCal;
    if (raw > zones[i].wetCal) raw = zones[i].wetCal;
    soilMoisture[i] = map(raw, zones[i].dryCal, zones[i].wetCal, 0, 100);
  }
}

// ============================================================================
// CONFIG
// ============================================================================

void loadConfig() {
  EEPROM.get(EEPROM_SYS_ADDR, sysConfig);
  uint16_t stored = sysConfig.checksum;
  sysConfig.checksum = 0;
  
  if (calcChecksum() != stored || sysConfig.zones == 0 || sysConfig.zones > 16) {
    Serial.println(F("Loading defaults"));
    setDefaults();
    saveConfig();
  } else {
    for (uint8_t i = 0; i < 16; i++) {
      EEPROM.get(EEPROM_ZONE_ADDR + i * sizeof(ZoneConfig), zones[i]);
    }
  }
}

void saveConfig() {
  sysConfig.checksum = 0;
  sysConfig.checksum = calcChecksum();
  EEPROM.put(EEPROM_SYS_ADDR, sysConfig);
  for (uint8_t i = 0; i < sysConfig.zones; i++) {
    EEPROM.put(EEPROM_ZONE_ADDR + i * sizeof(ZoneConfig), zones[i]);
  }
  Serial.println(F("Config saved"));
}

void saveZoneConfig(uint8_t z) {
  EEPROM.put(EEPROM_ZONE_ADDR + z * sizeof(ZoneConfig), zones[z]);
  Serial.print(F("Zone ")); Serial.print(z+1); Serial.println(F(" saved"));
}

void setDefaults() {
  sysConfig.zones = 6;
  sysConfig.tempDisplayF = 1;
  sysConfig.relayOn = 0;
  sysConfig.ntpEnabled = 1;
  sysConfig.freezeProtect = 1;
  sysConfig.timeZoneOffset = -5;
  sysConfig.freezeTemp = 35;
  sysConfig.tempSwitch = 92;
  sysConfig.tempAdjLow = 6;
  sysConfig.tempAdjHi = 4;
  
  for (uint8_t i = 0; i < 16; i++) {
    snprintf(zones[i].name, 10, "Zone %d", i + 1);
    zones[i].dryCal = 125;
    zones[i].wetCal = 550;
    zones[i].moistureLow = 40;
    zones[i].moistureHigh = 70;
    zones[i].mode = MODE_MOISTURE;
    zones[i].enabled = (i < 6);
    zones[i].maxDuration = 60;
  }
}

uint16_t calcChecksum() {
  uint16_t sum = 0;
  uint8_t* p = (uint8_t*)&sysConfig;
  for (size_t i = 0; i < sizeof(SystemConfig); i++) sum += p[i];
  return sum ^ 0x5A5A;
}

// ============================================================================
// UTILITIES
// ============================================================================

void winterize() {
  Serial.println(F("Winterizing..."));
  for (uint8_t i = 0; i < sysConfig.zones; i++) {
    if (!zones[i].enabled) continue;
    digitalWrite(i + 2, sysConfig.relayOn);
    delay(5000);
    digitalWrite(i + 2, !sysConfig.relayOn);
    delay(1000);
  }
  Serial.println(F("Done"));
}

void syncNTP() {
  Serial.println(F("NTP sync..."));
  time_t t = getNTPTime();
  if (t > 0) {
    setTime(t + sysConfig.timeZoneOffset * 3600L);
    Serial.println(F("Time set"));
  }
}

time_t getNTPTime() {
  byte buf[48] = {0};
  buf[0] = 0b11100011; buf[2] = 6; buf[3] = 0xEC;
  udp.begin(123);
  udp.beginPacket("pool.ntp.org", 123);
  udp.write(buf, 48);
  udp.endPacket();
  delay(1000);
  if (udp.parsePacket()) {
    udp.read(buf, 48);
    unsigned long secs = ((unsigned long)buf[40]<<24)|((unsigned long)buf[41]<<16)|
                         ((unsigned long)buf[42]<<8)|buf[43];
    udp.stop();
    return secs - 2208988800UL;
  }
  udp.stop();
  return 0;
}
