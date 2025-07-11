#ifndef WIFI_HELPER_H
#define WIFI_HELPER_H

#include <WiFi.h>

const char *ap_ssid = "Smart Light";
const char *ap_password = "12345678";
const char *hostname = "smart-light";

IPAddress local_ip(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

void setupAP() {
  WiFi.setHostname(hostname);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
}

void connectToWiFi(const String &ssid, const String &password) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
  Serial.println("Connecting to WiFi...");

  unsigned long startAttemptTime = millis();
  const unsigned long timeout = 10000; // 10 seconds

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < timeout) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected!");
    Serial.println("IP address: " + WiFi.localIP().toString());
  } else {
    Serial.println("Failed to connect. Falling back to AP mode.");
    setupAP();
  }
}

#endif