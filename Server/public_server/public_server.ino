#include <ESP8266WiFi.h>
#include <WiFiClientSecureBearSSL.h>
#include "certs.h"  // file .ino chứa chứng chỉ

const char* ssid = "";
const char* password = "";

const char* host = "";
const int httpsPort = 443;

BearSSL::WiFiClientSecure client;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");

  BearSSL::X509List cert(certificate_pem_crt);
  BearSSL::PrivateKey key(private_pem_key);
  BearSSL::X509List ca(aws_root_ca);

  client.setClientRSACert(&cert, &key);
  client.setTrustAnchors(&ca);

  if (!client.connect(host, httpsPort)) {
    Serial.println("Connection failed!");
    return;
  }

  Serial.println("Connected to AWS!");

  String payload = "{\"message\":\"Hello from ESP8266\"}";

  client.print(String("POST /your/api/path HTTP/1.1\r\n") +
               "Host: " + host + "\r\n" +
               "Content-Type: application/json\r\n" +
               "Content-Length: " + payload.length() + "\r\n\r\n" +
               payload + "\r\n");

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") break;
    Serial.println(line);
  }

  client.stop();
}

void loop() {
}
