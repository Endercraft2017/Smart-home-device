#line 1 "I:\\VS_Code_Programs\\AI_Assistant_Agent\\ESP32\\main\\storage_helper.h"
#ifndef STORAGE_HELPER_H
#define STORAGE_HELPER_H

#include <Preferences.h>

Preferences preferences;

void saveCredentials(const String &ssid, const String &password) {
  preferences.begin("wifi", false);
  preferences.putString("ssid", ssid);
  preferences.putString("pass", password);
  preferences.end();
  Serial.println("Credentials saved.");
}

bool loadCredentials(String &ssid, String &password) {
  preferences.begin("wifi", true);
  ssid = preferences.getString("ssid", "");
  password = preferences.getString("pass", "");
  preferences.end();
  return ssid.length() > 0 && password.length() > 0;
}

#endif