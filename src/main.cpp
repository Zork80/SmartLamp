
// Common includes
#include <FastLED.h>
#include <EEPROM.h>
#include "Globals.h"
#include "Themes.h"
#include "Persistence.h"

#if defined(HAS_WIFI)
  #include <WiFi.h>
  #include <WiFiUdp.h>
  #include <ArduinoOTA.h>
  #include <ESPAsyncWebServer.h>
  #include "SPIFFS.h"
  // Custom modules
  #include "WebHandlers.h"
  #include "MqttHandler.h"
#endif
#ifdef CONFIG_IDF_TARGET_ESP32
  #include "soc/soc.h"
  #include "soc/rtc_cntl_reg.h"
#endif

#ifdef HAS_BUTTONS
  #define BUTTON1 2
  #define BUTTON2 3
  void nextTheme();
  void prevTheme();
#endif

#ifdef HASRCSWITCH
  #include <RCSwitch.h>
  #define RCReveivePin 26
  RCSwitch mySwitch = RCSwitch();
  unsigned long onValueC  = 70737;
  unsigned long offValueC = 70740;
  unsigned long onValueD  = 70929;
  unsigned long offValueD = 70932;
  
  unsigned long onValue = onValueD;
  unsigned long offValue = offValueD;
#endif

#if defined(HASRCSWITCH) || defined(HAS_BUTTONS)
  unsigned long receiveTime = 0;
#endif

#if defined(HAS_WIFI)
AsyncWebServer server(80);
const char* ntpServer = "de.pool.ntp.org";

// https://randomnerdtutorials.com/esp32-ntp-timezones-daylight-saving/
void setTimezone(String timezone){
  TRACE("  Setting Timezone to " + timezone);
  setenv("TZ",timezone.c_str(),1);
  tzset();
}
#endif

void setup()
{
  #ifdef CONFIG_IDF_TARGET_ESP32
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // Disable brownout detector
  #endif

  Serial.begin(MONITOR_SPEED);

  #ifdef IS_NANO
    randomSeed(analogRead(0));
  #endif

  #ifdef HAS_BUTTONS
    pinMode(BUTTON1, INPUT_PULLUP);
    pinMode(BUTTON2, INPUT_PULLUP);
  #endif

  pinMode(LED_PIN, OUTPUT);

  #ifdef IS_NANO
  FastLED.addLeds<WS2812B, LED_PIN, LED_ORDER>(_leds, NUMPIXELS).setCorrection(TypicalLEDStrip);
  #else
  FastLED.addLeds<WS2811, LED_PIN, LED_ORDER>(_leds, NUMPIXELS);
  #endif

  // Limit power consumption (important for USB operation, e.g., to 500mA)
  #ifdef ROOFLIGHT
  // 12V Power Supply for RoofLight (e.g. 4000mA limit for full brightness)
  // FastLED needs the correct voltage (12V) to calculate power consumption correctly.
  FastLED.setMaxPowerInVoltsAndMilliamps(12, 4000); 
  #else
  // Limit power consumption (important for USB operation, e.g., to 500mA)
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 500); 
  #endif
  
  #ifdef ROOFLIGHT
  pinMode(RELAYPIN, OUTPUT);
  if (lampState.isLedOn) digitalWrite(RELAYPIN , LOW);
  else digitalWrite(RELAYPIN, HIGH);    
  delay(20);
  #endif

  #ifdef DOUBLERELAY          
    pinMode(RELAYPIN2, OUTPUT);
    #ifdef HASSPOTLIGHT
      if(lampState.isSpotlightOn) digitalWrite(RELAYPIN2 , LOW);
    #endif   
  #endif

  #ifndef IS_NANO // ESP32 specific EEPROM init
    if (!EEPROM.begin(512)) {
      TRACE("failed to initialise EEPROM");
    }
  #endif

  loadState();
  setLedTheme(lampState.ledTheme);
  TRACE("Starting with Theme: " + String(lampState.ledTheme));

  #ifdef HAS_WIFI
    if (!SPIFFS.begin(true)) {
      TRACE("An Error has occurred while mounting SPIFFS");
    } else {
      TRACE("SPIFFS mounted successfully");
    }
    loadMqttConfig();
    if (wifiSsid.length() > 0) {
      WiFi.mode(WIFI_STA);
      WiFi.begin(wifiSsid.c_str(), wifiPassword.c_str());
      int retries = 0;
      while (WiFi.status() != WL_CONNECTED && retries < 20) {
        delay(500);
        retries++;
      }
    }
    if (WiFi.status() != WL_CONNECTED) {
      isApMode = true;
      WiFi.mode(WIFI_AP);
      WiFi.softAP("SmartLampConfig", "12345678");
      TRACE("WiFi connection failed. Starting AP Mode: SmartLampConfig");
      TRACE("AP IP: " + WiFi.softAPIP().toString());
    } else {
      TRACE("WiFi connected");
      TRACE("Local IP: " + WiFi.localIP().toString());
    }
    ArduinoOTA.setHostname(hostname.c_str());
    ArduinoOTA.onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      TRACE(String("Start updating ") + type);
    });
    ArduinoOTA.onEnd([]() {
      TRACE("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      static int last_progress = -1;
      int current_progress = (progress / (total / 100));
      if (current_progress % 10 == 0 && current_progress != last_progress) {
        last_progress = current_progress;
        TRACE("OTA Progress: " + String(current_progress) + "%");
      }
    });
    ArduinoOTA.onError([](ota_error_t error) {
      String msg = "OTA Error[" + String(error) + "]: ";
      if (error == OTA_AUTH_ERROR) msg += "Auth Failed";
      else if (error == OTA_BEGIN_ERROR) msg += "Begin Failed";
      else if (error == OTA_CONNECT_ERROR) msg += "Connect Failed";
      else if (error == OTA_RECEIVE_ERROR) msg += "Receive Failed";
      else if (error == OTA_END_ERROR) msg += "End Failed";
      TRACE(msg);
    });
    ArduinoOTA.begin();
    configTime(0, 0, ntpServer);
    setTimezone("CET-1CEST,M3.5.0,M10.5.0/3"); // Timezone for Germany

    setupMqtt();
    // Webserver Routes
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ 
      TRACE("GET / requested");
      if (SPIFFS.exists("/SmartLamp.html")) {
        request->send(SPIFFS, "/SmartLamp.html", "text/html"); 
      } else {
        TRACE("Error: /SmartLamp.html not found on SPIFFS!");
        request->send(404, "text/plain", "SmartLamp.html not found. Please upload Filesystem Image.");
      }
    });
    server.on("/jquery-3.5.1.min.js", HTTP_GET, [](AsyncWebServerRequest *request){ request->send(SPIFFS, "/jquery-3.5.1.min.js", "text/javascript"); });
    server.on("/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest *request){ request->send(SPIFFS, "/bootstrap.min.css", "text/css"); });
    server.on("/bootstrap.bundle.min.js", HTTP_GET, [](AsyncWebServerRequest *request){ request->send(SPIFFS, "/bootstrap.bundle.min.js", "text/javascript"); });
    server.on("/SmartLamp.js", HTTP_GET, [](AsyncWebServerRequest *request){ request->send(SPIFFS, "/SmartLamp.js", "text/javascript"); });
    server.on("/SmartLamp.css", HTTP_GET, [](AsyncWebServerRequest *request){ request->send(SPIFFS, "/SmartLamp.css", "text/css"); });
    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){ request->send(SPIFFS, "/favicon.ico", "image/ico"); });

    // MQTT Config Endpoints
    registerMqttWebHandlers(server);
    server.on("/rest", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL, handle_rest);
    server.on("/read", HTTP_GET, handle_read);
    server.on("/readconfig", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL, handle_read_config);
    server.on("/debug", HTTP_GET, handle_debug);
    server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request){ request->send(200, "text/plain", "Rebooting..."); ESP.restart(); });
    server.onNotFound(handle_NotFound);

    server.begin(); 
  #endif // HAS_WIFI

  #ifdef HASPIR
  pinMode(PIRPIN, INPUT);
  #endif

  #ifdef HASRCSWITCH
    pinMode(RCReveivePin, INPUT);
    mySwitch.enableReceive(digitalPinToInterrupt(RCReveivePin));
  #endif
}

#ifdef HAS_WIFI
byte timeCounter = 0;
unsigned long previousMillis = 0;
unsigned long interval = 30000;
#endif

void loop()
{
  #ifdef HAS_WIFI
    if (!isApMode && WiFi.status() != WL_CONNECTED) {
      unsigned long currentMillis = millis();
      if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        WiFi.disconnect();
        WiFi.reconnect();
      }
    } else if (!isApMode && WiFi.status() == WL_CONNECTED) {
      // MQTT Loop and Reconnect
      loopMqtt();
    }
    
    ArduinoOTA.handle();
  #endif // HAS_WIFI

  #ifdef HAS_BUTTONS
    // Handle button presses with debouncing
    unsigned long now = millis();
    if (now - receiveTime > 250) {
      if (digitalRead(BUTTON1) == LOW) {
        receiveTime = now;
        nextTheme();
      } else if (digitalRead(BUTTON2) == LOW) {
        receiveTime = now;
        prevTheme();
      }
    }
  #endif // HAS_BUTTONS

  #ifdef HASRCSWITCH
  if (mySwitch.available()) {
    unsigned long now = millis();
    if (receiveTime == 0 || now - receiveTime > 500) {
      byte currentThemeIndex = lampState.ledTheme;
      receiveTime = now;
      
      if (mySwitch.getReceivedValue() == onValue) {
        currentThemeIndex++;
        // Skip Dawn and Dusk themes when cycling with RC switch
        if (currentThemeIndex == lampState.dawnTheme) currentThemeIndex = lampState.duskTheme + 1;
        if (currentThemeIndex >= themeCount) currentThemeIndex = 1; // Start from 1 to skip "Off"
        setLedTheme((Theme)currentThemeIndex);
        TRACE("RC-Switch: on");
      } else if (mySwitch.getReceivedValue() == offValue) {      
        setLedTheme(Theme_Off); // is YellowPlusSpot for Rooflight
        TRACE("RC-Switch: off");
      } 
      saveState();
    }
    mySwitch.resetAvailable();
  }  
  #endif

  #ifdef HASPIR
    bool detected = digitalRead(PIRPIN);
    if(detected) {
      if(millis() / 1000 - lastActivationTime > 1000){
        _outputString = String("PIR");
      }    
      lastActivationTime = millis() / 1000;
    }
  #endif

  setLed(lampState.ledTheme);

  #ifdef IS_NANO
      // Limit framerate (approx. 50 FPS)
    FastLED.delay(20); 
  #else
    delay(100);
  #endif

  #ifdef HAS_WIFI
    if (timeCounter > 10) {
      timeCounter = 0;
      struct tm timeinfo_tmp;
      if (getLocalTime(&timeinfo_tmp)) {
        timeinfo = timeinfo_tmp;
      }
    }
    timeCounter++;
  #endif
}

#ifdef HAS_BUTTONS
void nextTheme() {
  int next = (int)lampState.ledTheme + 1;
  if (next >= Theme_Count) next = 0;
  setLedTheme((Theme)next);
  saveState();
  TRACE("Theme: " + String(lampState.ledTheme));
}

void prevTheme() {
  int prev = (int)lampState.ledTheme - 1;
  if (prev < 0) prev = Theme_Count - 1;
  setLedTheme((Theme)prev);
  saveState();
  TRACE("Theme: " + String(lampState.ledTheme));
}
#endif