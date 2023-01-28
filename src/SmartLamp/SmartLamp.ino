//#define DEBUG

#include <WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include "ESPAsyncWebServer.h"
#include <Adafruit_NeoPixel.h>
#include "time.h"
#include "AsyncJson.h"
#include <ArduinoJson.h>

#include <EEPROM.h>
#include "SPIFFS.h"

#define ROOFLIGHT
//#define ISSMARTLAMP

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
  bool isLedOn = false;
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
  #define STAGES 1
#else
  #define PIXELSPERSTAGE 36
  #define STAGES 1
#endif
#define NUMPIXELS PIXELSPERSTAGE * STAGES

byte _ledTheme = 0;
unsigned long receiveTime = -1;

// Hier wird die Anzahl der angeschlossenen WS2812 LEDs bzw. NeoPixel angegeben
#ifdef ROOFLIGHT
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, SIGNALPIN, NEO_BRG + NEO_KHZ800);
#endif
#ifdef ISSMARTLAMP
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, SIGNALPIN, NEO_GRB + NEO_KHZ800);
#endif


uint32_t off = pixels.Color(0, 0, 0);
uint32_t red = pixels.Color(255, 0, 0);
uint32_t magenta = pixels.Color(255, 0, 255);
uint32_t blue = pixels.Color(0, 0, 255);
uint32_t blueishwhite = pixels.Color(40, 40, 200);
uint32_t greenishwhite = pixels.Color(40, 200, 40);
uint32_t white = pixels.Color(255, 255, 255);
uint32_t turkis = pixels.Color(0, 255, 255);
uint32_t yellow = pixels.Color(255, 255, 0);
uint32_t yellow2 = pixels.Color(255, 150, 0);
uint32_t orange = pixels.Color(171, 54, 0);
uint32_t asWarmWhite = pixels.Color(243, 231, 211);

uint32_t warmWhite = pixels.Color(123, 124, 52);

float dim = 0.2;
uint32_t warmWhiteDark = pixels.Color(123 * dim, 124 * dim, 52 * dim);

uint32_t fireColor = pixels.Color ( 80,  18,  0);

uint32_t waveColorDark = pixels.Color ( 0,  20,  117);
uint32_t waveColorAct = pixels.Color ( 0,  20,  117);

uint32_t pickedColor = pixels.Color(0, 0, 255);

int counter = 0;

#ifdef HASPIR
unsigned long lastActivationTime;
unsigned long activationDuration = 180000;
#endif

// Add your networks credentials here
const char* ssid = "SID";
const char* password = "Password";

// Set web server port number to 80
AsyncWebServer server(80);

const char* ntpServer = "de.pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

bool doDawn = false;
int dawn_hour = 6;
int dawn_minute = 0;
int light_interval = 60;

bool doDusk = false;
int dusk_hour = 22;
int dusk_minute = 0;

bool forward = true;
int waveDelay = 100;

bool isDynamicTheme[] = { false, true, false, false, false, true, false, true, true, true, true };
bool isThemeActive = false;
bool isSettingTheme = false;
bool firstAfterSwitch = true;

struct tm timeinfo;

String _outputString = String("");

TaskHandle_t Task1;
TaskHandle_t Task2;

void setup()
{
  pinMode(SIGNALPIN, OUTPUT);

  #ifdef ROOFLIGHT
  pinMode(RELAYPIN, OUTPUT);
  if(!isLedOn)
    digitalWrite(RELAYPIN , LOW);
  else 
    digitalWrite(RELAYPIN , HIGH);
  #endif
  
  #ifdef DOUBLERELAY          
    pinMode(RELAYPIN2, OUTPUT);
    #ifdef HASSPOTLIGHT
      if(isSpotlightOn)
        digitalWrite(RELAYPIN2 , LOW);
    #endif   
  #endif
  
  #ifdef DEBUG
  Serial.begin(115200);
  Serial.print("setup() running on core ");
  Serial.println(xPortGetCoreID());
  #endif

  pixels.begin(); // This initializes the NeoPixel library.

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
  Serial.println(ssid);
  #endif
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
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
  ArduinoOTA.setHostname("SmartLamp");
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
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

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

  xTaskCreatePinnedToCore(
    Task1code, /* Function to implement the task */
    "Task1", /* Name of the task */
    10000,  /* Stack size in words */
    NULL,  /* Task input parameter */
    5,  /* Priority of the task */
    &Task1,  /* Task handle. */
    0); /* Core where the task should run */


  xTaskCreatePinnedToCore(
    Task2code, /* Function to implement the task */
    "Task2", /* Name of the task */
    10000,  /* Stack size in words */
    NULL,  /* Task input parameter */
    1,  /* Priority of the task */
    &Task2,  /* Task handle. */
    1); /* Core where the task should run */      

}

void loop()
{
}

void Task2code( void * pvParameters)
{
  for(;;)
  {
    int wifi_retry = 0;
    while(WiFi.status() != WL_CONNECTED && wifi_retry < 5) {
      wifi_retry++;
      delay(100);
      #ifdef DEBUG
      Serial.println("WiFi not connected. Try to reconnect.");
      #endif
      WiFi.disconnect();
      WiFi.mode(WIFI_OFF);
      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid, password);
    }
    if(WiFi.status() != WL_CONNECTED && wifi_retry >= 5) {
      #ifdef DEBUG
      Serial.println("Reboot");
      #endif
      ESP.restart();
    }
    
    ArduinoOTA.handle();
  
    #ifdef HASRCSWITCH
    if (mySwitch.available()) {
      
      unsigned long receiveTimeAct = millis();
      if (receiveTime < 0 || receiveTimeAct - receiveTime > 500) {
  
        byte ledTheme = _ledTheme;
        receiveTime = receiveTimeAct;
        
        if (mySwitch.getReceivedValue() == onValue)
        {
          ledTheme++;
  
          if (ledTheme == 7) {
            ledTheme = 9;
          }
          if (ledTheme > 10) {
            ledTheme = 1;
          }
          setLedTheme(ledTheme);
  
          #ifdef DEBUG
          Serial.print("LED Theme = ");
          Serial.println(ledTheme);              
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
  }
}

int timeCounter = 0;

void Task1code( void * parameter) { 
  for (;;) {

    #ifdef HASPIR
    bool detected = digitalRead(PIRPIN);
    if(detected) {
      if(millis() - lastActivationTime > 1000){
        Serial.println("PIR");
        _outputString = String("PIR");
      }    
    
      lastActivationTime = millis();
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
      }
    }
    timeCounter++;
  }
}


StaticJsonDocument<160> getJsonData() {
  StaticJsonDocument<160> getObject;
  getObject["hour_dawn"] = dawn_hour;
  getObject["minute_dawn"] = dawn_minute;
  getObject["hour_dusk"] = dusk_hour;
  getObject["minute_dusk"] = dusk_minute;
  getObject["theme"] = _ledTheme;
  getObject["color"] = pickedColor;
  getObject["do_dawn"] = doDawn;
  getObject["do_dusk"] = doDusk;

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
        doDawn = (bool)postObj["do_dawn"];
      }
      if (postObj.containsKey("do_dusk")) {
        #ifdef DEBUG
        Serial.print("set do dusk ");
        Serial.println((bool)postObj["do_dusk"]);
        #endif
        doDusk = (bool)postObj["do_dusk"];
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

void setLedTheme(int ledTheme) {
  //while(isSettingTheme) ;
  _ledTheme = ledTheme;
  isThemeActive = false;
}

void handle_read(AsyncWebServerRequest *request) {
  StaticJsonDocument<160> getObject = getJsonData();

  char buffer[160];
  serializeJson(getObject, buffer);
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

int _ledThemeLast = 0;

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
  weekday = timeinfo.tm_wday + 1;

  int light_interval_s = light_interval * 60 ;  
  int dawnSecondsGone = 0;
  if (doDawn) {
    dawnSecondsGone = ((hour - dawn_hour) * 60 + (minute - dawn_minute)) * 60 + second;
    if (dawnSecondsGone > 0 && dawnSecondsGone < light_interval_s)
      if (ledTheme != 7)
      {      
        #ifdef HASPIR     
        if(_ledTheme != 5) {
          _ledTheme = 5;
          saveState();
        }
        #else
        if(_ledTheme != 0) {
          _ledTheme = 0;
          saveState();
        }
        #endif
        ledTheme = 7;
      }
  }
  int duskSecondsGone = 0;
  if (doDusk) {
    duskSecondsGone = ((hour - dusk_hour) * 60 + (minute - dusk_minute)) * 60 + second;
    if (duskSecondsGone > 0 && duskSecondsGone < light_interval_s)
      if (ledTheme != 8)
      {
        #ifdef HASPIR     
        if(_ledTheme != 5) {
          _ledTheme = 5;
          saveState();
        }
        #else
        if(_ledTheme != 0) {
          _ledTheme = 0;
          saveState();
        }
        #endif
        ledTheme = 8;
      }
  }
  if(_ledThemeLast != ledTheme)
  {
    _ledThemeLast = ledTheme;
    isThemeActive = false;
  }

  if(isThemeActive && !isDynamicTheme[ledTheme] && !firstAfterSwitch)
    return;

  isLedOn = true;

  //LED-Theme setzen
  #ifdef DOUBLERELAY
  isSpotlightOn = false;
  #endif

  if (ledTheme == 0 || _ledThemeLast == 0)
  {
    pixels.fill(off, 0, NUMPIXELS);
  }
  if (ledTheme == 0) 
  {
    isLedOn = false;
    #ifdef DOUBLERELAY
    isSpotlightOn = true;
    #endif
  }
  if (ledTheme == 1) {
    Fire();
  }
  else if (ledTheme == 2) {
    pixels.fill(yellow2, 0, NUMPIXELS);
    #ifdef DOUBLERELAY
    isSpotlightOn = true;
    #endif    
  }
  else if (ledTheme == 3) {
    pixels.fill(yellow2, 0, NUMPIXELS);
  }
  else if (ledTheme == 4) {
    pixels.fill(warmWhite, 0, NUMPIXELS);
  }
  else if (ledTheme == 5) {
    #ifdef HASPIR
    _outputString = String("Millis since last activation: ") + String(millis() - lastActivationTime);
    if(millis() - lastActivationTime < activationDuration) {      
      pixels.fill(warmWhiteDark, 0, NUMPIXELS);
      isLedOn = true;
    }
    else {
      pixels.fill(off, 0, NUMPIXELS);
      isLedOn = false;
    }
    #else
    pixels.fill(warmWhiteDark, 0, NUMPIXELS);
    #endif
  }
  else if (ledTheme == 6) {
    pixels.fill(pickedColor, 0, NUMPIXELS);
  }
  else if (ledTheme == 7) {
    if (dawnSecondsGone > 0 && dawnSecondsGone < light_interval_s)
    {
      int dimFactor = ceil(light_interval * 60 / 255.0);
      int dim = (dawnSecondsGone / dimFactor);
      if (dim > 255)
        dim = 255;
      if (dim < 0)
        dim = 0;
      uint32_t color = pixels.Color(dim, dim, dim);
      pixels.fill(color, 0, NUMPIXELS);
    }
    else
    {
      pixels.fill(off, 0, NUMPIXELS);

      #ifdef HASPIR     
      if(_ledTheme != 5) {
        _ledTheme = 5;
        saveState();
      }
      #endif
    }
  }
  else if (ledTheme == 8) {
    if (duskSecondsGone > 0 && duskSecondsGone < light_interval_s)
    {
      int dimFactor = ceil(light_interval_s / 255.0);
      int dim = ((light_interval_s - duskSecondsGone) / dimFactor);
      if (dim > 255)
        dim = 255;
      if (dim < 0)
        dim = 0;

      uint32_t color = pixels.Color(dim, dim, dim / 4);
      pixels.fill(color, 0, NUMPIXELS);
    }
    else
    {
      pixels.fill(off, 0, NUMPIXELS);
      #ifdef HASPIR     
      if(_ledTheme != 5) {
        _ledTheme = 5;
        saveState();
      }
      #endif
    }
  }
  else if (ledTheme == 9) {
    Wave();
  }
  else if (ledTheme == 10) {
    Rainbow();
  }

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
  pixels.show();
  

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

void Fire()
{
  if (!isThemeActive) {
      pixels.fill(off, 0, NUMPIXELS);
  }
  for (int i = 0; i < NUMPIXELS; i++)
  {
    AddColor(i, fireColor);
    int r = random(80);
    uint32_t diff_color = pixels.Color ( r, r / 2, r / 2);
    SubstractColor(i, diff_color);
  }

  delay(random(50, 150));
}

void Rainbow()
{
  counter++;
  counter = counter % 256;

  float offset = 256.0 / STAGES;
  uint32_t rb[STAGES];

  for (int i = 0; i < STAGES; i++)
  {
    rb[i] = Wheel((int)(counter + i * offset) % 256);
  }

  int j = 0;
  for (int i = STAGES - 1; i >= 0; i--)
  {
    pixels.fill(rb[i], j++ * PIXELSPERSTAGE, PIXELSPERSTAGE);
  }
}

void Wave()
{
  if (!isThemeActive) {
    waveColorAct = waveColorDark;
    pixels.fill(waveColorAct, 0, NUMPIXELS);

    waveDelay = random(10, 300);
  }

  uint8_t r, g, b; 
  r = (uint8_t)(waveColorAct >> 16),
  g = (uint8_t)(waveColorAct >>  8),
  b = (uint8_t)(waveColorAct >>  0);

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
  waveColorAct = pixels.Color (r, g, b);
  
  pixels.fill(waveColorAct, 0, NUMPIXELS);
  
  delay(waveDelay);
}

void Alert()
{
    pixels.fill(off, 0, NUMPIXELS);

    int delayFactor = 4;

    uint32_t color1;
    uint32_t color2;

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

    pixels.fill(color1, 0, NUMPIXELS);

    counter++;
    counter = counter % (2 * delayFactor);
}

///
/// Set color of LED
///
void AddColor(uint8_t position, uint32_t color)
{
  uint32_t blended_color = Blend(pixels.getPixelColor(position), color);
  pixels.setPixelColor(position, blended_color);
}

///
/// Set color of LED
///
void SubstractColor(uint8_t position, uint32_t color)
{
  uint32_t blended_color = Substract(pixels.getPixelColor(position), color);
  pixels.setPixelColor(position, blended_color);
}

///
/// Color blending
///
uint32_t Blend(uint32_t color1, uint32_t color2)
{
  uint8_t r1, g1, b1;
  uint8_t r2, g2, b2;
  uint8_t r3, g3, b3;

  r1 = (uint8_t)(color1 >> 16),
  g1 = (uint8_t)(color1 >>  8),
  b1 = (uint8_t)(color1 >>  0);

  r2 = (uint8_t)(color2 >> 16),
  g2 = (uint8_t)(color2 >>  8),
  b2 = (uint8_t)(color2 >>  0);

  return pixels.Color(constrain(r1 + r2, 0, 255), constrain(g1 + g2, 0, 255), constrain(b1 + b2, 0, 255));
}

///
/// Color blending
///
uint32_t Substract(uint32_t color1, uint32_t color2)
{
  uint8_t r1, g1, b1;
  uint8_t r2, g2, b2;
  uint8_t r3, g3, b3;
  int16_t r, g, b;

  r1 = (uint8_t)(color1 >> 16),
  g1 = (uint8_t)(color1 >>  8),
  b1 = (uint8_t)(color1 >>  0);

  r2 = (uint8_t)(color2 >> 16),
  g2 = (uint8_t)(color2 >>  8),
  b2 = (uint8_t)(color2 >>  0);

  r = (int16_t)r1 - (int16_t)r2;
  g = (int16_t)g1 - (int16_t)g2;
  b = (int16_t)b1 - (int16_t)b2;
  if (r < 0) r = 0;
  if (g < 0) g = 0;
  if (b < 0) b = 0;

  return pixels.Color(r, g, b);
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos)
{
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85)
  {
    return pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  else if (WheelPos < 170)
  {
    WheelPos -= 85;
    return pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  else
  {
    WheelPos -= 170;
    return pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
}

void saveState() {
  return;
  #ifdef DEBUG
  Serial.println("Save state");
  _outputString = String("Save state");
  #endif
  uint8_t r;
  uint8_t g;
  uint8_t b;
  r = (uint8_t)(pickedColor >> 16),
  g = (uint8_t)(pickedColor >>  8),
  b = (uint8_t)(pickedColor >>  0);
  
  EEPROM.write(0, (byte)_ledTheme);
  EEPROM.write(1, r);
  EEPROM.write(2, g);
  EEPROM.write(3, b);
  EEPROM.write(4, doDawn);
  EEPROM.write(5, doDusk);
  EEPROM.write(6, (byte)dawn_hour);
  EEPROM.write(7, (byte)dawn_minute);
  EEPROM.write(8, (byte)dusk_hour);
  EEPROM.write(9, (byte)dusk_minute);
  EEPROM.commit();
  #ifdef DEBUG
  Serial.println("Save state done");
  _outputString = String("Save state done");
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
  doDawn = EEPROM.read(4);
  doDusk = EEPROM.read(5);
  dawn_hour = EEPROM.read(6);
  dawn_minute = EEPROM.read(7);
  dusk_hour = EEPROM.read(8);
  dusk_minute = EEPROM.read(9);
  pickedColor = pixels.Color(r, g, b);
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
  getObject["color"] = pickedColor;
  getObject["do_dawn"] = doDawn;
  getObject["do_dusk"] = doDusk;
  char buffer[160];
  serializeJson(getObject, buffer);
  Serial.println(buffer);
  _outputString = String(buffer);
  #endif
}
