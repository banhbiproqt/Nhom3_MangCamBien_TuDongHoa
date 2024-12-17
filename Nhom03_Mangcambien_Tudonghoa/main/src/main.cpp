#include <WiFi.h>
#include <HTTPClient.h>

// Thông tin WiFi
const char* ssid = "banhbi";
const char* password = "123456789a";

// Địa chỉ server Express.js
const char* serverCommandUrl = "http://192.168.107.106:3000/status";
const char* serverLogUrl = "http://192.168.107.106:3000/log";
const char* serverRainUrl = "http://192.168.107.106:3000/rainStatus";

// Khai báo các chân cảm biến và điều khiển
#define RAIN_SENSOR_PIN 34
#define LIMIT_SWITCH_LEFT 32
#define LIMIT_SWITCH_RIGHT 33
#define MOTOR_ENA_PIN 19
#define MOTOR_IN1_PIN 18
#define MOTOR_IN2_PIN 5

// Biến trạng thái
bool autoMode = false;       // Chế độ tự động
bool manualMoving = false;   // Đang di chuyển trong chế độ thủ công

// Prototype các hàm
void sendLogToServer(String logMessage);
void sendRainStatus();
void handleAutoMode();
void handleMoveOut();
void handleMoveIn();
void handleStop();
void toggleAutoMode();
void moveMotorOut();
void moveMotorIn();
void stopMotor();

void setup() {
  pinMode(RAIN_SENSOR_PIN, INPUT);
  pinMode(LIMIT_SWITCH_LEFT, INPUT_PULLUP);
  pinMode(LIMIT_SWITCH_RIGHT, INPUT_PULLUP);
  pinMode(MOTOR_ENA_PIN, OUTPUT);
  pinMode(MOTOR_IN1_PIN, OUTPUT);
  pinMode(MOTOR_IN2_PIN, OUTPUT);

  Serial.begin(9600);
  Serial.println("Kết nối WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nKết nối thành công!");
  Serial.print("IP ESP32: ");
  Serial.println(WiFi.localIP());
  sendLogToServer("ESP32 Connected to WiFi");
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // Gửi yêu cầu HTTP GET đến server Express.js để nhận lệnh điều khiển
    http.begin(serverCommandUrl);
    int httpCode = http.GET();

    if (httpCode > 0) {
      String command = http.getString();
      Serial.print("Lệnh nhận từ server: ");
      Serial.println(command);

      // Ghi log lệnh nhận được
      sendLogToServer("Command received: " + command);

      // Thực hiện điều khiển động cơ dựa trên lệnh nhận được
      if (command == "moveOut") {
        handleMoveOut();
      } else if (command == "moveIn") {
        handleMoveIn();
      } else if (command == "stop") {
        handleStop();
      } else if (command == "toggleMode") {
        handleAutoMode();
      } else {
        sendLogToServer("Invalid command received");
      }
    } else {
      Serial.print("Lỗi HTTP: ");
      Serial.println(httpCode);
      sendLogToServer("HTTP request failed");
    }
    http.end();

    // Gửi trạng thái mưa định kỳ lên server
    sendRainStatus();

    // Xử lý chế độ tự động
    // if (autoMode) {
    //   handleAutoMode();
    // }
  } else {
    Serial.println("WiFi mất kết nối! Đang kết nối lại...");
    WiFi.begin(ssid, password);
  }

  delay(500); // Chu kỳ lặp 500ms
}

void sendLogToServer(String logMessage) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverLogUrl);
    http.addHeader("Content-Type", "application/json");

    String jsonPayload = "{\"log\":\"" + logMessage + "\"}";
    int httpCode = http.POST(jsonPayload);
    http.end();
  }
}

void sendRainStatus() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverRainUrl);
    http.addHeader("Content-Type", "application/json");

    String rainStatus = digitalRead(RAIN_SENSOR_PIN) == LOW ? "Raining" : "Not Raining";
    String jsonPayload = "{\"rain\":\"" + rainStatus + "\"}";
    int httpCode = http.POST(jsonPayload);
    if (httpCode > 0) {
      Serial.println("Rain Status Sent: " + rainStatus);
    }
    http.end();
  }
}

void handleAutoMode() {
  if (digitalRead(RAIN_SENSOR_PIN) == LOW) { // Trời mưa
    if (digitalRead(LIMIT_SWITCH_RIGHT) == HIGH) {
      moveMotorIn();
    } else {
      stopMotor();
    }
  } else { // Trời không mưa
    if (digitalRead(LIMIT_SWITCH_LEFT) == HIGH) {
      moveMotorOut();
    } else {
      stopMotor();
    }
  }
}

void handleMoveOut() {
  if (!autoMode) {
    if (digitalRead(LIMIT_SWITCH_LEFT) == HIGH) {
      moveMotorOut();
    } else {
      stopMotor();
      sendLogToServer("Cannot Move Out: Limit Switch Triggered");
    }
  }
}

void handleMoveIn() {
  if (!autoMode) {
    if (digitalRead(LIMIT_SWITCH_RIGHT) == HIGH) {
      moveMotorIn();
    } else {
      stopMotor();
      sendLogToServer("Cannot Move In: Limit Switch Triggered");
    }
  }
}

void handleStop() {
  stopMotor();
  sendLogToServer("Motor Stopped");
}

// void toggleAutoMode() {
//   autoMode = !autoMode;
//   stopMotor();
//   sendLogToServer(autoMode ? "Auto Mode: Enabled" : "Auto Mode: Disabled");
// }

void moveMotorOut() {
  digitalWrite(MOTOR_ENA_PIN, HIGH);
  digitalWrite(MOTOR_IN1_PIN, HIGH);
  digitalWrite(MOTOR_IN2_PIN, LOW);
  sendLogToServer("Motor: Moving Out");
}

void moveMotorIn() {
  digitalWrite(MOTOR_ENA_PIN, HIGH);
  digitalWrite(MOTOR_IN1_PIN, LOW);
  digitalWrite(MOTOR_IN2_PIN, HIGH);
  sendLogToServer("Motor: Moving In");
}

void stopMotor() {
  digitalWrite(MOTOR_ENA_PIN, LOW);
  digitalWrite(MOTOR_IN1_PIN, LOW);
  digitalWrite(MOTOR_IN2_PIN, LOW);
  sendLogToServer("Motor: Stopped");
}
