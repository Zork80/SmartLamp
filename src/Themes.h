#ifndef THEMES_H
#define THEMES_H

#include <Arduino.h>

enum Theme : byte {
    // The order must match the themes array in Themes.cpp
    Theme_Off,            // 0 //YellowPlusSpot for rooflight is "Off"
    Theme_Yellow,         // 1
    Theme_Bright,         // 2
    Theme_Selection,      // 3
    Theme_NightLight,     // 4
    Theme_Twinkle,        // 5
    Theme_Fire,           // 6
    #if !defined(IS_NANO)
    Theme_Dawn,           // 7
    Theme_Dusk,           // 8
    #endif 
    Theme_Wave,           // 9
    Theme_Rainbow,        // 10
    Theme_Confetti,       // 11
    Theme_Sinelon,        // 12
    Theme_Juggle,         // 13
    Theme_Count
};

typedef void (* ThemeFP)();

struct ThemeEntry {
  ThemeFP Function;
  bool IsDynamic;
  float DefaultDim;
  String Name;
  String Name_de;
};

extern const byte themeCount; // = Theme_Count
extern ThemeEntry themes[];

void setLedTheme(Theme ledTheme);
void setLed(Theme ledTheme);

#endif
