#ifndef TRACING_H
#define TRACING_H

#include <Arduino.h>

#ifdef DEBUG
  #ifdef IS_NANO
    // For Nano, just log to Serial
    #define TRACE(x) { if(Serial) Serial.println(x); }
  #else
    // For ESP32, log to Serial and MQTT
    extern void mqttLog(String text); // Forward declaration for ESP32
    #define TRACE(x) { if(Serial) Serial.println(x); mqttLog(String(x)); }
  #endif
#else
  // In RELEASE mode, do nothing
  #define TRACE(x) {}
#endif

#endif