//#define DEBUG

#include <WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include "ESPAsyncWebSrv.h"
#include <FastLED.h>
#include "time.h"
#include "AsyncJson.h"
#include <ArduinoJson.h>

#include <EEPROM.h>
#include "SPIFFS.h"

#include "net_config.h"

//#define ROOFLIGHT
#define ISSMARTLAMP

#ifdef ISSMARTLAMP
  //#define LOADTHEME
#endif

#ifdef ROOFLIGHT
  #define HASRCSWITCH
  #define HASSPOTLIGHT

  #define HASPIR
  #define RELAYPIN 27
  #define RELAYPIN2 14
  #define DOUBLERELAY
  #define PIRPIN 32
#endif

#define SIGNALPIN 12

#ifdef HASSPOTLIGHT
  bool isSpotlightOn = true;
  bool isLedOn = true;
#else
  bool isLedOn = true;
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

#ifdef ISSMARTLAMP
  #define PIXELSPERSTAGE 24
  //#define PIXELSPERSTAGE 12
  #define STAGES 1
#else
  #define PIXELSPERSTAGE 36
  #define STAGES 1
#endif
#define NUMPIXELS PIXELSPERSTAGE * STAGES

byte _ledTheme = 0;
byte _ledThemeLast = 0;
unsigned long receiveTime = 0;

// Hier wird die Anzahl der angeschlossenen WS2812 LEDs bzw. NeoPixel angegeben
CRGBArray<NUMPIXELS> _leds;
#ifdef ROOFLIGHT
//Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, SIGNALPIN, NEO_BRG + NEO_KHZ800);
#endif
#ifdef ISSMARTLAMP
//Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, SIGNALPIN, NEO_GRB + NEO_KHZ800);
#endif

// Add your networks credentials here
#ifndef SSID
  #define SSID "ssid"
#endif

#ifndef PASSWORD
  #define PASSWORD "password"
#endif

// Set web server port number to 80
#ifndef PORT
  #define PORT 80
#endif


CRGB off = CRGB::Black;
CRGB red = CRGB(255, 0, 0);
CRGB magenta = CRGB(255, 0, 255);
CRGB blue = CRGB(0, 0, 255);
CRGB blueishwhite = CRGB(40, 40, 200);
CRGB greenishwhite = CRGB(40, 200, 40);
CRGB white = CRGB(255, 255, 255);
CRGB turkis = CRGB(0, 255, 255);
CRGB yellow = CRGB(255, 255, 0);
CRGB yellow2 = CRGB(255, 150, 0);
CRGB orange = CRGB(171, 54, 0);
CRGB asWarmWhite = CRGB(243, 231, 211);

CRGB warmWhite = CRGB(123, 124, 52);

float dim = 0.2;
CRGB warmWhiteDark = CRGB(123 * dim, 124 * dim, 52 * dim);

CRGB fireColor = CRGB(80,  18,  0);

CRGB waveColorDark = CRGB(0,  20,  117);
CRGB waveColorAct = CRGB(0,  20,  117);

CRGB pickedColor = CRGB(0, 0, 255);

CRGBPalette16 gCurrentPalette;
CRGBPalette16 gTargetPalette;


int counter = 0;

#ifdef HASPIR
unsigned long lastActivationTime;
unsigned long activationDuration = 180;
#endif

AsyncWebServer server(PORT);

const char* ntpServer = "de.pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

bool dawnDays[7] = {true, true, true, true, true, false, false};
int dawn_hour = 6;
int dawn_minute = 0;
int light_interval = 60;
int light_interval_s = light_interval * 60 ;  
  
bool duskDays[7] = {true, true, true, true, true, true, true};
int dusk_hour = 22;
int dusk_minute = 0;

bool forward = true;
int waveDelay = 100;

bool isThemeActive = false;
bool isSettingTheme = false;
bool firstAfterSwitch = true;

int dawnTheme = 7;
int duskTheme = 8;
#ifdef HASPIR
int neutralTheme = 4;
#else
int neutralTheme = 0;
#endif
int dawnSecondsGone = 0;
int duskSecondsGone = 0;

struct tm timeinfo;

String _outputString = String("");

typedef void (* ThemeFP)();

struct ThemeEntry
{
  ThemeFP Function;
  bool IsDynamic;
  String Name;
  String Name_de;
};

void ThemeOff()
{
  fill_solid(_leds, NUMPIXELS, off);
  isLedOn = false;
  #ifdef DOUBLERELAY
  isSpotlightOn = true;
  #endif
}

void ThemeFire()
{
  if (!isThemeActive) 
  {
      fill_solid(_leds, NUMPIXELS, off);
  }
  for (int i = 0; i < NUMPIXELS; i++)
  {
    AddColor(i, fireColor);
    int r = random(80);
    CRGB diff_color = CRGB(r, r / 2, r / 2);
    SubstractColor(i, diff_color);
  }

  delay(random(50, 150));
}

void ThemeYellowPlusSpot()
{
  fill_solid(_leds, NUMPIXELS, yellow2);
  #ifdef DOUBLERELAY
  isSpotlightOn = true;
  #endif    
}

void ThemeYellow()
{
  fill_solid(_leds, NUMPIXELS, yellow2);
}

void ThemeYellowWarmWhite()
{
  fill_solid(_leds, NUMPIXELS, warmWhite);
}

void ThemeNightLight()
{
  #ifdef HASPIR
  bool doSwitchOff = false;
  unsigned long actTime = millis() / 1000;
  if(activationDuration < actTime - lastActivationTime)
  {
    lastActivationTime = actTime- activationDuration;
    doSwitchOff = true;
  }
  unsigned long activationTimeLeft = activationDuration - (actTime - lastActivationTime);

  _outputString = String("Seconds to switch off: ") + String(activationTimeLeft);
  if(activationTimeLeft > 0) 
  {      
    fill_solid(_leds, NUMPIXELS, warmWhiteDark);
    isLedOn = true;
  }
  else 
  {
    fill_solid(_leds, NUMPIXELS, off);
    //isLedOn = false;
  }
  #else
  fill_solid(_leds, NUMPIXELS, warmWhiteDark);
  #endif
}

void ThemePickedColor()
{
  fill_solid(_leds, NUMPIXELS, pickedColor);
}

void ThemeDawn()
{
  if (dawnSecondsGone > 0 && dawnSecondsGone < light_interval_s)
  {
    float dimFactor = light_interval_s / 255.0;
    byte dim = 255 - constrain(dawnSecondsGone / dimFactor, 0, 255);
    _outputString = String("dawnSecondsGone = ") + String(dawnSecondsGone) + String("; Dim = ") + String(dim);
      
    _leds.fill_solid(CRGB::White);
    _leds.fadeToBlackBy(dim);
  }
  else
  {
    fill_solid(_leds, NUMPIXELS, off);
    
    #ifdef HASPIR     
    if(_ledTheme != 5) {
      _ledTheme = 5;
      saveState();
    }
    #endif
  }
}

void ThemeTwinkle()
{
  Twinkles();
}

void ThemeDusk()
{
  if (duskSecondsGone > 0 && duskSecondsGone < light_interval_s)
  {
    float dimFactor = 255.0 / light_interval_s;
    byte dim = constrain(duskSecondsGone * dimFactor, 0, 255);
    _outputString = String("duskSecondsGone = ") + String(duskSecondsGone) + String("; Dim = ") + String(dim);

    _leds.fill_solid(yellow);
    _leds.fadeToBlackBy(dim);
  }
  else
  {
    fill_solid(_leds, NUMPIXELS, off);
    #ifdef HASPIR     
    if(_ledTheme != 5) {
      _ledTheme = 5;
      saveState();
    }
    #endif
  }
}

void ThemeWave()
{
  if (!isThemeActive) {
    waveColorAct = waveColorDark;
    fill_solid(_leds, NUMPIXELS, waveColorAct);

    waveDelay = random(10, 300);
  }

  uint8_t r, g, b; 
  r = (uint8_t)waveColorAct.r,
  g = (uint8_t)waveColorAct.g,
  b = (uint8_t)waveColorAct.b;

  if(forward)
  {
    r++;
    g++;

    if(r >= 20) {
      forward = false;
      waveDelay = random(10, 300);
    }
  }
  else{
    r--;
    g--;      

    if(r <= 0) {
      forward = true;
      waveDelay = random(10, 300);
    }
  }
  waveColorAct = CRGB(r, g, b);
  
  fill_solid(_leds, NUMPIXELS, waveColorAct);
  
  delay(waveDelay);
}

void ThemeRainbow()
{
  counter++;
  counter = counter % 256;

  float offset = 256.0 / STAGES;
  CRGB rb[STAGES];

  for (int i = 0; i < STAGES; i++)
  {
    rb[i] = Wheel((int)(counter + i * offset) % 256);
  }

  int j = 0;  
  for (int i = STAGES - 1; i >= 0; i--)
  {
    for(int j = 0; j < PIXELSPERSTAGE; j++)
    {
      _leds[i*PIXELSPERSTAGE + j] = rb[i];
    }
    //_leds(i++ * PIXELSPERSTAGE, i * PIXELSPERSTAGE - 1).fill_solid(rb[i]);
  }
}

void ThemeAlert()
{
    fill_solid(_leds, NUMPIXELS, off);

    int delayFactor = 4;

    CRGB color1;
    CRGB color2;

    switch (counter / delayFactor)
    {
      case 0:
        color1 = red;
        color2 = blue;
        break;
      case 1:
        color1 = blue;
        color2 = red;
        break;
    }

    fill_solid(_leds, NUMPIXELS, color1);

    counter++;
    counter = counter % (2 * delayFactor);
}

const byte themeCount = 11;

ThemeEntry themes[themeCount] =
{  
  #ifdef ROOFLIGHT
  { ThemeYellowPlusSpot,  false, "Yellow + Spot", "Gelb + Strahler" },    // 0
  //{ ThemeOff,             false, "Spot", "Strahler" },
  //{ ThemeYellowPlusSpot,  false, "Yellow + Spot", "Gelb + Strahler" },
  #else
  { ThemeOff,             false, "Off", "Aus" },                          // 0
  //{ ThemeOff,             false, "Off", "Aus" },
  //{ ThemeTwinkle,         true,  "Twinkle", "Glitzern" },
  #endif
                                                
  { ThemeYellow,          false, "Yellow", "Gelb" },                      // 1
  { ThemeYellowWarmWhite, false, "Bright", "Hell" },                      // 2
  { ThemePickedColor,     false, "Selection", "Wahl" },                   // 3
  { ThemeNightLight,      true,  "Night Light", "Nachtlicht" },           // 4  
  { ThemeTwinkle,         true,  "Twinkle", "Glitzern" },                 // 5
  { ThemeFire,            true,  "Fire", "Feuer" },                       // 6
  { ThemeDawn,            true,  "Dawn", "Morgendämmerung" },             // 7
  { ThemeDusk,            true,  "Dusk", "Abenddämmerung" },              // 8
  { ThemeWave,            true,  "Wave", "Welle" },                       // 9
  { ThemeRainbow,         true,  "Rainbow", "Regenbogen" }                // 10
};

// https://randomnerdtutorials.com/esp32-ntp-timezones-daylight-saving/
void setTimezone(String timezone){
  Serial.printf("  Setting Timezone to %s\n",timezone.c_str());
  setenv("TZ",timezone.c_str(),1);  //  Now adjust the TZ.  Clock settings are adjusted to show the new local time
  tzset();
}

void setup()
{
  pinMode(SIGNALPIN, OUTPUT);

  #ifdef ROOFLIGHT
  FastLED.addLeds<WS2811, SIGNALPIN, BRG>(_leds, NUMPIXELS);
  #endif
  #ifdef ISSMARTLAMP
  FastLED.addLeds<WS2811, SIGNALPIN, GRB>(_leds, NUMPIXELS);
  #endif
  chooseNextColorPalette(gTargetPalette);

  #ifdef ROOFLIGHT
  pinMode(RELAYPIN, OUTPUT);
  //Relais entsprechend isLedOn schalten
  if (isLedOn) 
  {
    digitalWrite(RELAYPIN , LOW);
  }
  else 
  {
    digitalWrite(RELAYPIN, HIGH);    
  }    
  delay(20);
  #endif

  #ifdef DEBUG
  Serial.begin(115200);
  Serial.print("setup() running on core ");
  Serial.println(xPortGetCoreID());
  #endif
    
  #ifdef DOUBLERELAY          
    pinMode(RELAYPIN2, OUTPUT);
    #ifdef HASSPOTLIGHT
      if(isSpotlightOn)
        digitalWrite(RELAYPIN2 , LOW);
    #endif   
  #endif

  setLed(_ledTheme);

  if (!EEPROM.begin(512)) // size in Byte
  {
    #ifdef DEBUG
    Serial.println("failed to initialise EEPROM");
    #endif
  }
  loadState();

  if (!SPIFFS.begin(true)) {
    #ifdef DEBUG
    Serial.println("An Error has occurred while mounting SPIFFS");
    #endif
  }

  // Connect to Wi-Fi network with SSID and password
  #ifdef DEBUG
  Serial.print("Connecting to ");
  Serial.println(SSID);
  #endif
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    #ifdef DEBUG
    Serial.println("Connection Failed! Rebooting...");
    #endif
    delay(5000);
    ESP.restart();
  }

  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
#ifdef ISSMARTLAMP
  ArduinoOTA.setHostname("SmartLampFlorian");
#else
  ArduinoOTA.setHostname("SmartRoofLamp");
#endif

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      SPIFFS.end();
      #ifdef DEBUG
      Serial.println("Start updating " + type);
      #endif
    })
    .onEnd([]() {
      #ifdef DEBUG
      Serial.println("\nEnd");
      #endif
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      #ifdef DEBUG
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
      #endif
    })
    .onError([](ota_error_t error) {
      #ifdef DEBUG
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
      #endif
    });

  ArduinoOTA.begin();

  //init and get the time
  //configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  configTime(0, 0, ntpServer);
  setTimezone("CET-1CEST,M3.5.0,M10.5.0/3"); //Timezone Berlin

  // Print local IP address and start web server
  #ifdef DEBUG
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  #endif

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/SmartLamp.html", "text/html");
  });

  server.on("/jquery-3.5.1.min.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/jquery-3.5.1.min.js", "text/javascript");
  });

  server.on("/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/bootstrap.min.css", "text/css");
  });

  server.on("/bootstrap.bundle.min.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/bootstrap.bundle.min.js", "text/javascript");
  });

/*
  server.on("/material.min.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/material.min.css", "text/css");
  });

  server.on("/material.min.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/material.min.js", "text/javascript");
  });
*/

  server.on("/SmartLamp.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/SmartLamp.js", "text/javascript");
  });

  server.on("/SmartLamp.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/SmartLamp.css", "text/css");
  });

  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/favicon.ico", "image/ico");
  });

  server.on("/rest", HTTP_POST, [](AsyncWebServerRequest *request){
    //nothing and dont remove it
  }, NULL, handle_rest);

  server.on("/read", HTTP_GET, handle_read);

  server.on("/readconfig", HTTP_POST, [](AsyncWebServerRequest *request){
  }, NULL, handle_read_config);

  server.on("/debug", HTTP_GET, handle_debug);
    
  server.onNotFound(handle_NotFound);

  server.begin();

  #ifdef HASPIR
  pinMode(PIRPIN, INPUT);
  #endif

  #ifdef HASRCSWITCH
    pinMode(RCReveivePin, INPUT);
    mySwitch.enableReceive(digitalPinToInterrupt(RCReveivePin));  // Receiver on interrupt 0 => that is pin #2
  #endif
}

byte timeCounter = 0;
unsigned long previousMillis = 0;
unsigned long interval = 30000;
void loop()
{
    unsigned long currentMillis = millis();
    //int wifi_retry = 0;
    while(WiFi.status() != WL_CONNECTED && (currentMillis - previousMillis >= interval)) {
      //wifi_retry++;
      delay(100);
      #ifdef DEBUG
      Serial.println("WiFi not connected. Try to reconnect.");
      #endif
      WiFi.disconnect();
      WiFi.reconnect();
      previousMillis = currentMillis;
    }
    //if(WiFi.status() != WL_CONNECTED && wifi_retry >= 5) {
    //  #ifdef DEBUG
    //  Serial.println("Reboot");
    //  #endif
    //  ESP.restart();
    //}
    
    ArduinoOTA.handle();
  
    #ifdef HASRCSWITCH
    if (mySwitch.available()) {
      
      unsigned long receiveTimeAct = millis();
      if (receiveTime <= 0 || receiveTimeAct - receiveTime > 500) {
  
        byte ledTheme = _ledTheme;
        receiveTime = receiveTimeAct;
        
        if (mySwitch.getReceivedValue() == onValue)
        {
          ledTheme++;
  
          if (ledTheme == dawnTheme) {
            ledTheme = duskTheme + 1;
          }
          if (ledTheme >= themeCount) {
            ledTheme = 1;
          }
          setLedTheme(ledTheme);
  
          #ifdef DEBUG
          Serial.print("LED Theme = ");
          Serial.println(ledTheme);
          _outputString = String("LED Theme = ") + String(ledTheme);
          #endif
        } 
        else if (mySwitch.getReceivedValue() == offValue) 
        {      
          ledTheme = 0;
          setLedTheme(ledTheme);
  
          #ifdef DEBUG
          Serial.println("LED aus");
          #endif
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
        Serial.println("PIR");
        _outputString = String("PIR");
      }    
    
      lastActivationTime = millis() / 1000;
    }
    #endif
    
    setLed(_ledTheme);
    delay(100);

    if (timeCounter > 10)
    {
      timeCounter = 0;
      struct tm timeinfo_tmp;
      if (!getLocalTime(&timeinfo_tmp)) {
        #ifdef DEBUG
        Serial.println("Failed to obtain time");        
        #endif
        return;
      }
      
      else {
        timeinfo = timeinfo_tmp;
        /*
        char str[40];
        strftime(str, sizeof str, "%A, %B %d %Y %H:%M:%S zone %Z %z", &timeinfo); 
        _outputString = String(str);
        */
      }
      
    }
    timeCounter++;
}


StaticJsonDocument<160> getJsonData() {
  StaticJsonDocument<160> getObject;
  getObject["hour_dawn"] = dawn_hour;
  getObject["minute_dawn"] = dawn_minute;
  getObject["hour_dusk"] = dusk_hour;
  getObject["minute_dusk"] = dusk_minute;
  getObject["theme"] = _ledTheme;
  getObject["color"] = uint32_t(0x00000000) |
               (uint32_t(pickedColor.r) << 16) |
               (uint32_t(pickedColor.g) << 8) |
               uint32_t(pickedColor.b);
  getObject["do_dawn"] = packDays(dawnDays);
  getObject["do_dusk"] = packDays(duskDays);

  return getObject;
}

void handle_rest(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  #ifdef DEBUG
  Serial.print(F("REST HTTP Method: "));
  Serial.println(request->method());
  #endif
  
  DynamicJsonDocument doc(512);
  DeserializationError  error = deserializeJson(doc, (const char*)data);    
  if (error) {
    // if the file didn't open, print an error:
    #ifdef DEBUG
    Serial.print(F("Error parsing JSON "));
    Serial.println(error.c_str());
    #endif
    
    String msg = error.c_str();

    request->send(400, F("text/html"),
                "Error in parsing json body! <br>" + msg);

  } else {
    JsonObject postObj = doc.as<JsonObject>();


    if (request->method() == HTTP_POST) {
      Serial.println("got POST");
      if (postObj.containsKey("hour_dawn")) {
        #ifdef DEBUG
        Serial.print("set hour dawn to ");
        Serial.println((int)postObj["hour_dawn"]);
        #endif
        dawn_hour = (int)postObj["hour_dawn"];
      }
      if (postObj.containsKey("minute_dawn")) {
        #ifdef DEBUG
        Serial.print("set minute dawn to ");
        Serial.println((int)postObj["minute_dawn"]);
        #endif
        dawn_minute = (int)postObj["minute_dawn"];
      }
      if (postObj.containsKey("hour_dusk")) {
        #ifdef DEBUG
        Serial.print("set hour dusk to ");
        Serial.println((int)postObj["hour_dusk"]);
        #endif
        dusk_hour = (int)postObj["hour_dusk"];
      }
      if (postObj.containsKey("minute_dusk")) {
        #ifdef DEBUG
        Serial.print("set minute dusk to ");
        Serial.println((int)postObj["minute_dusk"]);
        #endif
        dusk_minute = (int)postObj["minute_dusk"];
      }
      
      if (postObj.containsKey("theme")) {
        #ifdef DEBUG
        Serial.print("set theme to ");
        Serial.println((int)postObj["theme"]);
        #endif
        if(_ledTheme != (int)postObj["theme"]) {
          setLedTheme((int)postObj["theme"]);
        }
      }

      if (postObj.containsKey("color")) {
        #ifdef DEBUG
        Serial.print("set picked color ");
        Serial.println((uint32_t)postObj["color"], HEX);
        #endif
        isThemeActive = false;
        pickedColor = (uint32_t)postObj["color"];
      }

      if (postObj.containsKey("do_dawn")) {
        #ifdef DEBUG
        Serial.print("set do dawn ");
        Serial.println((bool)postObj["do_dawn"]);
        #endif
        byte doDawn = (byte)postObj["do_dawn"];
        unpackDays(doDawn, dawnDays);
      }
      if (postObj.containsKey("do_dusk")) {
        #ifdef DEBUG
        Serial.print("set do dusk ");
        Serial.println((bool)postObj["do_dusk"]);
        #endif
        byte doDusk = (byte)postObj["do_dusk"];
        unpackDays(doDusk, duskDays);
      }

      saveState();
    }
    
    StaticJsonDocument<160> getObject = getJsonData();
    char buffer[160];
    serializeJson(getObject, buffer);
    //Serial.println(buffer);
    
    request->send(200, "text/json", buffer);
  }
}

void handle_read(AsyncWebServerRequest *request) {
  StaticJsonDocument<160> getObject = getJsonData();

  char buffer[160];
  serializeJson(getObject, buffer);
  //Serial.println(buffer);
  
  request->send(200, "text/json", buffer);
}

void handle_read_config(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  String input = String((const char*)data).substring(0,2);

  Serial.println(input);
  
  String themeNamesTmp[themeCount];
  for(int i = 0; i < themeCount; i++)
  {
    if(input == "de")
      themeNamesTmp[i] = themes[i].Name_de;
    else
      themeNamesTmp[i] = themes[i].Name;

  }

  DynamicJsonDocument doc(1024);
  for(int i =0; i < themeCount; i++)
    doc.add(themeNamesTmp[i]);

  char buffer[160];
  serializeJson(doc, buffer);
  //Serial.println(buffer);
  
  request->send(200, "text/json", buffer);
}

void handle_debug(AsyncWebServerRequest *request) {
  String outputString = _outputString;
  _outputString = String("");
  request->send(200, "text/plain", String(outputString));
}

void handle_NotFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}


void setLedTheme(int ledTheme) {
  //while(isSettingTheme) ;
  _ledTheme = ledTheme;
  isThemeActive = false;
}

void setLed(byte ledTheme) {
  isSettingTheme = true;
  int second;
  int minute;
  int hour;
  int day;
  int month;
  int year;
  int weekday;

  second = timeinfo.tm_sec;
  minute = timeinfo.tm_min;
  hour = timeinfo.tm_hour;
  day = timeinfo.tm_mday;
  month = timeinfo.tm_mon + 1;
  year = timeinfo.tm_year + 1900;
  weekday = (timeinfo.tm_wday + 6) % 7; //days since Monday

  // Check if the current day is set for dawn or dusk
  bool doDawnToday = dawnDays[weekday];
  bool doDuskToday = duskDays[weekday];

  if (doDawnToday) {
    dawnSecondsGone = ((hour - dawn_hour) * 60 + (minute - dawn_minute)) * 60 + second;
    if (dawnSecondsGone > 0 && dawnSecondsGone < light_interval_s)
      if (ledTheme != dawnTheme)
      {
        if(_ledTheme != neutralTheme)
        {
          _ledTheme = neutralTheme;
          //saveState();
        }
        ledTheme = dawnTheme;
      }
  }
  if (doDuskToday) {
    duskSecondsGone = ((hour - dusk_hour) * 60 + (minute - dusk_minute)) * 60 + second;
    if (duskSecondsGone > 0 && duskSecondsGone < light_interval_s)
      if (ledTheme != duskTheme)
      {
        if(_ledTheme != neutralTheme)
        {
          _ledTheme = neutralTheme;
          //saveState();
        }
        ledTheme = duskTheme;
      }
  }
  if(_ledThemeLast != ledTheme)
  {
    _ledThemeLast = ledTheme;
    isThemeActive = false;
  }

  if(isThemeActive && !themes[ledTheme].IsDynamic && !firstAfterSwitch)
    return;

  isLedOn = true;

  //LED-Theme setzen
  #ifdef DOUBLERELAY
  isSpotlightOn = false;
  #endif

  themes[ledTheme].Function();

  firstAfterSwitch = false;
  #ifdef ROOFLIGHT
  //Relais entsprechend isLedOn schalten
  if (isLedOn) {
    digitalWrite(RELAYPIN , LOW);
    if(!isThemeActive)
      firstAfterSwitch = true;
  }
  else {
    digitalWrite(RELAYPIN, HIGH);    
  }    
  #endif
  FastLED.show();

  #ifdef DOUBLERELAY          
  if (isSpotlightOn) {
    digitalWrite(RELAYPIN2 , LOW);
  }
  else {
    digitalWrite(RELAYPIN2, HIGH);    
  }
  #endif
  
  isThemeActive = true;
  isSettingTheme = false;
}

///
/// Set color of LED
///
void AddColor(uint8_t position, CRGB color)
{
  _leds[position] += color;
}

///
/// Set color of LED
///
void SubstractColor(uint8_t position, CRGB color)
{
  _leds[position] -= color;
}

///
/// Color blending
///
CRGB Blend(CRGB color1, CRGB color2)
{
  uint8_t r1, g1, b1;
  uint8_t r2, g2, b2;
  uint8_t r3, g3, b3;

  r1 = (uint8_t)color1.r,
  g1 = (uint8_t)color1.g,
  b1 = (uint8_t)color1.b;

  r2 = (uint8_t)color2.r,
  g2 = (uint8_t)color2.g,
  b2 = (uint8_t)color2.b;

  return CRGB(constrain(r1 + r2, 0, 255), constrain(g1 + g2, 0, 255), constrain(b1 + b2, 0, 255));
}

///
/// Color blending
///
CRGB Substract(CRGB color1, CRGB color2)
{
  return color1 - color2;
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
CRGB Wheel(byte WheelPos)
{
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85)
  {
    return CRGB(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  else if (WheelPos < 170)
  {
    WheelPos -= 85;
    return CRGB(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  else
  {
    WheelPos -= 170;
    return CRGB(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
}

byte packDays(bool weekDays[]) {
  byte packedDays = 0;
  for (int i = 0; i < 7; i++) {
    packedDays |= (weekDays[i] << i);
  }
  return packedDays;
}

void unpackDays(byte packedDays, bool *weekDays) {
  for (int i = 0; i < 7; i++) {
    weekDays[i] = ((packedDays >> i) & 1) > 0;
  }
}
 
void saveState() {
  #ifdef DEBUG
  Serial.println("Save state");
  //_outputString = String("Save state");
  #endif
  uint8_t r;
  uint8_t g;
  uint8_t b;
  r = (uint8_t)pickedColor.r,
  g = (uint8_t)pickedColor.g,
  b = (uint8_t)pickedColor.b;

  byte packedDawnDays =  packDays(dawnDays);
  byte packedDuskDays =  packDays(duskDays);
  
  EEPROM.write(0, (byte)_ledTheme);
  EEPROM.write(1, r);
  EEPROM.write(2, g);
  EEPROM.write(3, b);
  EEPROM.write(4, packedDawnDays);
  EEPROM.write(5, packedDuskDays);
  EEPROM.write(6, (byte)dawn_hour);
  EEPROM.write(7, (byte)dawn_minute);
  EEPROM.write(8, (byte)dusk_hour);
  EEPROM.write(9, (byte)dusk_minute);
  EEPROM.commit();
  #ifdef DEBUG
  Serial.println("Save state done");
  //_outputString = String("Save state done");
  #endif
}


void loadState() {
  #ifdef DEBUG
  Serial.println("Load state");  
  _outputString = String("Load state");
  #endif
  #ifdef LOADTHEME
  setLedTheme(EEPROM.read(0));
  #else
  setLedTheme(0);
  #endif
  uint8_t r = EEPROM.read(1);
  uint8_t g = EEPROM.read(2);
  uint8_t b = EEPROM.read(3);
  unpackDays(EEPROM.read(4), dawnDays);
  unpackDays(EEPROM.read(5), duskDays);
  dawn_hour = EEPROM.read(6);
  dawn_minute = EEPROM.read(7);
  dusk_hour = EEPROM.read(8);
  dusk_minute = EEPROM.read(9);
  pickedColor = CRGB(r, g, b);
  #ifdef DEBUG
  Serial.println("Load state done");
  _outputString = String("Load state done");
  #endif

  #ifdef DEBUG
  StaticJsonDocument<160> getObject;
  getObject["hour_dawn"] = dawn_hour;
  getObject["minute_dawn"] = dawn_minute;
  getObject["hour_dusk"] = dusk_hour;
  getObject["minute_dusk"] = dusk_minute;
  getObject["theme"] = _ledTheme;
  getObject["color"] = uint32_t(0x00000000) |
               (uint32_t(pickedColor.r) << 16) |
               (uint32_t(pickedColor.g) << 8) |
               uint32_t(pickedColor.b);
  getObject["do_dawn"] = packDays(dawnDays);
  getObject["do_dusk"] = packDays(duskDays);
  char buffer[160];
  serializeJson(getObject, buffer);
  Serial.println(buffer);
  _outputString = String(buffer);
  #endif
}
