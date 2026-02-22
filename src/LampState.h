#ifndef LAMPSTATE_H
#define LAMPSTATE_H

#include <Arduino.h>
#include <FastLED.h>

#include "Tracing.h"

// Template-Klasse, die bei jeder Zuweisung prüft, ob sich der Wert geändert hat
// und dies protokolliert.
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

    // Implizite Konvertierung zum Basis-Typ (für Lesezugriffe)
    operator T() const { return value; }
    
    // Vergleichsoperatoren
    bool operator==(const T& other) const { return value == other; }
    bool operator!=(const T& other) const { return value != other; }
};

class LampState {
public:
    Traced<bool> isLedOn{true, "isLedOn"};
    // Sicherstellen, dass isSpotlightOn da ist, wenn ROOFLIGHT definiert ist (unabhängig von Globals.h)
    #if defined(HASSPOTLIGHT) || defined(ROOFLIGHT)
    Traced<bool> isSpotlightOn{true, "isSpotlightOn"};
    #endif

    Traced<byte> ledTheme{0, "ledTheme"};
    byte ledThemeLast = 0;
    Traced<byte> savedTheme{1, "savedTheme"}; // Speichert das Theme vor dem Ausschalten
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

    int dawnTheme = 7;
    int duskTheme = 8;
    #ifdef HASPIR
    int neutralTheme = 4;
    #else
    int neutralTheme = 0;
    #endif
    int dawnSecondsGone = 0;
    int duskSecondsGone = 0;
};

#endif