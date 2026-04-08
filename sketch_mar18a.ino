#include "WiFi.h"
#include "WebServer.h"

// ===== WIFI =====
const char* ssid = "iPhone 4s Pro Max";
const char* password = "ngoc1993";

WebServer server(80);

// ===== MOTOR =====
#define IN1 12
#define IN2 13
#define IN3 14
#define IN4 15

// ===== HC-SR04 =====
#define TRIG 5
#define ECHO 18

long duration;
float distance;

// ===== BIẾN LƯU LỆNH =====
String currentCommand = "STOP";

// ===== MOTOR CONTROL =====
void stopMotor() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void forward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void backward() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void left() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void right() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

// ===== ĐỌC KHOẢNG CÁCH =====
float getDistance() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  duration = pulseIn(ECHO, HIGH);
  distance = duration * 0.034 / 2;

  return distance;
}

// ===== HANDLE REQUEST =====
void handleRoot() {
  if (server.hasArg("cmd")) {
    currentCommand = server.arg("cmd");
    Serial.println(currentCommand);
  }

  server.send(200, "text/plain", "OK");
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  stopMotor();

  WiFi.begin(ssid, password);
  Serial.print("Connecting...");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.begin();
}

// ===== LOOP =====
void loop() {
  server.handleClient();

  float d = getDistance();
  Serial.println(d);

  // ===== CHỐNG VA CHẠM =====
  if (d > 0 && d < 5) {
    stopMotor();
    Serial.println("Obstacle!");
    return;
  }

  // ===== THỰC THI LỆNH =====
  if (currentCommand == "FORWARD") forward();
  else if (currentCommand == "BACKWARD") backward();
  else if (currentCommand == "LEFT") left();
  else if (currentCommand == "RIGHT") right();
  else stopMotor();
}