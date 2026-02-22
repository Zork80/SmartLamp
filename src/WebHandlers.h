#ifndef WEBHANDLERS_H
#define WEBHANDLERS_H

#include <ESPAsyncWebServer.h>

void handle_rest(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
void handle_read(AsyncWebServerRequest *request);
void handle_read_config(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
void handle_debug(AsyncWebServerRequest *request);
void handle_NotFound(AsyncWebServerRequest *request);

#endif
