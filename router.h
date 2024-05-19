#ifndef Router_h
#define Router_h

#include <FS.h>
#include <LittleFS.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

class Router{
  public:
    void begin();
    void wait();
};


#endif
