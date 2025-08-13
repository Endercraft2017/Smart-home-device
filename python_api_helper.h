#ifndef PYTHON_API_HELPER_H
#define PYTHON_API_HELPER_H

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "storage_helper.h"
#include "wifi_helper.h"
#include "config.h"

extern const char* device_name;
extern int NUM_LIGHTS;

unsigned long lastToggleTime = 0;
const unsigned long debounceDelay = 500;

bool lightState = false;
bool lightStates[MAX_LIGHTS] = {false};
String lightNames[MAX_LIGHTS] = {"Light 1", "Light 2", "Light 3", "Light 4"};
int lightPins[MAX_LIGHTS] = {2, 4, 5, 18};

void saveLightState(int i, bool state) {
  preferences.begin("light", false);
  String key = "state" + String(i);
  preferences.putBool(key.c_str(), state);
  preferences.end();
}

bool loadLightState(int i) {
  preferences.begin("light", true);
  String key = "state" + String(i);
  bool state = preferences.getBool(key.c_str(), false);
  preferences.end();
  return state;
}

void toggleLight(int i, bool state, bool save = true) {
  lightStates[i] = state;
  digitalWrite(lightPins[i], state ? HIGH : LOW);
  if (save) saveLightState(i, state);
  Serial.printf("Light %d (%s): %s\n", i, lightNames[i].c_str(), state ? "ON" : "OFF");
}

void setupPythonRoutes(AsyncWebServer& server) {
  // /id
  server.on("/id", HTTP_GET, [](AsyncWebServerRequest *request) {
    DynamicJsonDocument doc(512);
    doc["device"] = device_name;

    JsonArray lights = doc.createNestedArray("lights");
    for (int i = 0; i < NUM_LIGHTS; ++i) {
      JsonObject light = lights.createNestedObject();
      light["name"] = lightNames[i];
      light["api"] = "/" + lightNames[i] + "/toggle";
    }

    String json;
    serializeJson(doc, json);
    request->send(200, "application/json", json);
  });

  // /status
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    DynamicJsonDocument doc(256);
    JsonArray states = doc.createNestedArray("states");
    for (int i = 0; i < NUM_LIGHTS; ++i) {
      JsonObject obj = states.createNestedObject();
      obj["name"] = lightNames[i];
      obj["state"] = lightStates[i] ? "on" : "off";
    }

    String json;
    serializeJson(doc, json);
    request->send(200, "application/json", json);
  });

  // /lightName/toggle
  for (int i = 0; i < NUM_LIGHTS; ++i) {
    String path = "/" + lightNames[i] + "/toggle";
    server.on(path.c_str(), HTTP_POST, [i](AsyncWebServerRequest* request) {}, NULL,
    [i](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t, size_t) {
      if (millis() - lastToggleTime < debounceDelay) {
        request->send(429, "application/json", "{\"error\": \"Too many requests\"}");
        return;
      }
      lastToggleTime = millis();

      DynamicJsonDocument doc(256);
      if (deserializeJson(doc, data, len)) {
        request->send(400, "application/json", "{\"error\": \"Invalid JSON\"}");
        return;
      }

      String action = doc["action"];
      if (action == "on") {
        toggleLight(i, true);
      } else if (action == "off") {
        toggleLight(i, false);
      } else {
        request->send(400, "application/json", "{\"error\": \"Unknown action\"}");
        return;
      }

      request->send(200, "application/json", "{\"status\": \"success\"}");
    });
  }

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
    int lightIndex = doc["light"];

    if (lightIndex < 0 || lightIndex >= NUM_LIGHTS) {
      request->send(400, "application/json", "{\"error\": \"Invalid light index\"}");
      return;
    }

    if (action == "on") {
      toggleLight(lightIndex, true, false);
    } else if (action == "off") {
      toggleLight(lightIndex, false, false);
    } else {
      request->send(400, "application/json", "{\"error\": \"Unknown action\"}");
      return;
    }

    String response = "{\"light\":" + String(lightIndex) +
                      ",\"name\":\"" + lightNames[lightIndex] + "\"," +
                      "\"state\":" + String(lightStates[lightIndex] ? 1 : 0) + "}";

    request->send(200, "application/json", response);
  });


  // /test page
  server.on("/test", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = R"rawliteral(
      <!DOCTYPE html>
      <html>
      <head>
        <title>Test Lights</title>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <meta name="color-scheme" content="light">
        <style>
          body {
            font-family: 'Segoe UI', sans-serif;
            background-color: #74b7ffff;
            padding: 24px;
            text-align: center;
          }

          .grid-container {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(120px, 1fr));
            gap: 16px;
            justify-items: center;
            margin: 20px auto;
            max-width: 300px;
          }

          .light-box {
            width: 120px;
            height: 120px;
            display: flex;
            align-items: center;
            justify-content: center;
            font-size: 16px;
            font-weight: bold;
            color: #000;
            border-radius: 16px;
            border: 2px solid #0e0033ff;
            background-color: gray;
            transition: background-color 0.3s, box-shadow 0.3s;
            cursor: pointer;
            user-select: none;
          }

          .light-box.on {
            background-color: #ccff66;
            border-color: #aaff00;
            box-shadow: 0 0 8px #aaff00;
          }

          .restart-button {
            position: fixed;
            bottom: 12px;
            right: 12px;
            background-color: #4f7fb3ff;
            color: white;
            padding: 10px 16px;
            border-radius: 8px;
            text-decoration: none;
            font-size: 16px;
          }
        </style>
        <script>
          const NUM_LIGHTS = )rawliteral" + String(NUM_LIGHTS) + R"rawliteral(;

          function updateStates() {
            fetch("/status")
              .then(res => res.json())
              .then(data => {
                const states = data.states;
                for (let i = 0; i < NUM_LIGHTS; ++i) {
                  const box = document.getElementById("light" + i);
                  if (box && states[i]?.state === "on") {
                    box.classList.add("on");
                  } else if (box) {
                    box.classList.remove("on");
                  }
                }
              });
          }

          function toggleLight(i) {
            const box = document.getElementById("light" + i);
            const isOn = box.classList.contains("on");
            const action = isOn ? "off" : "on";

            fetch("/testToggle", {
              method: "POST",
              headers: { "Content-Type": "application/json" },
              body: JSON.stringify({ light: i, action: action })
            })
            .then(() => updateStates());
          }

          window.onload = updateStates;
        </script>
      </head>
      <body>
        <h2>Toggle Lights</h2>
        <div class="grid-container">
    )rawliteral";

    // Dynamically generate light boxes
    for (int i = 0; i < NUM_LIGHTS; ++i) {
      html += "<div id='light" + String(i) + "' class='light-box' onclick='toggleLight(" + String(i) + ")'>";
      html += lightNames[i];
      html += "</div>";
    }
    html += R"rawliteral(
      <a href="/restart" style="position: fixed; bottom: 12px; right: 12px; background-color: #4a90e2; color: white; padding: 10px 16px; border-radius: 8px; text-decoration: none; font-size: 14px;">Restart</a>
    </body>
    </html>
    )rawliteral";
  
    request->send(200, "text/html", html);
  });
  server.on("/restart", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Restarting...");
    request->redirect("/");
    delay(500);
    ESP.restart();
  });
}

#endif