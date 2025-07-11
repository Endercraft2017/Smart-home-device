#include <Arduino.h>
#line 1 "I:\\VS_Code_Programs\\AI_Assistant_Agent\\ESP32\\main\\main.ino"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

#include "wifi_helper.h"
#include "webserver_helper.h"
#include "storage_helper.h"
#include "python_api_helper.h"
#include "config.h"

unsigned long lastWiFiCheck = 0;
const unsigned long wifiCheckInterval = 10000;  // 10 seconds


const char* device_name = "light_1";  // global used by python_api_helper
AsyncWebServer server(80); // Create AsyncWebServer object on port 80

#line 17 "I:\\VS_Code_Programs\\AI_Assistant_Agent\\ESP32\\main\\main.ino"
void setup();
#line 53 "I:\\VS_Code_Programs\\AI_Assistant_Agent\\ESP32\\main\\main.ino"
void loop();
#line 17 "I:\\VS_Code_Programs\\AI_Assistant_Agent\\ESP32\\main\\main.ino"
void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  toggleLight(loadLightState());  // restore LED state

  delay(1000); // Add in setup() before WiFi startup

  String ssid, pass;
  if (loadCredentials(ssid, pass)) {
  Serial.println("Found saved WiFi credentials.");
  Serial.println("SSID: " + ssid);
  connectToWiFi(ssid, pass);

  unsigned long startAttemptTime = millis();
  const unsigned long timeout = 5000;

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < timeout) {
    delay(100);
    yield();
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Failed to connect. Starting AP mode...");
    setupAP();
  }
  } else {
  Serial.println("No saved WiFi credentials. Starting AP mode...");
  setupAP();
  }


  setupWebRoutes(server);
  setupPythonRoutes(server);
  server.begin();
}

void loop() {
  unsigned long now = millis();

  // Check WiFi status periodically
  if (now - lastWiFiCheck > wifiCheckInterval) {
    lastWiFiCheck = now;
    if (WiFi.getMode() == WIFI_STA && WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi disconnected. Trying to reconnect...");
      String ssid, pass;
      if (loadCredentials(ssid, pass)) {
        connectToWiFi(ssid, pass);
      } else {
        setupAP();
      }
    }
  }
}

