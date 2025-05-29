#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <BH1750.h>
#include <DHT.h>
#include <Wire.h>
#include <time.h>
#include "certs.h"  // Chứa certificate_pem_crt, private_pem_key, aws_root_ca

#define DHTPIN D5
#define DHTTYPE DHT11
#define RELAY_PIN D7
#define LED1_PIN D3
#define LED2_PIN D4
#define FAN_PIN D8
#define WATER_SENSOR_PIN D6
#define SOIL_SENSOR_PIN A0

const char* ssid = "";
const char* password = "";
const char* mqtt_server = "";
const int mqtt_port = 8883;
const char* mqtt_topic = "iot/AUR_NODE_1/data";

WiFiClientSecure net;
PubSubClient client(net);
BH1750 lightMeter;
DHT dht(DHTPIN, DHTTYPE);

unsigned long lastSendTime = 0;
const unsigned long interval = 1 * 60 * 1000UL;

void syncTime() {
  configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  time_t now = time(nullptr);
  while (now < 100000) {
    delay(500);
    now = time(nullptr);
  }
}

void connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
}

void connectToMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("AUR_NODE_1")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin(D2, D1);
  lightMeter.begin();
  dht.begin();

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);

  connectToWiFi();
  syncTime();

  net.setTrustAnchors(new X509List(aws_root_ca));
  net.setClientRSACert(new X509List(certificate_pem_crt), new PrivateKey(private_pem_key));
  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (!client.connected()) {
    connectToMQTT();
  }
  client.loop();

  unsigned long currentTime = millis();
  if (currentTime - lastSendTime >= interval || lastSendTime == 0) {
    lastSendTime = currentTime;

    float lux = lightMeter.readLightLevel();
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    if (isnan(temperature) || isnan(humidity)) {
      Serial.println("Failed to read from DHT sensor!");
   //   return;
    }

    int water = analogRead(WATER_SENSOR_PIN);
    int soil = analogRead(SOIL_SENSOR_PIN);


        // In ra dữ liệu cảm biến trên Serial để dễ debug
    Serial.println("----- Sensor Readings -----");
    Serial.print("Light (lux): ");
    Serial.println(lux);
    Serial.print("Temperature (C): ");
    Serial.println(temperature);
    Serial.print("Humidity (%): ");
    Serial.println(humidity);
    Serial.print("Rainfall (analog): ");
    Serial.println(water);
    Serial.print("Soil Moisture (analog): ");
    Serial.println(soil);
    Serial.println("---------------------------");

    time_t now = time(nullptr);
    char isoTime[25];
    strftime(isoTime, sizeof(isoTime), "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));

    String payload = "{";
    payload += "\"device_id\":\"AUR_NODE_1\",";
    payload += "\"timestamp\":\"" + String(isoTime) + "\",";
    payload += "\"temperature\":" + String(temperature, 1) + ",";
    payload += "\"humidity\":" + String(humidity, 1) + ",";
    payload += "\"rainfall\":" + String(water) + ",";
    payload += "\"n\":" + String(soil / 10) + ",";
    payload += "\"p\":" + String(soil / 15) + ",";
    payload += "\"k\":" + String(soil / 20) + ",";
    payload += "\"ph\":" + String((soil / 100.0), 2);
    payload += "}";

    Serial.println("Publishing to MQTT:");
    Serial.println(payload);

    client.publish(mqtt_topic, payload.c_str());
  }
}
