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
  
  if (server.hasArg("wifi_ssid") &&
      server.hasArg("wifi_password") &&
      server.hasArg("influxdb_url") && 
      server.hasArg("influxdb_token") && 
      server.hasArg("influxdb_org") && 
      server.hasArg("influxdb_bucket") &&
      server.hasArg("line_token") &&
      server.hasArg("location") &&
      server.hasArg("min_temp") &&
      server.hasArg("max_temp")) {

    String _wifi_ssid        = server.arg("wifi_ssid");
    String _wifi_password    = server.arg("wifi_password");
    String _influxdb_url     = server.arg("influxdb_url");
    String _influxdb_token   = server.arg("influxdb_token");
    String _influxdb_org     = server.arg("influxdb_org");
    String _influxdb_bucket  = server.arg("influxdb_bucket");
    String _line_token       = server.arg("line_token");
    String _location         = server.arg("location");
    String _min_temp         = server.arg("min_temp");
    String _max_temp         = server.arg("max_temp");   

    _wifi_ssid.replace(" ", "");
    _wifi_password.replace(" ", "");
    _influxdb_url.replace(" ", "");
    _influxdb_token.replace(" ", "");
    _influxdb_org.replace(" ", "");
    _influxdb_bucket.replace(" ", "");
    _line_token.replace(" ", "");
    _location.replace(" ", "");
    _min_temp.replace(" ", "");
    _max_temp.replace(" ", "");

    
    // สร้าง JSON object และใส่ข้อมูล
    DynamicJsonDocument doc(1024);
    doc["wifi_ssid"]        = _wifi_ssid;
    doc["wifi_password"]    = _wifi_password;
    doc["influxdb_url"]     = _influxdb_url;
    doc["influxdb_token"]   = _influxdb_token;
    doc["influxdb_org"]     = _influxdb_org;
    doc["influxdb_bucket"]  = _influxdb_bucket;
    doc["line_token"]       = _line_token;
    doc["location"]         = _location;
    doc["min_temp"]         = _min_temp.toInt();
    doc["max_temp"]         = _max_temp.toInt();    

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

    String response = "{ \"status\": \"success\", \"message\":\"success\" }";
    server.send(200, "application/json", response);
  } else {
    String response = "{ \"status\": \"error\", \"message\": \"Missing parameters\" }";
    server.send(400, "application/json", response);
  }
}

void Router::modeAP(WiFiClass& wf){
  // สร้าง Access Point ชื่อ "AutoConnectAP" (ไม่มีรหัสผ่าน)
    //MDNS.addService("http", "tcp", 80);
    wf.mode(WIFI_AP);
    wf.softAP("AP-"+ Router::chipID(), "");
    delay(100);
    Serial.println("Access Point started");
    Router::begin();  //ให้เริ่มการทำงาน
  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }
  
}

String Router::chipID() {
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
  ESP.restart();
}

void Router::begin() {
//  if(!LittleFS.begin()){
//    Serial.println("An Error has occurred while mounting LittleFS");
//   //    return;
//   }
  server.on("/", handleRoot);
  server.on("/save", HTTP_GET, handleSave);
  server.on("/influx.json", HTTP_GET, handleInflux);
  server.on("/reboot",HTTP_GET, handleReboot);
  server.onNotFound(handleRoot);
  server.begin();
}

void Router::wait(){
  server.handleClient();
}
