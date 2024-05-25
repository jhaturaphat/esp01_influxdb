#include "router.h"

ESP8266WebServer server(80);

String readFile(const char *path){  
  File file = LittleFS.open(path, "r");
  if(!file){
    Serial.println("Failed to open file");
    return "Failed to open file";
  }
  String html;
  while(file.available()){
    html = file.readString();
  }
  file.close();
  return html;  
}

void handleSave() {
  if (server.hasArg("influxdb_url") && 
      server.hasArg("influxdb_token") && 
      server.hasArg("influxdb_org") && 
      server.hasArg("influxdb_bucket") &&
      server.hasArg("line_token")&&
      server.hasArg("min_temp")&&
      server.hasArg("max_temp")) {

    String influxdb_url = server.arg("influxdb_url");
    String influxdb_token = server.arg("influxdb_token");
    String influxdb_org = server.arg("influxdb_org");
    String influxdb_bucket = server.arg("influxdb_bucket");
    String line_token = server.arg("line_token");
    String min_temp = server.arg("min_temp");
    String max_temp = server.arg("max_temp");    

    // สร้าง JSON object และใส่ข้อมูล
    DynamicJsonDocument doc(1024);
    doc["influxdb_url"] = influxdb_url;
    doc["influxdb_token"] = influxdb_token;
    doc["influxdb_org"] = influxdb_org;
    doc["influxdb_bucket"] = influxdb_bucket;
    doc["line_token"] = line_token;
    doc["min_temp"] = min_temp.toInt();
    doc["max_temp"] = max_temp.toInt();    

    // เปิดไฟล์เพื่อเขียนข้อมูล
    File file = LittleFS.open("/influx.json", "w");
    if (!file) {
      String response = "{ \"status\": \"error\", \"message\": \"Failed to open file for writing\" }";
      server.send(500, "application/json", response);
      return;
    }

    // เขียน JSON ลงในไฟล์
    serializeJson(doc, file);
    file.close();

    String response = "{ \"status\": \"success\" }";
    server.send(200, "application/json", response);
  } else {
    String response = "{ \"status\": \"error\", \"message\": \"Missing parameters\" }";
    server.send(400, "application/json", response);
  }
}

String chipID() {
  String chipID = "ESP_123456";
#ifdef ARDUINO_ARCH_ESP32  //ถ้าเป็น ESP32
  uint32_t chipId = 0;
  for (int i = 0; i < 17; i = i + 8) {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }
  chipID = String(chipId);
#elif defined(ARDUINO_ARCH_ESP8266) //ถ้าเป็น ESP8266
  chipID = String(ESP.getChipId());
#endif
  return chipID;
}

void handleInflux(){
  server.send(200, "application/json", readFile("/influx.json"));
}

void handleRoot() {   
    server.send(200, "text/html", readFile("/index.html"));
}

void handleReboot(){
  server.send(200, "application/json", "ESP rebooting");
  delay(1000);
  ESP.restart();
}

void Router::begin() {
 if(!LittleFS.begin()){
   Serial.println("An Error has occurred while mounting LittleFS");
  //    return;
  }
  server.on("/", handleRoot);
  server.on("/save", HTTP_GET, handleSave);
  server.on("/influx.json", HTTP_GET, handleInflux);
  server.on("/reboot",HTTP_GET, handleReboot);
  server.begin();
}

void Router::wait(){
  server.handleClient();
}
