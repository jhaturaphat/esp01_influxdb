#ifndef Router_h
#define Router_h

#include <FS.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#define WiFiClass ESP8266WiFiClass  // กำหนดให้ WiFiClass เป็น ESP8266WiFiClass สำหรับ ESP8266
#elif defined(ESP32)
#include <WiFi.h>
#include <ESPmDNS.h>
#define WiFiClass WiFiClass  // กำหนด WiFiClass สำหรับ ESP32 หมายเหตุสำหรับ esp32
#endif



class Router{
  public:
    void begin();
    void start();
    void modeAP(WiFiClass& wf);
    String chipID();
};


#endif
