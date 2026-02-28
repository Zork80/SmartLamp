#ifndef TRACING_H
#define TRACING_H

#include <Arduino.h>

// Forward declaration of the function that performs the actual logging
extern void mqttLog(String text);

#ifdef DEBUG
  // In DEBUG mode, log to Serial and MQTT
  #define TRACE(x) { if(Serial) Serial.println(x); mqttLog(String(x)); }
#else
  // In RELEASE mode, do nothing
  #define TRACE(x) {}
#endif

#endif