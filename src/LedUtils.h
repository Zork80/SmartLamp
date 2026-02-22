#ifndef LEDUTILS_H
#define LEDUTILS_H

#include <FastLED.h>

void AddColor(uint8_t position, CRGB color);
void SubstractColor(uint8_t position, CRGB color);
CRGB Blend(CRGB color1, CRGB color2);
CRGB Substract(CRGB color1, CRGB color2);
CRGB Wheel(byte WheelPos);

#endif
