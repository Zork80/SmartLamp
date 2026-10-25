#include "Themes.h"
#include "Globals.h"
#include "LedUtils.h"
#include "Twinkles.h"
#include "Persistence.h"

// Local variables for themes
int counter = 0;
bool forward = true;
int waveDelay = 100;

void ThemeOff() {
  fill_solid(_leds, NUMPIXELS, off);
  lampState.isLedOn = false;
  #ifdef DOUBLERELAY
  lampState.isSpotlightOn = true;
  #endif
  delay(20);
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
  delay(20);
}

void ThemeYellow() { fill_solid(_leds, NUMPIXELS, yellow2); delay(20);}
void ThemeYellowWarmWhite() { fill_solid(_leds, NUMPIXELS, warmWhite); delay(20);}
void ThemePickedColor() { fill_solid(_leds, NUMPIXELS, lampState.pickedColor); delay(20);}

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
  delay(20);
}

#ifndef IS_NANO
void ThemeDawn() {
  if (lampState.dawnSecondsGone > 0 && lampState.dawnSecondsGone <= lampState.light_interval_s) {
    lampState.dim = themes[Theme_Dawn].DefaultDim;
    byte dim = 255 - map(lampState.dawnSecondsGone, 0, lampState.light_interval_s, 0, 255);
    _outputString = String("dawnSecondsGone = ") + String(lampState.dawnSecondsGone) + String("; Dim = ") + String(dim);
    _leds.fill_solid(CRGB::White);
    _leds.fadeToBlackBy(dim);
    static int lastLoggedDawnSec = -1;
    if (lampState.dawnSecondsGone != lastLoggedDawnSec && lampState.dawnSecondsGone % 10 == 0) {
      lastLoggedDawnSec = lampState.dawnSecondsGone;
      TRACE(_outputString);
    }
  } else {
    _outputString = "";
    fill_solid(_leds, NUMPIXELS, off);
    #ifdef HASPIR     
    if(lampState.ledTheme != lampState.neutralTheme) { setLedTheme(lampState.neutralTheme); saveState(); }
    #else
    if(lampState.ledTheme != Theme_Off) { setLedTheme(Theme_Off); }
    #endif
  }
}
#endif

void ThemeTwinkle() { 
  if (!lampState.isThemeActive) {
    chooseNextColorPalette(gTargetPalette);
    gCurrentPalette = gTargetPalette;
  }
  Twinkles(_leds); 
  delay(20);
}

#ifndef IS_NANO
void ThemeDusk() {
  if (lampState.duskSecondsGone > 0 && lampState.duskSecondsGone <= lampState.light_interval_s) {
    lampState.dim = themes[Theme_Dusk].DefaultDim;
    byte dim = map(lampState.duskSecondsGone, 0, lampState.light_interval_s, 0, 255);
    _outputString = String("duskSecondsGone = ") + String(lampState.duskSecondsGone) + String("; Dim = ") + String(dim);
    _leds.fill_solid(yellow);
    _leds.fadeToBlackBy(dim);
    static int lastLoggedDuskSec = -1;
    if (lampState.duskSecondsGone != lastLoggedDuskSec && lampState.duskSecondsGone % 10 == 0) {
      lastLoggedDuskSec = lampState.duskSecondsGone;
      TRACE(_outputString);
    }
  } else {
    _outputString = "";
    fill_solid(_leds, NUMPIXELS, off);
    #ifdef HASPIR     
    if(lampState.ledTheme != lampState.neutralTheme) { setLedTheme(lampState.neutralTheme); saveState(); }
    #else
    if(lampState.ledTheme != Theme_Off) { setLedTheme(Theme_Off); }
    #endif
  }
}
#endif

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

void ThemeConfetti() {
  if (!lampState.isThemeActive) fill_solid(_leds, NUMPIXELS, off);
  fadeToBlackBy(_leds, NUMPIXELS, 10);
  int pos = random16(NUMPIXELS);
  _leds[pos] += CHSV(counter + random8(64), 200, 255);
  counter++;
  delay(20);
}

void ThemeSinelon() {
  if (!lampState.isThemeActive) fill_solid(_leds, NUMPIXELS, off);
  fadeToBlackBy(_leds, NUMPIXELS, 20);
  int pos = beatsin16(13, 0, NUMPIXELS - 1);
  _leds[pos] += CHSV(counter, 255, 192);
  counter++;
  delay(10);
}

void ThemeJuggle() {
  if (!lampState.isThemeActive) fill_solid(_leds, NUMPIXELS, off);
  fadeToBlackBy(_leds, NUMPIXELS, 20);
  byte dothue = 0;
  for (int i = 0; i < 8; i++) {
    _leds[beatsin16(i + 7, 0, NUMPIXELS - 1)] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
  delay(10);
}

void ThemeWitchesBrew() {
  #ifdef DOUBLERELAY
  lampState.isSpotlightOn = false; // Spot off for maximum spooky ceiling atmosphere
  #endif

  // 1. Base cauldron potion: Rich, vibrant witch purple / magenta-violet
  // Using higher hue (210-226) so red LEDs are strong and vibrant (not cold blue!)
  uint16_t noiseSpeed = millis() / 14;
  for (int i = 0; i < NUMPIXELS; i++) {
    // Spatial noise creates smooth swirling gradients across the ceiling
    uint8_t n = inoise8(i * 45, noiseSpeed);
    // Rich Hexen-Violett: Hue 212 to 228, full saturation 255
    uint8_t hue = 212 + scale8(n, 16); 
    // Powerful brightness: 110 to 220 (plenty of light thrown onto the ceiling)
    uint8_t bri = map(n, 0, 255, 110, 220);
    _leds[i] = CHSV(hue, 255, bri);
  }

  // 2. Toxic Green Bubbles that emerge, grow, pop, and dissolve into the potion
  struct Bubble {
    int pos;          // LED index
    uint8_t stage;    // 0 = inactive, >0 active timer
    uint8_t maxStage; // Total lifetime in frames
    uint8_t maxBri;   // Peak brightness
  };
  static Bubble bubbles[4] = {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}};

  // Spawn new bubble (~20% chance per frame for active bubbling)
  if (random8(100) < 20) {
    for (int b = 0; b < 4; b++) {
      if (bubbles[b].stage == 0) {
        bubbles[b].pos = random16(NUMPIXELS);
        bubbles[b].stage = 1;
        bubbles[b].maxStage = random8(16, 32); // Snappy ~320ms to 640ms bubble pop
        bubbles[b].maxBri = random8(220, 255); // Full power
        break;
      }
    }
  }

  // Render & advance active bubbles using blend() instead of += to keep toxic green pure!
  for (int b = 0; b < 4; b++) {
    if (bubbles[b].stage > 0) {
      uint8_t progress = map(bubbles[b].stage, 1, bubbles[b].maxStage, 0, 255);
      
      // Fast attack, bright pop, and decay
      uint8_t bBri = 0;
      if (progress < 140) {
        // Swelling and popping
        bBri = map(progress, 0, 140, 60, bubbles[b].maxBri);
      } else {
        // Rapid dissolving
        bBri = map(progress, 140, 255, bubbles[b].maxBri, 0);
      }

      // Pure Toxic Slime Green (Hue 96) and Chartreuse/Lime Pop core (Hue 84)
      CRGB toxicGreen = (progress > 100 && progress < 160) ? CHSV(84, 255, 255) : CHSV(96, 255, 255);

      int center = bubbles[b].pos;
      int left   = (center - 1 + NUMPIXELS) % NUMPIXELS;
      int right  = (center + 1) % NUMPIXELS;

      // OVERWRITE / BLEND: cleanly replaces the purple with pure toxic green without mixing into cyan!
      _leds[center] = blend(_leds[center], toxicGreen, bBri);
      _leds[left]   = blend(_leds[left],   toxicGreen, scale8(bBri, 150));
      _leds[right]  = blend(_leds[right],  toxicGreen, scale8(bBri, 150));

      bubbles[b].stage++;
      if (bubbles[b].stage >= bubbles[b].maxStage) {
        bubbles[b].stage = 0;
      }
    }
  }

  delay(20);
}

void ThemeRainbow() {
  counter = (counter + 1) % 256;
  float offset = 256.0 / STAGES;
  CRGB rb[STAGES];
  for (int i = 0; i < STAGES; i++) rb[i] = Wheel((int)(counter + i * offset) % 256);
  for (int i = STAGES - 1; i >= 0; i--) {
    for(int j = 0; j < PIXELSPERSTAGE; j++) _leds[i*PIXELSPERSTAGE + j] = rb[i];
  }
  delay(20);
}

const byte themeCount = Theme_Count;
ThemeEntry themes[themeCount] = {  
  #ifdef ROOFLIGHT
  { ThemeYellowPlusSpot,  false, 1.0, "Yellow + Spot", "Gelb + Strahler" },
  #else
  { ThemeOff,             false, 0.0, "Off", "Aus" },
  #endif                                      
  { ThemeYellow,          false, 1.0, "Yellow", "Gelb" },
  { ThemeYellowWarmWhite, false, 1.0, "Bright", "Hell" },
  { ThemePickedColor,     false, 0.5, "Selection", "Wahl" },
  { ThemeNightLight,      true,  0.1, "Night Light", "Nachtlicht" },
  { ThemeTwinkle,         true,  0.8, "Twinkle", "Glitzern" },
  { ThemeFire,            true,  0.5, "Fire", "Feuer" },
  #ifdef IS_NANO
  // Dawn/Dusk not available on Nano
  #else
  { ThemeDawn,            true,  1.0, "Dawn", "Morgendämmerung" },
  { ThemeDusk,            true,  1.0, "Dusk", "Abenddämmerung" },
  #endif
  { ThemeWave,            true,  0.5, "Wave", "Welle" },
  { ThemeRainbow,         true,  0.5, "Rainbow", "Regenbogen" },
  { ThemeConfetti,        true,  0.8, "Confetti", "Konfetti" },
  { ThemeSinelon,         true,  0.8, "Sinelon", "Sinelon" },
  { ThemeJuggle,          true,  0.8, "Juggle", "Jonglieren" },
  { ThemeWitchesBrew,     true,  1.0, "Witches' Brew", "Hexenkessel" }
};

void setLedTheme(Theme ledTheme) {
  lampState.ledTheme = ledTheme;
  if (ledTheme >= 0 && ledTheme < themeCount) {
    lampState.dim = themes[ledTheme].DefaultDim;
  }
  lampState.isThemeActive = false;
}

void setLed(Theme ledTheme) {
  // Debug: Check if setLed even notices the change
  static byte debugLastInputTheme = 255;
  if (ledTheme != debugLastInputTheme) {
    TRACE("setLed input changed: " + String(debugLastInputTheme) + " -> " + String(ledTheme));
    debugLastInputTheme = ledTheme;
  }

  Theme currentTheme = ledTheme;

  lampState.isSettingTheme = true;
  #ifndef IS_NANO
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

  // Dawn/Dusk logic only for ESP32
  if (lampState.dawnDays[weekday]) {
    lampState.dawnSecondsGone = ((hour - lampState.dawn_hour) * 60 + (minute - lampState.dawn_minute)) * 60 + second;
    if (trace) TRACE("Dawn: " + String(lampState.dawn_hour) + ":" + String(lampState.dawn_minute) + " -> Gone: " + String(lampState.dawnSecondsGone));
    if (lampState.dawnSecondsGone > 0 && lampState.dawnSecondsGone <= lampState.light_interval_s + 5)      if (currentTheme != lampState.dawnTheme) {
        if (lampState.dawnSecondsGone <= lampState.light_interval_s) TRACE("Switching to Dawn Theme");
        if(lampState.ledTheme != lampState.neutralTheme) {
          lampState.ledTheme = lampState.neutralTheme;
        }
        currentTheme = lampState.dawnTheme;
      }
  }
  if (lampState.duskDays[weekday]) {
    lampState.duskSecondsGone = ((hour - lampState.dusk_hour) * 60 + (minute - lampState.dusk_minute)) * 60 + second;
    if (trace) TRACE("Dusk: " + String(lampState.dusk_hour) + ":" + String(lampState.dusk_minute) + " -> Gone: " + String(lampState.duskSecondsGone));
    if (lampState.duskSecondsGone > 0 && lampState.duskSecondsGone <= lampState.light_interval_s + 5)
      if (currentTheme != lampState.duskTheme) {
        if (lampState.duskSecondsGone <= lampState.light_interval_s) TRACE("Switching to Dusk Theme");
        if(lampState.ledTheme != lampState.neutralTheme) {
          lampState.ledTheme = lampState.neutralTheme;
        }
        currentTheme = lampState.duskTheme;
      }
  }
  #endif // !IS_NANO
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
  
  // Apply global brightness (important for MQTT dimmer)
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
