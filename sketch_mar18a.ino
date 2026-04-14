#include <WiFi.h>
#include <WebServer.h>

// ===== WIFI =====
const char* ssid = "P202";
const char* password = "12345678@9";

WebServer server(80);

// ===== L298N Pins =====
#define IN1 12
#define IN2 13
#define IN3 14
#define IN4 15
#define ENA 25
#define ENB 26

// ===== HC-SR04 =====
#define TRIG 2
#define ECHO 4

// ===== BUZZER =====
#define BUZZER 27          // ← Chân loa nhỏ

String currentCommand = "STOP";

// ====================== MOTOR ======================
void stopMotor() {
  digitalWrite(ENA, LOW);
  digitalWrite(ENB, LOW);
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
}

void forward() {
  digitalWrite(ENA, HIGH);
  digitalWrite(ENB, HIGH);
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}

void backward() {
  digitalWrite(ENA, HIGH);
  digitalWrite(ENB, HIGH);
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
}

void turnLeft() {
  digitalWrite(ENA, HIGH);
  digitalWrite(ENB, HIGH);
  digitalWrite(IN1, LOW);  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}

void turnRight() {
  digitalWrite(ENA, HIGH);
  digitalWrite(ENB, HIGH);
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, LOW);
}

// ====================== SAFE DISTANCE + BUZZER ======================
float getDistance() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  long duration = pulseIn(ECHO, HIGH, 20000);
  if (duration == 0 || duration > 20000) return 999.0;
  return duration * 0.034 / 2.0;
}

void buzzerAlert(float distance) {
  if (distance > 0 && distance < 15) {           // Cảnh báo khi < 15cm
    digitalWrite(BUZZER, HIGH);                  // Kêu liên tục
  } else {
    digitalWrite(BUZZER, LOW);                   // Tắt loa
  }
}

// ====================== WEB ======================
void handleRoot() {
  if (server.hasArg("cmd")) {
    currentCommand = server.arg("cmd");
    Serial.print("→ Command: ");
    Serial.println(currentCommand);
  }
  server.send(200, "text/plain", "OK");
}

// ====================== SETUP ======================
void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT); pinMode(ENB, OUTPUT);
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(BUZZER, OUTPUT);          // ← Thêm buzzer

  digitalWrite(ENA, HIGH);
  digitalWrite(ENB, HIGH);
  digitalWrite(BUZZER, LOW);        // Tắt loa ban đầu
  stopMotor();

  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
  Serial.print("ESP32 DevKit V1 IP: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.begin();
  Serial.println("Server started - Gesture control + Buzzer ready");
}

// ====================== LOOP ======================
void loop() {
  server.handleClient();

  static unsigned long lastDist = 0;
  float distance = 999.0;

  if (millis() - lastDist > 80) {
    distance = getDistance();
    lastDist = millis();
  }

  buzzerAlert(distance);   // ← Gọi hàm loa cảnh báo

  if (distance > 0 && distance < 10) {
    stopMotor();
    currentCommand = "STOP";
    Serial.println("Obstacle detected! Buzzer ON");
    delay(50);
    return;
  }

  if (currentCommand == "FORWARD")      forward();
  else if (currentCommand == "BACKWARD") backward();
  else if (currentCommand == "LEFT")    turnLeft();
  else if (currentCommand == "RIGHT")   turnRight();
  else                                  stopMotor();

  delay(10);
}