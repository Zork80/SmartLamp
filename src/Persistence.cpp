#include "Persistence.h"
#include "Globals.h"
#include "Themes.h" // For setLedTheme
#include <EEPROM.h>

#ifndef IS_NANO
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
#endif

#ifndef IS_NANO
static bool isPersistenceDirty = false;
static unsigned long persistenceDirtyTimestamp = 0;
const unsigned long PERSISTENCE_DEBOUNCE_MS = 2500;
#endif

void saveStateDebounced() {
#ifdef IS_NANO
  saveState();
#else
  isPersistenceDirty = true;
  persistenceDirtyTimestamp = millis();
#endif
}

void loopPersistence() {
#ifndef IS_NANO
  if (isPersistenceDirty && (millis() - persistenceDirtyTimestamp >= PERSISTENCE_DEBOUNCE_MS)) {
    isPersistenceDirty = false;
    saveState();
  }
#endif
}

void saveState() {
#ifdef IS_NANO
  EEPROM.put(0, (byte)lampState.ledTheme);
#else
  isPersistenceDirty = false;
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
  EEPROM.write(10, (byte)((float)lampState.dim * 255));
  writeString(20, wifiSsid);
  writeString(60, wifiPassword);
  writeString(130, hostname);
  EEPROM.commit();
#endif
}

void loadState() {
#ifdef IS_NANO
  byte val = EEPROM.read(0);
  if (val < Theme_Count && val != (byte)Theme_Off) {
    lampState.savedTheme = (Theme)val;
  }
  #if defined(RESTORE_THEME_ON_BOOT) || defined(LOADTHEME)
  lampState.ledTheme = (val < Theme_Count) ? (Theme)val : Theme_Off;
  #else
  lampState.ledTheme = Theme_Off;
  #endif
#else
  TRACE("Load state");
  byte themeVal = EEPROM.read(0);
  if (themeVal < Theme_Count && themeVal != (byte)Theme_Off) {
    lampState.savedTheme = (Theme)themeVal;
  }
  #if defined(RESTORE_THEME_ON_BOOT) || defined(LOADTHEME)
  if (themeVal < Theme_Count) {
    setLedTheme((Theme)themeVal);
  } else {
    setLedTheme(Theme_Off);
  }
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
  if (lampState.dawn_hour > 23) lampState.dawn_hour = 6;
  if (lampState.dawn_minute > 59) lampState.dawn_minute = 0;
  if (lampState.dusk_hour > 23) lampState.dusk_hour = 22;
  if (lampState.dusk_minute > 59) lampState.dusk_minute = 0;

  byte dimVal = EEPROM.read(10);
  if (dimVal > 0 && (dimVal != 255 || EEPROM.read(6) <= 23)) {
    lampState.dim = (float)dimVal / 255.0;
  }
  lampState.pickedColor = CRGB(r, g, b);
  
  wifiSsid = readString(20, 32);
  wifiPassword = readString(60, 64);
  hostname = readString(130, 32);
  
  if (hostname.length() == 0) {
    hostname = "SmartLamp";
  }
#endif
}

#ifndef IS_NANO
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
#endif
