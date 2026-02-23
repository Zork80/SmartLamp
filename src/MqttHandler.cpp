#include "MqttHandler.h"
#include "Globals.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "SPIFFS.h"
#include "Themes.h"
#include "Persistence.h"

// MQTT Variablen
String mqttServer = "";
#define MQTT_PORT 1883

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Status-Tracking für MQTT Updates
bool lastIsLedOnMqtt = true;
float lastDimMqtt = 0.2;
CRGB lastPickedColorMqtt = CRGB(0,0,255);
Theme lastLedThemeMqtt = Theme_Off;

int lastDawnHour = -1;
int lastDawnMinute = -1;
int lastDuskHour = -1;
int lastDuskMinute = -1;
byte lastDawnDaysPacked = 0;
byte lastDuskDaysPacked = 0;

String getChipId() {
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  return mac;
}

void mqttLog(String text) {
  if (mqttClient.connected()) {
    String topic = "smartlamp/" + getChipId() + "/debug";
    mqttClient.publish(topic.c_str(), text.c_str());
  }
}

void publishState() {
  JsonDocument doc;
#ifdef ROOFLIGHT
  // Für RoofLight ist Theme 1 (Yellow, Spot aus) der "Aus"-Zustand
  doc["state"] = (lampState.ledTheme == Theme_Yellow) ? "OFF" : "ON";
#else
  doc["state"] = (bool)lampState.isLedOn ? "ON" : "OFF";
#endif
  doc["brightness"] = (int)((float)lampState.dim * 255);
  if (lampState.ledTheme < Theme_Count) {
    doc["effect"] = themes[lampState.ledTheme].Name;
  } else {
    doc["effect"] = nullptr;
  }
  JsonObject color = doc["color"].to<JsonObject>();
  color["r"] = lampState.pickedColor.r;
  color["g"] = lampState.pickedColor.g;
  color["b"] = lampState.pickedColor.b;

  char buffer[512];
  serializeJson(doc, buffer);
  String topic = "smartlamp/" + getChipId() + "/state";
  mqttClient.publish(topic.c_str(), buffer, true);
}

void publishConfigState() {
  JsonDocument doc;
  
  char timeBuffer[6];
  sprintf(timeBuffer, "%02d:%02d", lampState.dawn_hour, lampState.dawn_minute);
  doc["dawn"]["time"] = timeBuffer;
  
  JsonArray dawnDays = doc["dawn"]["days"].to<JsonArray>();
  for(int i=0; i<7; i++) dawnDays.add(lampState.dawnDays[i] ? "ON" : "OFF");

  sprintf(timeBuffer, "%02d:%02d", lampState.dusk_hour, lampState.dusk_minute);
  doc["dusk"]["time"] = timeBuffer;

  JsonArray duskDays = doc["dusk"]["days"].to<JsonArray>();
  for(int i=0; i<7; i++) duskDays.add(lampState.duskDays[i] ? "ON" : "OFF");

  char buffer[1024];
  serializeJson(doc, buffer);
  String topic = "smartlamp/" + getChipId() + "/config/state";
  mqttClient.publish(topic.c_str(), buffer, true);
}

void sendDiscovery() {
JsonDocument doc;
  String chipId = getChipId();
  String deviceName = hostname.length() > 0 ? hostname : "SmartLamp " + chipId;

  doc["name"] = deviceName; 
  doc["unique_id"] = "smartlamp_" + chipId;
  doc["cmd_t"] = "smartlamp/" + chipId + "/set";
  doc["stat_t"] = "smartlamp/" + chipId + "/state";
  doc["avty_t"] = "smartlamp/" + chipId + "/availability";
  doc["schema"] = "json";
  doc["brightness"] = true;
  doc["effect"] = true;
  doc["optimistic"] = false; // Wichtig: HA wartet auf Status-Rückmeldung

  // Modern: supported_color_modes statt "rgb": true
  JsonArray colorModes = doc["supported_color_modes"].to<JsonArray>();
  colorModes.add("rgb");

  // Device Registry Information (Identifiers MÜSSEN ein Array sein)
  JsonObject device = doc["device"].to<JsonObject>();
  JsonArray identifiers = device["identifiers"].to<JsonArray>();
  identifiers.add("smartlamp_" + chipId);
  
  device["name"] = deviceName;
  device["model"] = "SmartLamp ESP32";
  device["manufacturer"] = "DIY";
  device["sw_version"] = "1.0";

  JsonArray effectList = doc["effect_list"].to<JsonArray>();
  for(int i=0; i < themeCount; i++) {
    if (themes[i].Name == "Dawn" || themes[i].Name == "Dusk") continue;
    effectList.add(themes[i].Name);
  }

  char buffer[1024];
  serializeJson(doc, buffer);
  
  String topic = "homeassistant/light/smartlamp_" + chipId + "/config";
  mqttClient.publish(topic.c_str(), buffer, true);

  // Discovery für Dawn/Dusk Einstellungen
  const char* days[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
  const char* types[] = {"dawn", "dusk"};
  
  for (int t = 0; t < 2; t++) {
    String type = types[t];
    String typeCap = type; 
    typeCap[0] = toupper(typeCap[0]);

    // Time Entity
    doc.clear();
    doc["name"] = deviceName + " " + typeCap + " Time";
    doc["unique_id"] = "smartlamp_" + chipId + "_" + type + "_time";
    doc["stat_t"] = "smartlamp/" + chipId + "/config/state";
    doc["val_tpl"] = "{{ value_json." + type + ".time }}";
    doc["cmd_t"] = "smartlamp/" + chipId + "/" + type + "/time/set";
    doc["icon"] = "mdi:clock-outline";
    doc["entity_category"] = "config";
    doc["pattern"] = "\\d{2}:\\d{2}";
    doc["max"] = 5;
    
    // Device Info muss für jede Nachricht neu gebaut werden
    JsonObject dev = doc["device"].to<JsonObject>();
    JsonArray ids = dev["identifiers"].to<JsonArray>();
    ids.add("smartlamp_" + chipId);
    dev["name"] = deviceName;
    dev["model"] = "SmartLamp ESP32";
    dev["manufacturer"] = "DIY";
    dev["sw_version"] = "1.0";
    
    String topicTime = "homeassistant/text/smartlamp_" + chipId + "_" + type + "_time/config";
    char bufferTime[1024];
    serializeJson(doc, bufferTime);
    mqttClient.publish(topicTime.c_str(), bufferTime, true);
    delay(10); // Kurze Pause um Netzwerk-Flooding zu vermeiden

    // Day Switches
    for (int i = 0; i < 7; i++) {
      doc.clear();
      doc["name"] = deviceName + " " + typeCap + " " + days[i];
      doc["unique_id"] = "smartlamp_" + chipId + "_" + type + "_" + String(i);
      doc["stat_t"] = "smartlamp/" + chipId + "/config/state";
      doc["val_tpl"] = "{{ value_json." + type + ".days[" + String(i) + "] }}";
      doc["cmd_t"] = "smartlamp/" + chipId + "/" + type + "/" + String(i) + "/set";
      doc["icon"] = "mdi:calendar-check";
      doc["entity_category"] = "config";
      
      JsonObject devSwitch = doc["device"].to<JsonObject>();
      JsonArray idsSwitch = devSwitch["identifiers"].to<JsonArray>();
      idsSwitch.add("smartlamp_" + chipId);
      devSwitch["name"] = deviceName;
      devSwitch["model"] = "SmartLamp ESP32";
      devSwitch["manufacturer"] = "DIY";
      devSwitch["sw_version"] = "1.0";

      String topicSwitch = "homeassistant/switch/smartlamp_" + chipId + "_" + type + "_" + String(i) + "/config";
      char bufferSwitch[1024];
      serializeJson(doc, bufferSwitch);
      mqttClient.publish(topicSwitch.c_str(), bufferSwitch, true);
      delay(10);
    }
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload, length);

  if (error) {
    TRACE("MQTT JSON Error: " + String(error.c_str()));
    return;
  }

  // Trace eingehende Nachricht (Payload in String wandeln für Log)
  char debugBuf[length + 1];
  memcpy(debugBuf, payload, length);
  debugBuf[length] = '\0';
  TRACE("MQTT RX: " + String(debugBuf));

  if (doc.containsKey("state")) {
    const char* state = doc["state"];
    bool on = (strcmp(state, "ON") == 0);
#ifdef ROOFLIGHT
    lampState.isLedOn = true; // Rooflight ist physisch immer an
    
    if (!on) {
      TRACE("MQTT: Switching OFF (Theme Yellow)");
      setLedTheme(Theme_Yellow);
    } else { // state is ON
      // If brightness is sent while the lamp is "OFF" (Theme != 0),
      // we assume the user only wants to dim the ambient light.
      // In this case, we do NOT change the theme to 0 (Spot on).
      bool isDimmingAmbient = doc.containsKey("brightness") && lampState.ledTheme != Theme_YellowPlusSpot;
      // Check if theme will be overwritten by color/effect later in this function
      bool willSetThemeLater = doc.containsKey("color") || doc.containsKey("effect");

      if (isDimmingAmbient) {
        TRACE("MQTT: Brightness change while 'OFF' -> Keeping current Theme");
      } else if (!willSetThemeLater) {
        TRACE("MQTT: Switching ON (Theme 0)");
        setLedTheme(Theme_YellowPlusSpot);
      }
      lampState.isThemeActive = false; // Update erzwingen
      delay(50);
    }
#else
    lampState.isLedOn = on;
    
    if (!on) {
      // Bevor wir ausschalten, speichern wir das aktuelle Theme (wenn es nicht schon Aus ist)
      if (lampState.ledTheme != Theme_Off) {
        lampState.savedTheme = lampState.ledTheme;
      }
      setLedTheme(Theme_Off); // Theme Off ist "Aus"
    } else if (lampState.ledTheme == Theme_Off) {
      // State is ON and current theme is OFF
      // Only restore theme if we are not about to set a color or effect
      if (!doc.containsKey("color") && !doc.containsKey("effect")) {
        if (lampState.savedTheme > 0 && lampState.savedTheme < Theme_Count) {
          setLedTheme(lampState.savedTheme);
        } else {
          // Fallback to a default "on" theme if nothing valid was saved
          setLedTheme(Theme_Yellow);
        }
      }
    }
#endif
    lampState.isThemeActive = false; // Update der LEDs erzwingen
  }

  if (doc.containsKey("brightness")) {
    int b = doc["brightness"];
    lampState.dim = (float)b / 255.0;
    // Bei statischen Themes (wie Theme 0 bei Rooflight) Update erzwingen, damit Helligkeit übernommen wird
    if (lampState.ledTheme < Theme_Count && !themes[lampState.ledTheme].IsDynamic) {
      lampState.isThemeActive = false;
    }
  }

  if (doc.containsKey("color")) {
    lampState.pickedColor.r = doc["color"]["r"];
    lampState.pickedColor.g = doc["color"]["g"];
    lampState.pickedColor.b = doc["color"]["b"];
    // Wenn eine Farbe gewählt wird, schalten wir auf das "Selection" Theme (Index 3)
    setLedTheme(Theme_Selection);
  }

  if (doc.containsKey("effect")) {
    String effect = doc["effect"];
    // Suche den Index anhand des Namens
    for(int i=0; i < Theme_Count; i++) {
      if (themes[i].Name == effect) {
        setLedTheme((Theme)i);
        break;
      }
    }
  }

  // Config Handler (Dawn/Dusk)
  String t = String(topic);
  if (t.indexOf("/dawn/") != -1 || t.indexOf("/dusk/") != -1) {
    char p[length + 1];
    memcpy(p, payload, length);
    p[length] = '\0';
    String val = String(p);

    bool isDawn = (t.indexOf("/dawn/") != -1);
    
    if (t.endsWith("/time/set")) {
      int h, m;
      if (sscanf(p, "%d:%d", &h, &m) == 2) {
        if (isDawn) {
          lampState.dawn_hour = h;
          lampState.dawn_minute = m;
        } else {
          lampState.dusk_hour = h;
          lampState.dusk_minute = m;
        }
        saveState();
        publishConfigState();
      }
    } else {
      // Check for day switch (ends with /0/set ... /6/set)
      for (int i = 0; i < 7; i++) {
        if (t.endsWith("/" + String(i) + "/set")) {
          bool on = (val == "ON");
          if (isDawn) {
            lampState.dawnDays[i] = on;
          } else {
            lampState.duskDays[i] = on;
          }
          saveState();
          publishConfigState();
          break;
        }
      }
    }
    return; // Config handled
  }

  // publishState() hier entfernen! 
  // Die Änderungserkennung in loopMqtt() übernimmt das Senden.
}

void loadMqttConfig() {
  if (SPIFFS.exists("/mqtt.json")) {
    File file = SPIFFS.open("/mqtt.json", "r");
    if (file) {
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, file);
      if (!error && doc.containsKey("mqttServer")) {
        mqttServer = doc["mqttServer"].as<String>();
      }
      file.close();
    }
  }
  if (mqttServer == "") mqttServer = "192.168.178.66"; 
}

void saveMqttConfig() {
  File file = SPIFFS.open("/mqtt.json", "w");
  if (file) {
    JsonDocument doc;
    doc["mqttServer"] = mqttServer;
    serializeJson(doc, file);
    file.close();
  }
}

void reconnectMqtt() {
  if (!mqttClient.connected()) {
    String chipId = getChipId();
    String clientId = "SmartLamp-" + chipId;
    
    // Topics für die Verfügbarkeit
    String availabilityTopic = "smartlamp/" + chipId + "/availability";
    
    // connect(id, user, pass, willTopic, willQos, willRetain, willMessage)
    // Wir setzen den Last Will auf "offline", falls der ESP32 die Verbindung verliert.
    if (mqttClient.connect(clientId.c_str(), nullptr, nullptr, availabilityTopic.c_str(), 1, true, "offline")) { 
      #ifdef DEBUG
      TRACE("MQTT connected");
      #endif

      // Sofort nach dem Verbinden "online" senden
      mqttClient.publish(availabilityTopic.c_str(), "online", true);
      
      // Debug-Nachricht senden, damit das Topic im Explorer sofort sichtbar wird
      mqttLog("MQTT Connected. Debugging active.");

      // Discovery und Abonnements
      sendDiscovery();
      mqttClient.subscribe(("smartlamp/" + chipId + "/set").c_str());
      mqttClient.subscribe(("smartlamp/" + chipId + "/+/+/set").c_str()); // Config Topics
      publishState();
      publishConfigState();
    }
  }
}

void setupMqtt() {
  loadMqttConfig(); // WICHTIG: Erst laden, dann Server setzen!
  
  mqttClient.setServer(mqttServer.c_str(), MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  
  // Optional: Buffer vergrößern für große JSON-Pakete (Discovery + Effekte)
  mqttClient.setBufferSize(1500); 
}

void loopMqtt() {
  if (WiFi.status() == WL_CONNECTED) {
    if (!mqttClient.connected()) {
      static unsigned long lastMqttAttempt = 0;
      if (millis() - lastMqttAttempt > 5000) {
        lastMqttAttempt = millis();
        reconnectMqtt();
      }
    } else {
      mqttClient.loop();
      
      // Check for state changes
      if (lampState.isLedOn != lastIsLedOnMqtt || lampState.ledTheme != lastLedThemeMqtt || lampState.dim != lastDimMqtt || lampState.pickedColor != lastPickedColorMqtt) {
        publishState();
        lastIsLedOnMqtt = lampState.isLedOn;
        lastLedThemeMqtt = lampState.ledTheme;
        lastDimMqtt = lampState.dim;
        lastPickedColorMqtt = lampState.pickedColor;
      }

      // Check for config changes
      byte currentDawnDays = packDays(lampState.dawnDays);
      byte currentDuskDays = packDays(lampState.duskDays);
      
      if (lampState.dawn_hour != lastDawnHour || lampState.dawn_minute != lastDawnMinute ||
          lampState.dusk_hour != lastDuskHour || lampState.dusk_minute != lastDuskMinute ||
          currentDawnDays != lastDawnDaysPacked || currentDuskDays != lastDuskDaysPacked) {
            
        publishConfigState();
        lastDawnHour = lampState.dawn_hour;
        lastDawnMinute = lampState.dawn_minute;
        lastDuskHour = lampState.dusk_hour;
        lastDuskMinute = lampState.dusk_minute;
        lastDawnDaysPacked = currentDawnDays;
        lastDuskDaysPacked = currentDuskDays;
      }
    }
  }
}

void registerMqttWebHandlers(AsyncWebServer &server) {
  server.on("/mqtt_config", HTTP_GET, [](AsyncWebServerRequest *request){
    JsonDocument doc;
    doc["mqttServer"] = mqttServer;
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });
  
  server.on("/mqtt_config", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    JsonDocument doc;
    deserializeJson(doc, data, len);
    if (doc.containsKey("mqttServer")) {
      mqttServer = doc["mqttServer"].as<String>();
      saveMqttConfig();
    }
    request->send(200, "application/json", "{\"status\":\"ok\"}");
  });
}