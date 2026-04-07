//Made by Simon

#include <WiFi.h>
#include <WebServer.h>
volatile int minutes = 0;  // Zähler für Minuten

const int clock_wait = 800;
// === WiFi + Webserver ===
const char* ssid = "Uhr";
const char* password = "Whatever"; //I don't care, please just use anything different. I used something else
WebServer server(80);

const int L298N_ENA = 25;
const int L298N_IN1 = 26;
const int L298N_IN2 = 27;

String HTML = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Uhr Korrektur</title>
</head>
<body>
  <h2>Uhr-Minuten Korrektur</h2>
  <form action="/set" method="get">
    <label for="value">Minuten (positiv/negativ):</label>
    <input type="number" id="value" name="value" required>
    <input type="submit" value="Senden">
  </form>
  <h3>Häufige Werte:</h3>
  <ul>
    <li><a href="/set?value=60">1 Stunde (60 Minuten)</a></li>
    <li><a href="/set?value=660">11 Stunden (660 Minuten)</a></li>
    <li><a href="/set?value=720">12 Stunden (720 Minuten)</a></li>
  </ul>
  <h4>Build by Simon,</h4>
    <li><a href="https://github.com/Cam42exe/train_clock/">Files on GitHub</a></li>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", HTML);
}

void handleSet() {
  if (server.hasArg("value")) {
    int val = server.arg("value").toInt();
    noInterrupts();
    minutes += val;
    interrupts();
    server.send(200, "text/html", "<p>Minuten aktualisiert: " + String(val) + "</p><a href='/'>Zurück</a>");
  } else {
    server.send(400, "text/plain", "Fehler: Wert fehlt");
  }
}

#include <DS3231.h>
#include <Wire.h>
#define LED_BUILTIN 2

// Setup clock
DS3231 myRTC;


void setup() {
  // Begin I2C communication
  Wire.begin();
  Serial.begin(115200);

  // Setup alarm one to fire every second
  myRTC.turnOffAlarm(1);
  myRTC.setA1Time(0, 0, 0, 0, 0b01111110, false, false, false);
  myRTC.turnOnAlarm(1);
  myRTC.checkIfAlarm(1);

  WiFi.softAP(ssid, password);
  server.on("/", handleRoot);
  server.on("/set", handleSet);
  server.begin();

  pinMode(L298N_ENA, OUTPUT);
  pinMode(L298N_IN1, OUTPUT);
  pinMode(L298N_IN2, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  // static variable to keep track of LED on/off state
  static byte state = false;

  // if alarm went of, do alarm stuff
  // first call to checkIFAlarm does not clear alarm flag
  if (myRTC.checkIfAlarm(1, false)) {
    minutes++;
    myRTC.checkIfAlarm(1, true);
  }
  if (minutes > 0) {
    state = ~state;
    digitalWrite(L298N_IN2, !state);
    digitalWrite(L298N_IN1, state);
    digitalWrite(L298N_ENA, HIGH);
    digitalWrite(LED_BUILTIN, HIGH);
    minutes--;
    delay(clock_wait);
    digitalWrite(L298N_ENA, LOW);
    digitalWrite(LED_BUILTIN, LOW);
  }
  server.handleClient();
  digitalWrite(L298N_ENA, LOW);
  delay(200);
}
