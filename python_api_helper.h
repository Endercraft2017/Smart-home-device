#ifndef PYTHON_API_HELPER_H
#define PYTHON_API_HELPER_H

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "storage_helper.h"
#include "wifi_helper.h"
#include "config.h"

extern const char* device_name;
unsigned long lastToggleTime = 0;
const unsigned long debounceDelay = 500;  // 500ms debounce

bool lightState = false;

void saveLightState(bool state) {
  preferences.begin("light", false);
  preferences.putBool("state", state);
  preferences.end();
}

bool loadLightState() {
  preferences.begin("light", true);
  bool state = preferences.getBool("state", false);
  preferences.end();
  return state;
}

void toggleLight(bool state) {
  lightState = state;
  digitalWrite(LED, lightState ? HIGH : LOW);
  saveLightState(lightState);
  Serial.println(lightState ? "Light ON" : "Light OFF");
}

void setupPythonRoutes(AsyncWebServer& server) {
  // /id
  server.on("/id", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "{\"device\": \"" + String(device_name) + "\"}";
    request->send(200, "application/json", json);
  });

  // /status
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    String status = lightState ? "on" : "off";
    request->send(200, "application/json", "{\"light\": \"" + status + "\"}");
  });

  // /toggle
  server.on("/toggle", HTTP_POST, [](AsyncWebServerRequest* request) {}, NULL,
  [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t, size_t) {
    // Debounce check
    unsigned long now = millis();
    if (now - lastToggleTime < debounceDelay) {
      request->send(429, "application/json", "{\"error\": \"Too many requests\"}");
      return;
    }
    lastToggleTime = now;

    // Parse JSON
    DynamicJsonDocument doc(256);
    DeserializationError err = deserializeJson(doc, data, len);
    if (err) {
      request->send(400, "application/json", "{\"error\": \"Invalid JSON\"}");
      return;
    }

    String action = doc["action"];
    if (action == "on") {
      toggleLight(true);
    } else if (action == "off") {
      toggleLight(false);
    } else {
      request->send(400, "application/json", "{\"error\": \"Unknown action\"}");
      return;
    }

    request->send(200, "application/json", "{\"status\": \"success\"}");
    });

  // /newWiFiCredentials
  server.on("/newWiFiCredentials", HTTP_POST, [](AsyncWebServerRequest* request) {}, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t, size_t) {
      DynamicJsonDocument doc(256);
      DeserializationError err = deserializeJson(doc, data, len);
      if (err) {
        request->send(400, "application/json", "{\"error\": \"Invalid JSON\"}");
        return;
      }

      String new_ssid = doc["ssid"];
      String new_pass = doc["password"];
      Serial.println("Trying new credentials: " + new_ssid);

      WiFi.begin(new_ssid.c_str(), new_pass.c_str());
      unsigned long startAttempt = millis();
      while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 10000) {
        delay(500);
        Serial.print(".");
      }

      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected to new WiFi!");
        saveCredentials(new_ssid, new_pass);
        request->send(200, "application/json", "{\"status\": \"connected\"}");
      } else {
        Serial.println("\nFailed to connect. Staying on old WiFi.");
        request->send(500, "application/json", "{\"error\": \"Failed to connect\"}");
        setupAP(); // fallback if needed
      }
    });

  // /testToggle (temporary toggle - no save)
  server.on("/testToggle", HTTP_POST, [](AsyncWebServerRequest* request) {}, NULL,
  [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t, size_t) {
    unsigned long now = millis();
    if (now - lastToggleTime < debounceDelay) {
      request->send(429, "application/json", "{\"error\": \"Too many requests\"}");
      return;
    }
    lastToggleTime = now;

    DynamicJsonDocument doc(256);
    DeserializationError err = deserializeJson(doc, data, len);
    if (err) {
      request->send(400, "application/json", "{\"error\": \"Invalid JSON\"}");
      return;
    }

    String action = doc["action"];
    if (action == "on") {
      lightState = true;
      digitalWrite(LED, HIGH);  // no save
    } else if (action == "off") {
      lightState = false;
      digitalWrite(LED, LOW);  // no save
    } else {
      request->send(400, "application/json", "{\"error\": \"Unknown action\"}");
      return;
    }

    request->send(200, "application/json", "{\"status\": \"toggled\"}");
  });

  // /test page
  server.on("/test", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = R"rawliteral(
      <!DOCTYPE html>
      <html>
      <head>
        <title>Test Page</title>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <style>
          body {
            font-family: 'Segoe UI', sans-serif;
            text-align: center;
            background-color: #c9dcf1;
            padding: 24px;
          }
          h2 {
            font-size: 28px;
          }
          button {
            background-color: #598cc2;
            color: white;
            border: none;
            padding: 12px 24px;
            font-size: 16px;
            margin-top: 12px;
            border-radius: 8px
            cursor: pointer;
          }
          #state {
            font-size: 20px;
            margin-top: 20px;
          }
        </style>
        <script>
          function fetchState() {
            fetch("/status")
              .then(res => res.json())
              .then(data => {
                document.getElementById("state").innerText = "State: " + data.light;
              });
          }

          function toggleLight(action) {
            fetch("/testToggle", {
              method: "POST",
              headers: { "Content-Type": "application/json" },
              body: JSON.stringify({ action: action })
            }).then(() => fetchState());
          }

          window.onload = fetchState;
        </script>
      </head>
      <body>
        <h2>Test Page</h2>
        <button onclick="toggleLight('on')">Turn ON</button>
        <button onclick="toggleLight('off')">Turn OFF</button>
        <p id="state">State: Loading...</p>
      </body>
      </html>
    )rawliteral";

    request->send(200, "text/html", html);
  });

}

#endif