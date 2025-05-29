#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

const char* ssid = "";
const char* password = "";

const String readAPIKey = "";
const String channelID = "";
const String field = "1";

const int pumpRelayPin = D5; // Relay điều khiển máy bơm
const int fanRelayPin  = D6; // Relay điều khiển quạt

unsigned long lastCommandTime = 0;
int lastCommand = -1;

void setup() {
  Serial.begin(9600);
  WiFi.begin(ssid, password);
  Serial.print("Đang kết nối WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nĐã kết nối WiFi");

  pinMode(pumpRelayPin, OUTPUT);
  pinMode(fanRelayPin, OUTPUT);

  digitalWrite(pumpRelayPin, HIGH); // Relay OFF (LOW bật, HIGH tắt)
  digitalWrite(fanRelayPin, HIGH);  // Relay OFF
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String url = "http://api.thingspeak.com/channels/" + channelID + "/fields/" + field + "/last.txt?api_key=" + readAPIKey;
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      int command = payload.toInt();
      Serial.print("Lệnh nhận được: "); Serial.println(command);

      if (command != lastCommand && command >= 0 && command <= 3) {
        lastCommand = command;
        handleCommand(command);
      } else {
        Serial.println("Không có lệnh mới hoặc lệnh không hợp lệ.");
      }
    } else {
      Serial.print("Lỗi HTTP: ");
      Serial.println(httpCode);
    }

    http.end();
  } else {
    Serial.println("Mất kết nối WiFi. Đang kết nối lại...");
    WiFi.reconnect();
  }

  delay(10000); // Check mỗi 10s
}

void handleCommand(int cmd) {
  switch (cmd) {
    case 0:
      Serial.println("Thêm phân bón (Add Fertilizer)");
      runPumpAndFan();
      break;
    case 1:
      Serial.println("Tăng tưới nước (Increase Watering)");
      runPumpAndFan();
      break;
    case 2:
      Serial.println("Không hành động (No Specific Action)");
      break;
    case 3:
      Serial.println("Tăng độ ẩm (Provide More Humidity)");
      runPumpAndFan();
      break;
  }
}

void runPumpAndFan() {
  // Bật relay (LOW là ON)
  digitalWrite(pumpRelayPin, LOW);
  digitalWrite(fanRelayPin, LOW);
  Serial.println("Đang bật máy bơm (5s) và quạt (5 phút)");

  delay(5000); // 5 giây máy bơm

  digitalWrite(pumpRelayPin, HIGH); // Tắt máy bơm
  Serial.println("Tắt máy bơm");

  unsigned long fanStart = millis();
  while (millis() - fanStart < 5 * 60 * 1000UL) {
    delay(500);
  }

  digitalWrite(fanRelayPin, HIGH); // Tắt quạt
  Serial.println("Tắt quạt");
}
