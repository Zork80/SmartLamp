#include "Globals.h"

// --- Shared Global Variable Definitions ---
CRGBArray<NUMPIXELS> _leds;
LampState lampState;

// Definition of palettes for Twinkles.h
CRGBPalette16 gCurrentPalette;
CRGBPalette16 gTargetPalette;

// Colors
CRGB off = CRGB::Black;
CRGB red = CRGB(255, 0, 0);
CRGB blue = CRGB(0, 0, 255);
CRGB yellow = CRGB(255, 255, 0);
CRGB yellow2 = CRGB(255, 150, 0);
CRGB warmWhite = CRGB(123, 124, 52);
// warmWhiteDark gedimmt (ca. 20%), passend zur Logik im Hauptprojekt für Nachtlicht
// Wir nutzen hier Konstanten statt lampState.dim, um Initialisierungsreihenfolge-Probleme zu vermeiden.
CRGB warmWhiteDark = CRGB(123 * 0.2, 124 * 0.2, 52 * 0.2);
CRGB fireColor = CRGB(80, 18, 0);
CRGB waveColorDark = CRGB(0, 20, 117);
CRGB waveColorAct = CRGB(0, 20, 117);

// --- Platform-Specific Global Variable Definitions ---
#ifdef IS_NANO
  // Dummy implementations for Nano to satisfy linker
  String wifiSsid;
  String wifiPassword;
  String hostname;
  bool isApMode = false;
  struct tm timeinfo;
  String _outputString;
#else // IS_ESP32
  // Real implementations for ESP32
  String wifiSsid = "";
  String wifiPassword = "";
  String hostname = "";
  bool isApMode = false;
  struct tm timeinfo;
  String _outputString = String("");
  #ifdef HASPIR
  unsigned long lastActivationTime;
  unsigned long activationDuration = 180;
  #endif
#endif