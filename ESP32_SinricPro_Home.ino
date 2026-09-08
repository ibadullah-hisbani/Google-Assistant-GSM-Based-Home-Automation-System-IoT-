/*
  ============================================================================
  Google Assistant and GSM-Based Smart Home Automation System
  ESP32 FIRMWARE — Voice Control Path (via SinricPro + Google Home)
  ============================================================================
  Function:
    - Connects ESP32 to your Wi-Fi network
    - Connects to the SinricPro cloud using the SinricPro library
    - Registers 4 SinricPro Switch devices (Fan, Socket, Bulb1, Bulb2)
    - Google Home / Google Assistant voice commands -> SinricPro -> ESP32
      -> relay ON/OFF
    - Reports back device state to SinricPro so Google Home app stays in sync

  Required Arduino Library (install via Library Manager):
    - "SinricPro" by sinricpro  (https://github.com/sinricpro/esp8266-esp32-sdk)

  Wiring (adjust pin numbers to match your actual wiring guide):
    Relay IN1 (Fan)    -> ESP32 GPIO 25
    Relay IN2 (Socket) -> ESP32 GPIO 26
    Relay IN3 (Bulb 1) -> ESP32 GPIO 27
    Relay IN4 (Bulb 2) -> ESP32 GPIO 14
    Relay VCC -> 5V, Relay GND -> Common GND

    Voltage divider on ESP32 RX pin (GPIO 16 if using UART2, or as per your
    design) — protects ESP32 from any 5V logic signal shared with Arduino
    Uno / SIM800L side of the board (only needed if ESP32 shares a serial
    line with 5V logic; otherwise this pin is unused in the voice-only path).

  SinricPro setup (do this once on sinric.pro before flashing):
    1. Create a free account at https://portal.sinric.pro
    2. Create 4 "Switch" devices: Fan, Socket, Bulb1, Bulb2
    3. Copy each Device ID below
    4. Copy your APP_KEY and APP_SECRET from the SinricPro portal
    5. Link SinricPro to Google Home from the SinricPro portal
       (Account -> Connect to Google Home), then say "Hey Google, sync my devices"
  ============================================================================
*/

#include <WiFi.h>
#include "SinricPro.h"
#include "SinricProSwitch.h"

// ---------------- WiFi Credentials ----------------
#define WIFI_SSID     "Wifi"
#define WIFI_PASS     "78655465"

// ---------------- SinricPro Credentials ----------------
#define APP_KEY       "a7c3e921-5d48-4b16-8f27-63e91c5a2d74"
#define APP_SECRET    "8Fq2Lm7Xv9Kc4Rz6Wp1Hs5Tn3Ba0YdEe"

// ---------------- SinricPro Device IDs (copied from your Devices list) ----------------
// IMPORTANT: these were read from a phone photo of your screen and may contain
// small errors. Please double check each ID by clicking the copy icon (the
// small overlapping-squares icon next to each device on portal.sinric.pro)
// and pasting it here directly, to avoid a typo breaking the connection.
#define FAN_ID        "6a6b3cf86ba33a88b99ccb1d"   // Room Fan
#define BULB1_ID      "6a6b3c68969af7ec24771646"   // Room Lamp
#define BULB2_ID      "6a6b3c2729c6be33428339e4"   // Room Light

// NOTE: Only 3 devices (Fan, Lamp, Light) exist in your SinricPro account —
// there is no "Socket" device yet. Add a 4th "Switch" device on the portal
// (e.g. "Room Socket") if you want the plug socket voice-controlled too,
// then uncomment the line below and set its ID.
// #define SOCKET_ID  "YOUR_SOCKET_DEVICE_ID"

// ---------------- Relay Pins ----------------
#define RELAY_FAN     25
#define RELAY_BULB1   27
#define RELAY_BULB2   14
// #define RELAY_SOCKET  26   // uncomment once you add the Socket device above

// Most 5V relay boards are ACTIVE LOW: LOW = relay ON, HIGH = relay OFF
#define RELAY_ON   LOW
#define RELAY_OFF  HIGH

#define BAUD_RATE  115200

// =====================================================================
// SinricPro callback: called whenever Google Home / Google Assistant
// sends an ON/OFF command for a given device
// =====================================================================
bool onPowerState(const String &deviceId, bool &state) {
  int relayPin = -1;

  if (deviceId == FAN_ID)         relayPin = RELAY_FAN;
  else if (deviceId == BULB1_ID)  relayPin = RELAY_BULB1;
  else if (deviceId == BULB2_ID)  relayPin = RELAY_BULB2;
  // else if (deviceId == SOCKET_ID) relayPin = RELAY_SOCKET; // enable once Socket device is added

  if (relayPin == -1) return false; // unknown device id

  digitalWrite(relayPin, state ? RELAY_ON : RELAY_OFF);

  Serial.printf("[SinricPro] Device %s set to %s\n",
                deviceId.c_str(), state ? "ON" : "OFF");

  return true; // report success back to SinricPro / Google Home
}

// =====================================================================
// Wi-Fi setup
// =====================================================================
void setupWiFi() {
  Serial.printf("\n[WiFi]: Connecting to %s ...", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }
  Serial.printf("\n[WiFi]: Connected, IP address: %s\n", WiFi.localIP().toString().c_str());
}

// =====================================================================
// Register all 4 switch devices with SinricPro and set up callbacks
// =====================================================================
void setupSinricPro() {
  SinricProSwitch &fan    = SinricPro[FAN_ID];
  SinricProSwitch &bulb1  = SinricPro[BULB1_ID];
  SinricProSwitch &bulb2  = SinricPro[BULB2_ID];
  // SinricProSwitch &socket = SinricPro[SOCKET_ID]; // enable once Socket device is added

  fan.onPowerState(onPowerState);
  bulb1.onPowerState(onPowerState);
  bulb2.onPowerState(onPowerState);
  // socket.onPowerState(onPowerState);

  SinricPro.onConnected([]() { Serial.println("[SinricPro]: Connected to cloud."); });
  SinricPro.onDisconnected([]() { Serial.println("[SinricPro]: Disconnected from cloud."); });

  SinricPro.begin(APP_KEY, APP_SECRET);
}

// =====================================================================
// Relay pin initialization — all appliances OFF at startup
// =====================================================================
void setupRelays() {
  pinMode(RELAY_FAN, OUTPUT);
  pinMode(RELAY_BULB1, OUTPUT);
  pinMode(RELAY_BULB2, OUTPUT);
  // pinMode(RELAY_SOCKET, OUTPUT); // uncomment once Socket device/pin is added

  digitalWrite(RELAY_FAN, RELAY_OFF);
  digitalWrite(RELAY_BULB1, RELAY_OFF);
  digitalWrite(RELAY_BULB2, RELAY_OFF);
  // digitalWrite(RELAY_SOCKET, RELAY_OFF);
}

void setup() {
  Serial.begin(BAUD_RATE);
  setupRelays();
  setupWiFi();
  setupSinricPro();
  Serial.println("System Ready. Waiting for Google Home / Google Assistant commands...");
}

void loop() {
  SinricPro.handle();
}
