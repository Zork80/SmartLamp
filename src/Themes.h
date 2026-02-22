#ifndef THEMES_H
#define THEMES_H

#include <Arduino.h>

typedef void (* ThemeFP)();

struct ThemeEntry {
  ThemeFP Function;
  bool IsDynamic;
  float DefaultDim;
  String Name;
  String Name_de;
};

extern const byte themeCount;
extern ThemeEntry themes[];

void setLedTheme(int ledTheme);
void setLed(byte ledTheme);

#endif
