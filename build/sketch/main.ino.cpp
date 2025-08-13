#include <Arduino.h>
#line 1 "I:\\VS_Code_Programs\\AI_Assistant_Agent\\ESP32\\main\\main.ino"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

#include "wifi_helper.h"
#include "webserver_helper.h"
#include "storage_helper.h"
#include "python_api_helper.h"
#include "config.h"

String deviceNameStr = "esp-light";  // Keep this alive
const char* device_name = nullptr;   // Global pointer to c_str
int NUM_LIGHTS = MAX_LIGHTS;         // Default, can be overwritten by Preferences

AsyncWebServer server(80);

#line 16 "I:\\VS_Code_Programs\\AI_Assistant_Agent\\ESP32\\main\\main.ino"
void setup();
#line 67 "I:\\VS_Code_Programs\\AI_Assistant_Agent\\ESP32\\main\\main.ino"
void loop();
#line 16 "I:\\VS_Code_Programs\\AI_Assistant_Agent\\ESP32\\main\\main.ino"
void setup() {
  Serial.begin(115200);

  // ===== Load Config =====
  preferences.begin("config", true);
  deviceNameStr = preferences.getString("device", "esp-light");
  device_name = deviceNameStr.c_str();

  NUM_LIGHTS = preferences.getInt("numLights", MAX_LIGHTS);
  for (int i = 0; i < NUM_LIGHTS; ++i) {
    String key = "light" + String(i);
    String defaultName = "Light " + String(i + 1);
    lightNames[i] = preferences.getString(key.c_str(), defaultName);
  }
  preferences.end();

  // ===== Setup pins and states =====
  for (int i = 0; i < NUM_LIGHTS; ++i) {
    pinMode(lightPins[i], OUTPUT);
    toggleLight(i, loadLightState(i), false);  // Don’t save during boot
  }

  delay(1000);  // Stabilize before WiFi

  // ===== WiFi Setup =====
  String ssid, pass;
  if (loadCredentials(ssid, pass)) {
    Serial.println("Found saved WiFi credentials.");
    Serial.println("SSID: " + ssid);
    connectToWiFi(ssid, pass);

    unsigned long startAttempt = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 5000) {
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
  static unsigned long lastWiFiCheck = 0;
  const unsigned long wifiCheckInterval = 10000;

  unsigned long now = millis();
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
