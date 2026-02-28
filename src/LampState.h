#ifndef LAMPSTATE_H
#define LAMPSTATE_H

#include <Arduino.h>
#include <FastLED.h>

#include "Tracing.h"
#include "Themes.h"

// Template class that checks if the value has changed on every assignment
// and logs it.
template <typename T>
class Traced {
    T value;
    const char* name;
public:
    Traced(T v, const char* n) : value(v), name(n) {}

    Traced<T>& operator=(const T& v) {
        if (value != v) {
            TRACE(String(name) + " changed: " + String(value) + " -> " + String(v));
            value = v;
        }
        return *this;
    }

    // Implicit conversion to base type (for read access)
    operator T() const { return value; }
    
    // Comparison operators
    bool operator==(const T& other) const { return value == other; }
    bool operator!=(const T& other) const { return value != other; }
};

class LampState {
public:
    Traced<bool> isLedOn{true, "isLedOn"};
    // Ensure isSpotlightOn is present if ROOFLIGHT is defined (independent of Globals.h)
    #if defined(HASSPOTLIGHT) || defined(ROOFLIGHT)
    Traced<bool> isSpotlightOn{true, "isSpotlightOn"};
    #endif

    Traced<Theme> ledTheme{Theme_Off, "ledTheme"};
    Theme ledThemeLast = Theme_Off;
    Traced<Theme> savedTheme{Theme_Yellow, "savedTheme"}; // Saves the theme before switching off
    Traced<bool> isThemeActive{false, "isThemeActive"};
    bool isSettingTheme = false;
    bool firstAfterSwitch = true;

    int dawn_hour = 6;
    int dawn_minute = 0;
    int dusk_hour = 22;
    int dusk_minute = 0;
    bool dawnDays[7] = {true, true, true, true, true, false, false};
    bool duskDays[7] = {true, true, true, true, true, true, true};
    int light_interval = 60;
    int light_interval_s = 3600;

    CRGB pickedColor = CRGB(0, 0, 255);
    Traced<float> dim{0.2, "dim"};

    Theme dawnTheme = Theme_Dawn;
    Theme duskTheme = Theme_Dusk;
    #ifdef HASPIR
    Theme neutralTheme = Theme_NightLight;
    #else
    Theme neutralTheme = Theme_Off;
    #endif
    int dawnSecondsGone = 0;
    int duskSecondsGone = 0;
};

#endif