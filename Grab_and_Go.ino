#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Servo.h>

// ----- Motor Pins (adjust as needed for your setup) -----
const int ENA = D1;    // PWM pin for left motor enable
const int IN1 = D2;    
const int IN2 = D3;    
const int IN3 = D5;    
const int IN4 = D6;    
const int ENB = D7;    // PWM pin for right motor enable

const int motorSpeed = 800;  // 0–1023 PWM speed

// ----- Servo Pins -----
const int servoPins[] = {D0, D8, A0, D4, D9, D10}; // You can adjust if needed
Servo servos[6];

// ----- Create Web Server on port 80 -----
ESP8266WebServer server(80);

// ----- HTML + CSS + JS -----
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Wemos D1 Robot Control</title>
  <style>
    body { text-align: center; font-family: Arial; }
    button, input[type=range] { margin: 10px; padding: 10px 30px; font-size: 18px; }
    .servo { margin-bottom: 20px; }
  </style>
  <script>
    function sendCmd(cmd) {
      fetch("/" + cmd);
    }
    function setServo(id, val) {
      fetch(/servo?num=${id}&angle=${val});
      document.getElementById("val" + id).innerText = val;
    }
  </script>
</head>
<body>
  <h2>Wemos D1 Robot & Servo Control</h2>
  <button onclick="sendCmd('forward')">FORWARD</button><br>
  <button onclick="sendCmd('left')">LEFT</button>
  <button onclick="sendCmd('right')">RIGHT</button><br>
  <button onclick="sendCmd('backward')">BACKWARD</button><br>
  <button onclick="sendCmd('stop')" style="background:red; color:white;">STOP</button><br><br>

  <h3>Servo Control</h3>
  %SERVO_CONTROLS%
</body>
</html>
)rawliteral";

// ----- Dynamic Servo Sliders -----
String getServoControls() {
  String html = "";
  for (int i = 0; i < 6; i++) {
    int minAngle = (i == 4) ? 0 : 0;
    int maxAngle = (i == 4) ? 20 : 180;
    int defaultAngle = (i == 4) ? 0 : 90;

    html += "<div class='servo'>Servo " + String(i) +
            ": <span id='val" + String(i) + "'>" + String(defaultAngle) + "</span><br>" +
            "<input type='range' min='" + String(minAngle) + "' max='" + String(maxAngle) +
            "' value='" + String(defaultAngle) +
            "' oninput='setServo(" + String(i) + ", this.value)'></div>";
  }
  return html;
}

// ----- Motor Control Functions -----
void moveForward() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  analogWrite(ENA, motorSpeed);
  analogWrite(ENB, motorSpeed);
  server.send(200, "text/plain", "Forward");
}

void moveBackward() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  analogWrite(ENA, motorSpeed);
  analogWrite(ENB, motorSpeed);
  server.send(200, "text/plain", "Backward");
}

void turnLeft() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  analogWrite(ENA, motorSpeed);
  analogWrite(ENB, motorSpeed);
  server.send(200, "text/plain", "Left");
}

void turnRight() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  analogWrite(ENA, motorSpeed);
  analogWrite(ENB, motorSpeed);
  server.send(200, "text/plain", "Right");
}

void stopMotors() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
  server.send(200, "text/plain", "Stopped");
}

// ----- Handle Web Root -----
void handleRoot() {
  String page = htmlPage;
  page.replace("%SERVO_CONTROLS%", getServoControls());
  server.send(200, "text/html", page);
}

// ----- Servo Control Handler -----
void handleServo() {
  if (server.hasArg("num") && server.hasArg("angle")) {
    int num = server.arg("num").toInt();
    int angle = server.arg("angle").toInt();
    if (num >= 0 && num < 6) {
      if (num == 4) angle = constrain(angle, 0, 20);
      else angle = constrain(angle, 0, 180);
      servos[num].write(angle);
      server.send(200, "text/plain", "Servo updated");
    } else {
      server.send(400, "text/plain", "Invalid servo");
    }
  } else {
    server.send(400, "text/plain", "Missing parameters");
  }
}

void setup() {
  Serial.begin(115200);

  // Motor pins
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENB, OUTPUT);

  // Servo setup
  for (int i = 0; i < 6; i++) {
    servos[i].attach(servoPins[i]);
    if (i == 4) servos[i].write(0);
    else servos[i].write(90);
  }

  // Wi-Fi AP Mode
  WiFi.softAP("WEMOS_ROBOT", "12345678");
  Serial.println("Access Point: WEMOS_ROBOT");
  Serial.println("Visit: http://192.168.4.1");

  // Routes
  server.on("/", handleRoot);
  server.on("/forward", moveForward);
  server.on("/backward", moveBackward);
  server.on("/left", turnLeft);
  server.on("/right", turnRight);
  server.on("/stop", stopMotors);
  server.on("/servo", handleServo);
  server.begin();

  Serial.println("Server started.");
}

void loop() {
  server.handleClient();
