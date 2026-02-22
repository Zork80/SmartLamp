#ifndef MqttHandler_h
#define MqttHandler_h

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "LampState.h"

void loadMqttConfig();
void setupMqtt();
void loopMqtt();
void registerMqttWebHandlers(AsyncWebServer &server);

#endif