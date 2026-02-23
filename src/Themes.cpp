#include "Themes.h"
#include "Globals.h"
#include "LedUtils.h"
#include "Twinkles.h"
#include "Persistence.h"

// Lokale Variablen für Themes
int counter = 0;
bool forward = true;
int waveDelay = 100;

void ThemeOff() {
  fill_solid(_leds, NUMPIXELS, off);
  lampState.isLedOn = false;
  #ifdef DOUBLERELAY
  lampState.isSpotlightOn = true;
  #endif
}

void ThemeFire() {
  if (!lampState.isThemeActive) fill_solid(_leds, NUMPIXELS, off);
  for (int i = 0; i < NUMPIXELS; i++) {
    AddColor(i, fireColor);
    int r = random(80);
    SubstractColor(i, CRGB(r, r / 2, r / 2));
  }
  delay(random(50, 150));
}

void ThemeYellowPlusSpot() {
  fill_solid(_leds, NUMPIXELS, yellow2);
  #ifdef DOUBLERELAY
  lampState.isSpotlightOn = true;
  #endif    
}

void ThemeYellow() { fill_solid(_leds, NUMPIXELS, yellow2); }
void ThemeYellowWarmWhite() { fill_solid(_leds, NUMPIXELS, warmWhite); }
void ThemePickedColor() { fill_solid(_leds, NUMPIXELS, lampState.pickedColor); }

void ThemeNightLight() {
  #ifdef HASPIR
  unsigned long actTime = millis() / 1000;
  if(activationDuration < actTime - lastActivationTime) {
    lastActivationTime = actTime - activationDuration;
  }
  unsigned long activationTimeLeft = activationDuration - (actTime - lastActivationTime);
  _outputString = String("Seconds to switch off: ") + String(activationTimeLeft);
  if(activationTimeLeft > 0) {      
    fill_solid(_leds, NUMPIXELS, warmWhiteDark);
    lampState.isLedOn = true;
  } else {
    fill_solid(_leds, NUMPIXELS, off);
  }
  #else
  fill_solid(_leds, NUMPIXELS, warmWhiteDark);
  #endif
}

void ThemeDawn() {
  if (lampState.dawnSecondsGone > 0 && lampState.dawnSecondsGone < lampState.light_interval_s) {
    lampState.dim = themes[Theme_Dawn].DefaultDim;
    float dimFactor = lampState.light_interval_s / 255.0;
    byte dim = 255 - constrain(lampState.dawnSecondsGone / dimFactor, 0, 255);
    _outputString = String("dawnSecondsGone = ") + String(lampState.dawnSecondsGone) + String("; Dim = ") + String(dim);
    _leds.fill_solid(CRGB::White);
    _leds.fadeToBlackBy(dim);
    TRACE(_outputString);
  } else {
    fill_solid(_leds, NUMPIXELS, off);
    #ifdef HASPIR     
    if(lampState.ledTheme != lampState.neutralTheme) { setLedTheme(lampState.neutralTheme); saveState(); }
    #endif
  }
}

void ThemeTwinkle() { 
  if (!lampState.isThemeActive) {
    chooseNextColorPalette(gTargetPalette);
    gCurrentPalette = gTargetPalette;
  }
  Twinkles(_leds); 
}

void ThemeDusk() {
  if (lampState.duskSecondsGone > 0 && lampState.duskSecondsGone < lampState.light_interval_s) {
    lampState.dim = themes[Theme_Dusk].DefaultDim;
    float dimFactor = 255.0 / lampState.light_interval_s;
    byte dim = constrain(lampState.duskSecondsGone * dimFactor, 0, 255);
    _outputString = String("duskSecondsGone = ") + String(lampState.duskSecondsGone) + String("; Dim = ") + String(dim);
    _leds.fill_solid(yellow);
    _leds.fadeToBlackBy(dim);
    TRACE(_outputString);
  } else {
    fill_solid(_leds, NUMPIXELS, off);
    #ifdef HASPIR     
    if(lampState.ledTheme != lampState.neutralTheme) { setLedTheme(lampState.neutralTheme); saveState(); }
    #endif
  }
}

void ThemeWave() {
  if (!lampState.isThemeActive) {
    waveColorAct = waveColorDark;
    fill_solid(_leds, NUMPIXELS, waveColorAct);
    waveDelay = random(10, 300);
  }
  if(forward) {
    waveColorAct.r++; waveColorAct.g++;
    if(waveColorAct.r >= 20) { forward = false; waveDelay = random(10, 300); }
  } else {
    waveColorAct.r--; waveColorAct.g--;      
    if(waveColorAct.r <= 0) { forward = true; waveDelay = random(10, 300); }
  }
  fill_solid(_leds, NUMPIXELS, waveColorAct);
  delay(waveDelay);
}

void ThemeRainbow() {
  counter = (counter + 1) % 256;
  float offset = 256.0 / STAGES;
  CRGB rb[STAGES];
  for (int i = 0; i < STAGES; i++) rb[i] = Wheel((int)(counter + i * offset) % 256);
  for (int i = STAGES - 1; i >= 0; i--) {
    for(int j = 0; j < PIXELSPERSTAGE; j++) _leds[i*PIXELSPERSTAGE + j] = rb[i];
  }
}

const byte themeCount = Theme_Count;
ThemeEntry themes[themeCount] = {  
  #ifdef ROOFLIGHT
  { ThemeYellowPlusSpot,  false, 1.0, "Yellow + Spot", "Gelb + Strahler" },
  #else
  { ThemeOff,             false, 0.0, "Off", "Aus" },
  #endif                                      
  { ThemeYellow,          false, 0.5, "Yellow", "Gelb" },
  { ThemeYellowWarmWhite, false, 1.0, "Bright", "Hell" },
  { ThemePickedColor,     false, 0.5, "Selection", "Wahl" },
  { ThemeNightLight,      true,  0.1, "Night Light", "Nachtlicht" },
  { ThemeTwinkle,         true,  0.8, "Twinkle", "Glitzern" },
  { ThemeFire,            true,  0.5, "Fire", "Feuer" },
  { ThemeDawn,            true,  1.0, "Dawn", "Morgendämmerung" },
  { ThemeDusk,            true,  1.0, "Dusk", "Abenddämmerung" },
  { ThemeWave,            true,  0.5, "Wave", "Welle" },
  { ThemeRainbow,         true,  0.5, "Rainbow", "Regenbogen" }
};

void setLedTheme(Theme ledTheme) {
  lampState.ledTheme = ledTheme;
  if (ledTheme >= 0 && ledTheme < themeCount) {
    lampState.dim = themes[ledTheme].DefaultDim;
  }
  lampState.isThemeActive = false;
}

void setLed(Theme ledTheme) {
  // Debug: Prüfen ob setLed überhaupt die Änderung mitbekommt
  static byte debugLastInputTheme = 255;
  if (ledTheme != debugLastInputTheme) {
    TRACE("setLed input changed: " + String(debugLastInputTheme) + " -> " + String(ledTheme));
    debugLastInputTheme = ledTheme;
  }

  Theme currentTheme = ledTheme;

  lampState.isSettingTheme = true;
  int second = timeinfo.tm_sec;
  int minute = timeinfo.tm_min;
  int hour = timeinfo.tm_hour;
  int weekday = (timeinfo.tm_wday + 6) % 7;

  static unsigned long lastTraceTime = 0;
  bool trace = false;
  if (millis() - lastTraceTime > 10000) {
    lastTraceTime = millis();
    trace = true;
    TRACE("Current Time: " + String(hour) + ":" + String(minute) + ":" + String(second));
  }

  if (lampState.dawnDays[weekday]) {
    lampState.dawnSecondsGone = ((hour - lampState.dawn_hour) * 60 + (minute - lampState.dawn_minute)) * 60 + second;
    if (trace) TRACE("Dawn: " + String(lampState.dawn_hour) + ":" + String(lampState.dawn_minute) + " -> Gone: " + String(lampState.dawnSecondsGone));
    if (lampState.dawnSecondsGone > 0 && lampState.dawnSecondsGone < lampState.light_interval_s)      if (currentTheme != lampState.dawnTheme) {
        TRACE("Switching to Dawn Theme");
        if(lampState.ledTheme != lampState.neutralTheme) {
          lampState.ledTheme = lampState.neutralTheme;
        }
        currentTheme = lampState.dawnTheme;
      }
  }
  if (lampState.duskDays[weekday]) {
    lampState.duskSecondsGone = ((hour - lampState.dusk_hour) * 60 + (minute - lampState.dusk_minute)) * 60 + second;
    if (trace) TRACE("Dusk: " + String(lampState.dusk_hour) + ":" + String(lampState.dusk_minute) + " -> Gone: " + String(lampState.duskSecondsGone));
    if (lampState.duskSecondsGone > 0 && lampState.duskSecondsGone < lampState.light_interval_s)
      if (currentTheme != lampState.duskTheme) {
        TRACE("Switching to Dusk Theme");
        if(lampState.ledTheme != lampState.neutralTheme) {
          lampState.ledTheme = lampState.neutralTheme;
        }
        currentTheme = lampState.duskTheme;
      }
  }
  if(lampState.ledThemeLast != currentTheme) {
    TRACE("Theme changed: " + String(lampState.ledThemeLast) + " -> " + String(currentTheme));
    lampState.ledThemeLast = currentTheme;
    lampState.isThemeActive = false;
  }

  if(lampState.isThemeActive && !themes[currentTheme].IsDynamic && !lampState.firstAfterSwitch) return;

  lampState.isLedOn = true;
  #ifdef DOUBLERELAY
  lampState.isSpotlightOn = false;
  #endif

  themes[currentTheme].Function();

  lampState.firstAfterSwitch = false;
  #ifdef ROOFLIGHT
  if (lampState.isLedOn) {
    digitalWrite(RELAYPIN , LOW);
    if(!lampState.isThemeActive) lampState.firstAfterSwitch = true;
  } else {
    digitalWrite(RELAYPIN, HIGH);    
  }    
  #endif
  
  // Globale Helligkeit anwenden (wichtig für MQTT Dimmer)
  FastLED.setBrightness((uint8_t)((float)lampState.dim * 255));
  FastLED.show();

  #ifdef DOUBLERELAY          
  if (lampState.isSpotlightOn) digitalWrite(RELAYPIN2 , LOW);
  else digitalWrite(RELAYPIN2, HIGH);    
  static bool lastSpotlightState = !lampState.isSpotlightOn;
  if (lampState.isSpotlightOn != lastSpotlightState) {
    TRACE("Spotlight: " + String(lampState.isSpotlightOn ? "ON" : "OFF"));
    lastSpotlightState = lampState.isSpotlightOn;
  }
  #endif
  
  lampState.isThemeActive = true;
  lampState.isSettingTheme = false;
}
