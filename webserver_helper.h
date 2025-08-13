#ifndef WEBSERVER_HELPER_H
#define WEBSERVER_HELPER_H

#include "config.h"              // ⬅️ Add this first to get MAX_LIGHTS etc
#include "storage_helper.h"     // ⬅️ This gives us lightNames[] and NUM_LIGHTS
#include "wifi_helper.h"
#include <ESPAsyncWebServer.h>

String cachedSSIDOptions = "";

// Only called on boot and during manual rescan
void rescanWiFiNetworks() {
  static unsigned long lastScanTime = 0;
  unsigned long now = millis();
  if (now - lastScanTime < 5000) return;  // 5 second delay between rescans

  lastScanTime = now;
  cachedSSIDOptions = "";
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; ++i) {
    cachedSSIDOptions += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + "</option>";
  }
}

void setupWebRoutes(AsyncWebServer &server) {
  // Initial scan at boot
  rescanWiFiNetworks();

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String lastConnected = preferences.getString("ssid", "");

      String html = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <title>WiFi Setup</title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
      body {
        font-family: 'Segoe UI', sans-serif;
        background-color: #74b7ffff;
        margin: 0;
        padding: 24px;
        font-size: 16px;
        display: flex;
        flex-direction: column;
        align-items: center;
      }

      .container {
        display: flex;
        flex-direction: column;
        align-items: center;
        width: 100%;
        max-width: 420px;
      }

      h2 {
        font-size: 28px;
        margin-bottom: 20px;
        text-align: center;
        color: #333;
      }

      form {
        background-color: #589ee9ff;
        padding: 24px;
        border-radius: 16px;
        text-align: left;
        width: 100%;
        box-shadow: 0 4px 12px rgba(0, 0, 0, 0.08);
        box-sizing: border-box;
      }

      input, select {
        font-size: 16px;
        padding: 8px 12px;
        width: 100%;
        margin: 12px 0;
        box-sizing: border-box;
      }

      button, input[type=submit] {
        background-color: #4f7fb3ff;
        color: white;
        border: none;
        padding: 12px 20px;
        font-size: 16px;
        margin-top: 12px;
        cursor: pointer;
        border-radius: 8px;
      }

      #spinner {
        display: none;
        margin-top: 12px;
        width: 32px;
        height: 32px;
        border: 4px solid #ccc;
        border-top: 4px solid #333;
        border-radius: 50%;
        animation: spin 1s linear infinite;
      }

      @keyframes spin {
        0% { transform: rotate(0deg); }
        100% { transform: rotate(360deg); }
      }

      .password-wrapper {
        position: relative;
      }

      .password-wrapper input {
        padding-right: 44px;
      }

      .password-wrapper svg {
        width: 24px;
        height: 24px;
        position: absolute;
        right: 10px;
        top: 50%;
        transform: translateY(-50%);
        cursor: pointer;
      }

      #status {
        margin-top: 12px;
        color: #444;
      }
    </style>
    <script>
      function rescan() {
        document.getElementById("spinner").style.display = "block";
        document.getElementById("rescanBtn").disabled = true;
        document.getElementById("status").innerText = "Scanning for networks...";
        fetch("/rescan").then(() => location.reload());
      }

      function togglePassword() {
        const input = document.getElementById("password");
        const icon = document.getElementById("togglePassword");
        if (input.type === "password") {
          input.type = "text";
          icon.innerHTML = `
            <path d='M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z'/>
            <circle cx='12' cy='12' r='3'/>`;
        } else {
          input.type = "password";
          icon.innerHTML = `
            <path d='M1 1l22 22'/>
            <path d='M17.94 17.94A10.94 10.94 0 0112 20c-5 0-9.27-3-11-7
                     1.13-2.47 3.17-4.47 5.66-5.68'/>
            <path d='M9.88 9.88A3 3 0 1114.12 14.12'/>`;
        }
      }

      window.addEventListener("DOMContentLoaded", () => {
        document.getElementById("togglePassword").addEventListener("click", togglePassword);
      });
    </script>
  </head>
  <body>
    <div class="container">
      <h2>Connect to WiFi</h2>
  )rawliteral";


    if (lastConnected.length() > 0) {
    html += "<p><b>Last connected SSID:</b> " + lastConnected + "</p>";
  }

  html += R"rawliteral(
      <form action='/connect' method='get'>
        SSID:
        <select name='ssid'>
  )rawliteral";

  html += cachedSSIDOptions;

  html += R"rawliteral(
        </select><br>
        Password:
        <div class="password-wrapper">
          <input id="password" type="password" name="password">
          <svg id="togglePassword" xmlns="http://www.w3.org/2000/svg" fill="none"
               stroke="#333" stroke-width="2" stroke-linecap="round"
               stroke-linejoin="round" viewBox="0 0 24 24">
            <path d="M1 1l22 22"/>
            <path d="M17.94 17.94A10.94 10.94 0 0112 20c-5 0-9.27-3-11-7
                     1.13-2.47 3.17-4.47 5.66-5.68"/>
            <path d="M9.88 9.88A3 3 0 1114.12 14.12"/>
          </svg>
        </div><br>
        <input type="submit" value="Connect">
      </form>

      <p id="status"></p>
      <button id="rescanBtn" onclick="rescan()">Scan WiFi</button>
      <div id="spinner"></div>
    </div>
  </body>
  </html>
  )rawliteral";
  html += R"rawliteral(
    <a href="/test" style="position: fixed; bottom: 12px; right: 12px; background-color: #4a90e2; color: white; padding: 10px 16px; border-radius: 8px; text-decoration: none; font-size: 14px;">Test</a>
    <a href="/config" style="position: fixed; bottom: 12px; left: 12px; background-color: #4a90e2; color: white; padding: 10px 16px; border-radius: 8px; text-decoration: none; font-size: 14px;">Config</a>
  </body>
  </html>
  )rawliteral";

  request->send(200, "text/html", html);

  });

  // Rescan endpoint
  server.on("/rescan", HTTP_GET, [](AsyncWebServerRequest *request){
    rescanWiFiNetworks();
    request->send(200, "text/plain", "ok");
  });

  // Handle connect
  server.on("/connect", HTTP_GET, [](AsyncWebServerRequest *request){
    if (!request->hasParam("ssid") || !request->hasParam("password")) {
      request->send(400, "text/html", "<h2>Error: Missing SSID or password</h2>");
      return;
    }

    String ssid = request->getParam("ssid")->value();
    String password = request->getParam("password")->value();

    if (ssid.length() == 0 || password.length() == 0) {
      request->send(400, "text/html", "<h2>Error: SSID and password cannot be empty</h2>");
      return;
    }

    Serial.println("Received WiFi credentials:");
    Serial.println("SSID: " + ssid);
    Serial.println("Password: " + password);

    saveCredentials(ssid, password);
    connectToWiFi(ssid, password);

    if (WiFi.status() == WL_CONNECTED) {
      request->send(200, "text/html", "<h2>Connected!</h2><p>You may now close this page.</p>");
    } else {
      setupAP();  // fallback
      request->send(500, "text/html", "<h2>Failed to connect. Try again.</h2>");
    }
  });
  // /config - Light and device configuration page
  server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = R"rawliteral(
      <!DOCTYPE html>
      <html>
      <head>
        <title>Device Config</title>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <style>
          body {
            font-family: 'Segoe UI', sans-serif;
            background-color: #74b7ff;
            margin: 0;
            padding: 20px;
            display: flex;
            justify-content: center;
            align-items: flex-start;
            min-height: 100vh;
          }
          .container {
            width: 100%;
            max-width: 480px;
            background: #589ee9ff;
            padding: 24px;
            border-radius: 16px;
            box-shadow: 0 4px 12px rgba(0, 0, 0, 0.08);
            box-sizing: border-box;
          }
          h2 {
            text-align: center;
            margin-top: 0;
          }
          form {
            max-width: 100%;
          }
          form label {
            display: block;
            margin-top: 12px;
            font-weight: 600;
          }
          form input,
          form select {
            width: 100%;
            padding: 8px;
            margin-top: 4px;
            border-radius: 8px;
            border: 1px solid #ccc;
            box-sizing: border-box;
          }
          .light-input {
          margin-top: 12px;
          }
          button {
            margin-top: 20px;
            width: 100%;
            padding: 12px;
            background: #4f7fb3;
            color: white;
            border: none;
            border-radius: 8px;
            font-size: 16px;
            cursor: pointer;
          }
        </style>
      </head>
      <script>
        function updateLightInputs() {
          const count = parseInt(document.querySelector("select[name='numLights']").value);
          for (let i = 0; i < 4; i++) {
            const group = document.getElementById("lightGroup" + i);
            if (i < count) {
              group.style.display = "block";
            } else {
              group.style.display = "none";
            }
          }
        }

        document.addEventListener("DOMContentLoaded", () => {
          document.querySelector("select[name='numLights']").addEventListener("change", updateLightInputs);
          updateLightInputs();  // run once at load
        });
      </script>
      <body>
        <div class="container">
          <h2>Device Configuration</h2>
        <form action="/saveConfig" method="get">
          <label>Device Name</label>
          <input name="device" value=")rawliteral";
      html += device_name;
      html += R"rawliteral("> 
          <label>Number of Lights</label>
          <select name="numLights">
    )rawliteral";

    for (int i = 1; i <= MAX_LIGHTS; ++i) {
      html += "<option value='" + String(i) + "'";
      if (i == NUM_LIGHTS) html += " selected";
      html += ">" + String(i) + "</option>";
    }

    html += R"rawliteral(
          </select>
    )rawliteral";

    for (int i = 0; i < MAX_LIGHTS; ++i) {
      html += "<div class='light-input' id='lightGroup" + String(i) + "'>";
      html += "<label>Light " + String(i + 1) + " Name</label>";
      html += "<input name='light" + String(i) + "' value='" + lightNames[i] + "'>";
      html += "</div>";
    }


    html += R"rawliteral(
          <button type="submit">Save</button>
        </form>
        </div>
      </body>
      </html>
    )rawliteral";

    request->send(200, "text/html", html);
  });

  // /saveConfig - Save configuration
  server.on("/saveConfig", HTTP_GET, [](AsyncWebServerRequest *request) {
    preferences.begin("config", false);

    if (request->hasParam("device")) {
      String newDevice = request->getParam("device")->value();
      preferences.putString("device", newDevice);
    }

    if (request->hasParam("numLights")) {
      int n = request->getParam("numLights")->value().toInt();
      if (n >= 1 && n <= MAX_LIGHTS) {
        preferences.putInt("numLights", n);
      }
    }

    for (int i = 0; i < MAX_LIGHTS; ++i) {
      String param = "light" + String(i);
      if (request->hasParam(param)) {
        String name = request->getParam(param)->value();
        preferences.putString(param.c_str(), name);
      }
    }

    preferences.end();

    // Send a page that triggers restart
    String html = R"rawliteral(
      <html>
      <head>
        <meta charset="UTF-8">
        <meta http-equiv="refresh" content="5">
        <title>Restarting</title>
        <style>
          body {
            font-family: 'Segoe UI', sans-serif;
            background-color: #c9dcf1;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            height: 100vh;
          }
          h2 {
            color: #333;
          }
        </style>
      </head>
      <body>
        <h2>Configuration Saved!</h2>
        <p>Restarting in 3 seconds...</p>
      </body>
      </html>
    )rawliteral";

    request->send(200, "text/html", html);

    // Restart the device after 3 seconds
    request->redirect("/");
    delay(3000);
    ESP.restart();
  });

}

#endif
