#line 1 "I:\\VS_Code_Programs\\AI_Assistant_Agent\\ESP32\\main\\webserver_helper.h"
#ifndef WEBSERVER_HELPER_H
#define WEBSERVER_HELPER_H

#include <ESPAsyncWebServer.h>
#include "wifi_helper.h"
#include "storage_helper.h"

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
        background-color: #c9dcf1;
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
        background-color: #e0f0ff;
        padding: 24px;
        border-radius: 16px;
        text-align: left;
        width: 100%;
        box-shadow: 0 4px 10px rgba(0, 0, 0, 0.08);
        box-sizing: border-box;
      }

      input, select {
        font-size: 16px;
        padding: 8px 12px;
        width: 100%;
        margin: 10px 0;
        box-sizing: border-box;
      }

      button, input[type=submit] {
        background-color: #598cc2;
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
}

#endif
