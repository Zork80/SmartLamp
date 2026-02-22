#include "WebHandlers.h"
#include "Globals.h"
#include "Persistence.h"
#include "Themes.h"
#include <ArduinoJson.h>

void handle_rest(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  TRACE("handle_rest: Method=" + String(request->methodToString()) + ", len=" + String(len) + ", index=" + String(index) + ", total=" + String(total));
  if (index == 0 && len == total) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, data, len);    
    if (error) {
      TRACE("JSON Error: " + String(error.c_str()));
      request->send(400, F("text/html"), "Error in parsing json body! <br>" + String(error.c_str()));
      return;
    }
    JsonObject postObj = doc.as<JsonObject>();
    if (request->method() == HTTP_POST) {
      if (postObj["hour_dawn"].is<int>()) lampState.dawn_hour = (int)postObj["hour_dawn"];
      if (postObj["minute_dawn"].is<int>()) lampState.dawn_minute = (int)postObj["minute_dawn"];
      if (postObj["hour_dusk"].is<int>()) lampState.dusk_hour = (int)postObj["hour_dusk"];
      if (postObj["minute_dusk"].is<int>()) lampState.dusk_minute = (int)postObj["minute_dusk"];
      if (postObj["brightness"].is<int>()) {
        int b = postObj["brightness"];
        lampState.dim = (float)b / 255.0;
        // Bei statischen Themes Update erzwingen, damit Helligkeit übernommen wird
        if ((byte)lampState.ledTheme < themeCount && !themes[lampState.ledTheme].IsDynamic) lampState.isThemeActive = false;
      }
      if (postObj["theme"].is<int>()) {
        if((int)lampState.ledTheme != (int)postObj["theme"]) setLedTheme((int)postObj["theme"]);
      }
      if (postObj["color"].is<uint32_t>()) {
        lampState.isThemeActive = false;
        lampState.pickedColor = (uint32_t)postObj["color"];
      }
      if (postObj["do_dawn"].is<int>()) unpackDays((byte)postObj["do_dawn"], lampState.dawnDays);
      if (postObj["do_dusk"].is<int>()) unpackDays((byte)postObj["do_dusk"], lampState.duskDays);
      
      if (postObj["ssid"].is<String>()) wifiSsid = postObj["ssid"].as<String>();
      if (postObj["password"].is<String>()) wifiPassword = postObj["password"].as<String>();
      if (postObj["hostname"].is<String>()) hostname = postObj["hostname"].as<String>();
      
      saveState();
    }
    JsonDocument getObject = getJsonData();
    char buffer[512];
    serializeJson(getObject, buffer);
    request->send(200, "text/json", buffer);
  } else if (index + len == total) {
    TRACE("Error: Payload fragmented or too large (end of stream)");
    request->send(400, "text/plain", "Payload too large or fragmented");
  } else {
    TRACE("Warning: Payload fragment received (ignored)");
  }
}

void handle_read(AsyncWebServerRequest *request) {
  JsonDocument getObject = getJsonData();
  char buffer[256];
  serializeJson(getObject, buffer);
  request->send(200, "text/json", buffer);
}

void handle_read_config(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  TRACE("handle_read_config: len=" + String(len) + ", index=" + String(index) + ", total=" + String(total));
  if (index == 0 && len == total) {
    String input = "";
    if (len >= 2) {
      char tmp[3];
      memcpy(tmp, data, 2);
      tmp[2] = 0;
      input = String(tmp);
    }
    JsonDocument doc;
    for(int i =0; i < themeCount; i++) {
      if(input == "de") doc.add(themes[i].Name_de);
      else doc.add(themes[i].Name);
    }
    char buffer[1024]; // Buffer vergrößert
    serializeJson(doc, buffer);
    request->send(200, "text/json", buffer);
  }
}

void handle_debug(AsyncWebServerRequest *request) {
  String outputString = _outputString;
  _outputString = String("");
  request->send(200, "text/plain", outputString);
}

void handle_NotFound(AsyncWebServerRequest *request) {
  TRACE("NOT FOUND: " + request->url());
  request->send(404, "text/plain", "Not found");
}
