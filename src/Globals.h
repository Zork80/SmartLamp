#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include "Tracing.h"
#include <FastLED.h>
#include "time.h"

// --- Configuration ---

#ifdef ROOFLIGHT
  #define HASRCSWITCH
  #define HASSPOTLIGHT
  #define HASPIR
  #define RELAYPIN 27
  #define RELAYPIN2 14
  #define DOUBLERELAY
  #define PIRPIN 32
#endif

#include "LampState.h"

#ifndef PIXELSPERSTAGE
  #define PIXELSPERSTAGE 24
#endif

#ifndef STAGES
  #define STAGES 1
#endif
#define NUMPIXELS (PIXELSPERSTAGE * STAGES)

// --- Global Variables (Declaration) ---
extern CRGBArray<NUMPIXELS> _leds;
extern LampState lampState;

extern String wifiSsid;
extern String wifiPassword;
extern String hostname;
extern bool isApMode;

extern CRGB off;
extern CRGB red;
extern CRGB blue;
extern CRGB yellow;
extern CRGB yellow2;
extern CRGB warmWhite;
extern CRGB warmWhiteDark;
extern CRGB fireColor;
extern CRGB waveColorDark;
extern CRGB waveColorAct;

extern struct tm timeinfo;
extern String _outputString;

#ifdef HASPIR
extern unsigned long lastActivationTime;
extern unsigned long activationDuration;
#endif

#endif
