#include "Persistence.h"
#include "Globals.h"
#include "Themes.h" // For setLedTheme
#include <EEPROM.h>

byte packDays(bool weekDays[]) {
  byte packedDays = 0;
  for (int i = 0; i < 7; i++) {
    packedDays |= (weekDays[i] << i);
  }
  return packedDays;
}

void unpackDays(byte packedDays, bool *weekDays) {
  for (int i = 0; i < 7; i++) {
    weekDays[i] = ((packedDays >> i) & 1) > 0;
  }
}

void writeString(int add, String data) {
  int _size = data.length();
  for (int i = 0; i < _size; i++) {
    EEPROM.write(add + i, data[i]);
  }
  EEPROM.write(add + _size, '\0');
}

String readString(int add, int maxLen) {
  String data = "";
  for (int i = 0; i < maxLen; i++) {
    char c = EEPROM.read(add + i);
    if (c == '\0' || c == 255) break;
    data += c;
  }
  return data;
}

void saveState() {
  TRACE("Save state");
  EEPROM.write(0, (byte)lampState.ledTheme);
  EEPROM.write(1, lampState.pickedColor.r);
  EEPROM.write(2, lampState.pickedColor.g);
  EEPROM.write(3, lampState.pickedColor.b);
  EEPROM.write(4, packDays(lampState.dawnDays));
  EEPROM.write(5, packDays(lampState.duskDays));
  EEPROM.write(6, (byte)lampState.dawn_hour);
  EEPROM.write(7, (byte)lampState.dawn_minute);
  EEPROM.write(8, (byte)lampState.dusk_hour);
  EEPROM.write(9, (byte)lampState.dusk_minute);
  writeString(20, wifiSsid);
  writeString(60, wifiPassword);
  writeString(130, hostname);
  EEPROM.commit();
}

void loadState() {
  TRACE("Load state");
  #ifdef LOADTHEME
  setLedTheme((Theme)EEPROM.read(0));
  #else
  setLedTheme(Theme_Off);
  #endif
  uint8_t r = EEPROM.read(1);
  uint8_t g = EEPROM.read(2);
  uint8_t b = EEPROM.read(3);
  unpackDays(EEPROM.read(4), lampState.dawnDays);
  unpackDays(EEPROM.read(5), lampState.duskDays);
  lampState.dawn_hour = EEPROM.read(6);
  lampState.dawn_minute = EEPROM.read(7);
  lampState.dusk_hour = EEPROM.read(8);
  lampState.dusk_minute = EEPROM.read(9);
  lampState.pickedColor = CRGB(r, g, b);
  
  wifiSsid = readString(20, 32);
  wifiPassword = readString(60, 64);
  hostname = readString(130, 32);
  
  if (hostname.length() == 0) {
    hostname = "SmartLamp";
  }
}

JsonDocument getJsonData() {
  JsonDocument getObject;
  getObject["hour_dawn"] = lampState.dawn_hour;
  getObject["minute_dawn"] = lampState.dawn_minute;
  getObject["hour_dusk"] = lampState.dusk_hour;
  getObject["minute_dusk"] = lampState.dusk_minute;
  getObject["theme"] = (byte)lampState.ledTheme;
  getObject["brightness"] = (int)((float)lampState.dim * 255);
  getObject["color"] = uint32_t(0x00000000) |
               (uint32_t(lampState.pickedColor.r) << 16) |
               (uint32_t(lampState.pickedColor.g) << 8) |
               uint32_t(lampState.pickedColor.b);
  getObject["do_dawn"] = packDays(lampState.dawnDays);
  getObject["do_dusk"] = packDays(lampState.duskDays);
  getObject["ssid"] = wifiSsid;
  getObject["hostname"] = hostname;
  // For security reasons, we don't send the password back, or send it empty
  return getObject;
}
