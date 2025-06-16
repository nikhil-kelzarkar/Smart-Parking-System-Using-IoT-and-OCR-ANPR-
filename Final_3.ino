#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>

// === Wi-Fi credentials ===
const char* ssid = "S23";
const char* password = "mehta0201";

// === WebServer ===
WebServer server(80);

// === Servo ===
#define SERVO_PIN 33
Servo gateServo;

// === Parking Slot LED Pins ===
#define SLOT_1_LED 27
#define SLOT_2_LED 14
#define SLOT_3_LED 12
#define SLOT_4_LED 13
int leds[] = {SLOT_1_LED, SLOT_2_LED, SLOT_3_LED, SLOT_4_LED};

// === Multiplexer Control Pins ===
#define MUX_S0 4
#define MUX_S1 2
#define MUX_S2 5
#define MUX_SIG 36  // PIR Signal from MUX to ADC pin

// === Ultrasonic Sensor Pins (Updated) ===
#define TRIG_PIN 25
#define ECHO_PIN 26
long duration;
float distance;

// === Functions ===
void selectMuxChannel(int channel) {
  digitalWrite(MUX_S0, channel & 0x01);
  digitalWrite(MUX_S1, (channel >> 1) & 0x01);
  digitalWrite(MUX_S2, (channel >> 2) & 0x01);
}

float getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  duration = pulseIn(ECHO_PIN, HIGH, 30000); // 30ms timeout
  distance = (duration * 0.0343) / 2.0;
  return distance;
}

String getParkingStatusHTML() {
  String html = "<!DOCTYPE html><html><head><meta http-equiv='refresh' content='2'>";
  html += "<style>body{font-family:Arial;} li{margin:6px 0;}</style></head><body><h2>🚗 Parking Slot Status</h2><ul>";

  for (int i = 0; i < 4; i++) {
    selectMuxChannel(i);
    delay(10);
    int pirVal = analogRead(MUX_SIG);
    bool occupied = pirVal > 1000;
    digitalWrite(leds[i], occupied ? HIGH : LOW);
    html += "<li>Slot " + String(i + 1) + ": <strong>" + (occupied ? "Occupied" : "Free") + "</strong></li>";
  }

  html += "</ul></body></html>";
  return html;
}

// === Setup ===
void setup() {
  Serial.begin(9600);

  // Servo Setup
  gateServo.attach(SERVO_PIN);
  gateServo.write(0);  // Gate closed

  // LED Setup
  for (int i = 0; i < 4; i++) pinMode(leds[i], OUTPUT);

  // MUX Pins
  pinMode(MUX_S0, OUTPUT);
  pinMode(MUX_S1, OUTPUT);
  pinMode(MUX_S2, OUTPUT);
  pinMode(MUX_SIG, INPUT);

  // Ultrasonic Pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Wi-Fi Setup
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected! IP: " + WiFi.localIP().toString());

  // WebServer Route
  server.on("/", []() {
    server.send(200, "text/html", getParkingStatusHTML());
  });
  server.begin();
}

// === Loop ===
void loop() {
  server.handleClient();

  // Check Ultrasonic Sensor
  float dist = getDistance();
  if (dist > 2 && dist < 20) {
    Serial.println("TRIGGER");
    delay(5000);  // Prevent spamming
  }

  // Check Serial for Servo Command
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd == "SERVO_OPEN") {
      gateServo.write(90);
      delay(5000);
      gateServo.write(0);
    }
  }
}
