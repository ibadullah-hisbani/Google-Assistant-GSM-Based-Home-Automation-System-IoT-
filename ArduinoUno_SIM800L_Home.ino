/*
  ============================================================================
  Google Assistant and GSM-Based Smart Home Automation System
  ARDUINO UNO FIRMWARE — MASTER RELAY CONTROLLER
  ============================================================================
  Role:
    The Arduino Uno is the ONLY controller that directly drives the shared
    4-channel relay module. This avoids two microcontrollers (Arduino + ESP32)
    trying to drive the same relay input pins at the same time.

    It receives ON/OFF commands from TWO independent sources and acts on
    whichever arrives:
      (1) SIM800L GSM module  -> SMS commands (works with NO WiFi/Internet)
      (2) ESP32 module        -> forwarded voice commands from Google Home,
                                   received via SinricPro (works WHEN WiFi/
                                   Internet is available)

    Both paths converge here, so appliance control keeps working whether or
    not the WiFi/Internet connection is up.

  Wiring (adjust pin numbers to match your actual wiring guide):
    SIM800L TX  -> Arduino D2   (SoftwareSerial RX, for SMS)
    SIM800L RX  -> Arduino D3   (SoftwareSerial TX, for SMS)
    SIM800L VCC -> LM2596 buck converter output (~4V), common GND with Arduino

    ESP32 TX    -> Arduino D12  (SoftwareSerial RX, for ESP32->Arduino commands)
    ESP32 RX    <- Arduino D13  (SoftwareSerial TX) -- ONLY if Arduino needs to
                    talk back to ESP32; use a voltage divider on this line
                    since Arduino is 5V logic and ESP32 RX is 3.3V-only.

    Relay IN1 (Fan)     -> Arduino D4
    Relay IN2 (Socket)  -> Arduino D5
    Relay IN3 (Bulb 1)  -> Arduino D6
    Relay IN4 (Bulb 2)  -> Arduino D7
    Relay VCC -> 5V, Relay GND -> Common GND

  NOTE: If your relay module is ACTIVE LOW (most common 5V relay boards are),
  RELAY_ON / RELAY_OFF below are already set for that. Change if yours differs.
  ============================================================================
*/

#include <SoftwareSerial.h>

// ---------------- SIM800L Serial (SMS path) ----------------
SoftwareSerial sim800(2, 3); // RX, TX  (SIM800L TX -> D2, SIM800L RX -> D3)

// ---------------- ESP32 Serial (Voice/SinricPro path) ----------------
SoftwareSerial esp32Link(12, 13); // RX, TX (ESP32 TX -> D12, ESP32 RX <- D13 via voltage divider)

// ---------------- Relay Pins ----------------
#define RELAY_FAN     4
#define RELAY_SOCKET  5
#define RELAY_BULB1   6
#define RELAY_BULB2   7

// Most 5V relay boards are ACTIVE LOW: LOW = relay ON, HIGH = relay OFF
#define RELAY_ON   LOW
#define RELAY_OFF  HIGH

// ---------------- Authorized phone number (optional security) ----------------
// Leave blank ("") to accept SMS commands from ANY sender, or set your
// number in international format e.g. "+923001234567" to restrict access.
String AUTHORIZED_NUMBER = "";

// ---------------- State tracking ----------------
bool fanState = false, socketState = false, bulb1State = false, bulb2State = false;

String smsBuffer = "";
String senderNumber = "";
String espBuffer = "";

void setup() {
  Serial.begin(9600);
  sim800.begin(9600);
  esp32Link.begin(9600);

  pinMode(RELAY_FAN, OUTPUT);
  pinMode(RELAY_SOCKET, OUTPUT);
  pinMode(RELAY_BULB1, OUTPUT);
  pinMode(RELAY_BULB2, OUTPUT);

  digitalWrite(RELAY_FAN, RELAY_OFF);
  digitalWrite(RELAY_SOCKET, RELAY_OFF);
  digitalWrite(RELAY_BULB1, RELAY_OFF);
  digitalWrite(RELAY_BULB2, RELAY_OFF);

  Serial.println(F("Initializing SIM800L..."));
  delay(3000);

  sendAT("AT", 1000);                // basic handshake
  sendAT("AT+CMGF=1", 1000);         // set SMS to text mode
  sendAT("AT+CNMI=1,2,0,0,0", 1000); // forward new SMS directly to serial

  Serial.println(F("System Ready. Master controller listening on: SMS (SIM800L) and ESP32 link."));
}

void loop() {
  readIncomingSMS();       // Offline path — works with no WiFi/Internet
  readIncomingFromESP32(); // Online path — voice commands forwarded by ESP32
}

// =====================================================================
// Send AT command and print response (used during setup)
// =====================================================================
void sendAT(String cmd, int waitMs) {
  sim800.println(cmd);
  delay(waitMs);
  while (sim800.available()) {
    Serial.write(sim800.read());
  }
}

// =====================================================================
// OFFLINE PATH: Continuously read SIM800L serial for a new incoming SMS
// =====================================================================
void readIncomingSMS() {
  while (sim800.available()) {
    String line = sim800.readStringUntil('\n');
    line.trim();

    // Example unsolicited notification:
    // +CMT: "+923001234567","","26/08/14,10:15:22+20"
    // fanon
    if (line.startsWith("+CMT:")) {
      int firstQuote = line.indexOf('"');
      int secondQuote = line.indexOf('"', firstQuote + 1);
      if (firstQuote != -1 && secondQuote != -1) {
        senderNumber = line.substring(firstQuote + 1, secondQuote);
      }
      smsBuffer = sim800.readStringUntil('\n');
      smsBuffer.trim();
      processCommand(smsBuffer, senderNumber, true); // true = reply via SMS
    }
  }
}

// =====================================================================
// ONLINE PATH: Continuously read commands forwarded by the ESP32
// (ESP32 sends a simple one-word command string terminated by '\n',
//  e.g. "fanon", "fanoff", "socketon" ... whenever a SinricPro/Google
//  Home command is received on the ESP32 side)
// =====================================================================
void readIncomingFromESP32() {
  while (esp32Link.available()) {
    char c = esp32Link.read();
    if (c == '\n') {
      espBuffer.trim();
      if (espBuffer.length() > 0) {
        processCommand(espBuffer, "", false); // false = no SMS reply needed
      }
      espBuffer = "";
    } else {
      espBuffer += c;
    }
  }
}

// =====================================================================
// Parse and execute a command, regardless of which path it came from
// =====================================================================
void processCommand(String msg, String from, bool replyBySMS) {
  msg.toLowerCase();
  msg.trim();
  msg.replace(" ", ""); // remove spaces so "fan on" and "fanon" both work

  Serial.print(F("Command received: "));
  Serial.println(msg);

  // Optional sender authorization check (SMS path only)
  if (replyBySMS && AUTHORIZED_NUMBER.length() > 0 && from.indexOf(AUTHORIZED_NUMBER) == -1) {
    Serial.println(F("Unauthorized sender. Ignoring command."));
    return;
  }

  String reply = "";

  if (msg == "fanon")        { setRelay(RELAY_FAN, true);  fanState = true;  reply = "Ok Fan is On"; }
  else if (msg == "fanoff")  { setRelay(RELAY_FAN, false); fanState = false; reply = "Ok Fan is Off"; }

  else if (msg == "socketon")  { setRelay(RELAY_SOCKET, true);  socketState = true;  reply = "Ok Socket is On"; }
  else if (msg == "socketoff") { setRelay(RELAY_SOCKET, false); socketState = false; reply = "Ok Socket is Off"; }

  else if (msg == "lampon")  { setRelay(RELAY_BULB1, true);  bulb1State = true;  reply = "Ok Lamp is On"; }
  else if (msg == "lampoff") { setRelay(RELAY_BULB1, false); bulb1State = false; reply = "Ok Lamp is Off"; }

  else if (msg == "lighton")  { setRelay(RELAY_BULB2, true);  bulb2State = true;  reply = "Ok Light is On"; }
  else if (msg == "lightoff") { setRelay(RELAY_BULB2, false); bulb2State = false; reply = "Ok Light is Off"; }

  else if (msg == "allon")  { allRelays(true);  reply = "Ok All Load is On"; }
  else if (msg == "alloff") { allRelays(false); reply = "Ok All Load is Off"; }

  else if (msg == "status") {
    reply = "Fan:" + String(fanState ? "On" : "Off") +
            " Socket:" + String(socketState ? "On" : "Off") +
            " Lamp:" + String(bulb1State ? "On" : "Off") +
            " Light:" + String(bulb2State ? "On" : "Off");
  }
  else {
    reply = "Unknown command. Try: fanon/fanoff, socketon/socketoff, lampon/lampoff, lighton/lightoff, allon/alloff, status";
  }

  // Only send an SMS confirmation if the command came in via SMS
  if (replyBySMS) {
    sendSMS(from, reply);
  } else {
    Serial.print(F("(from ESP32/voice) -> "));
    Serial.println(reply);
  }
}

// =====================================================================
// Helper: set a single relay ON/OFF
// =====================================================================
void setRelay(int pin, bool on) {
  digitalWrite(pin, on ? RELAY_ON : RELAY_OFF);
}

void allRelays(bool on) {
  setRelay(RELAY_FAN, on);     fanState = on;
  setRelay(RELAY_SOCKET, on);  socketState = on;
  setRelay(RELAY_BULB1, on);   bulb1State = on;
  setRelay(RELAY_BULB2, on);   bulb2State = on;
}

// =====================================================================
// Send confirmation SMS back to sender (SMS path only)
// =====================================================================
void sendSMS(String number, String text) {
  if (number.length() == 0) return; // no sender info available, skip
  sim800.print("AT+CMGS=\"");
  sim800.print(number);
  sim800.println("\"");
  delay(500);
  sim800.print(text);
  delay(500);
  sim800.write(26); // Ctrl+Z to send
  delay(3000);
}
