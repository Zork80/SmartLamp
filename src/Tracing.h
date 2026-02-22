#ifndef TRACING_H
#define TRACING_H

#include <Arduino.h>

// Forward-Deklaration der Funktion, die das eigentliche Logging durchführt
extern void mqttLog(String text);

#ifdef DEBUG
  // Im DEBUG-Modus auf Serial und MQTT loggen
  #define TRACE(x) { if(Serial) Serial.println(x); mqttLog(String(x)); }
#else
  // Im RELEASE-Modus nichts tun
  #define TRACE(x) {}
#endif

#endif