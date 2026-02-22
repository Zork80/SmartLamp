#include "LedUtils.h"
#include "Globals.h"

void AddColor(uint8_t position, CRGB color) {
  _leds[position] += color;
}

void SubstractColor(uint8_t position, CRGB color) {
  _leds[position] -= color;
}

CRGB Blend(CRGB color1, CRGB color2) {
  return CRGB(constrain(color1.r + color2.r, 0, 255), 
              constrain(color1.g + color2.g, 0, 255), 
              constrain(color1.b + color2.b, 0, 255));
}

CRGB Substract(CRGB color1, CRGB color2) {
  return color1 - color2;
}

CRGB Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return CRGB(255 - WheelPos * 3, 0, WheelPos * 3);
  } else if (WheelPos < 170) {
    WheelPos -= 85;
    return CRGB(0, WheelPos * 3, 255 - WheelPos * 3);
  } else {
    WheelPos -= 170;
    return CRGB(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
}
