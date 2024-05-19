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
      server.hasArg("influxdb_bucket")) {

    String influxdb_url = server.arg("influxdb_url");
    String influxdb_token = server.arg("influxdb_token");
    String influxdb_org = server.arg("influxdb_org");
    String influxdb_bucket = server.arg("influxdb_bucket");

    // สร้าง JSON object และใส่ข้อมูล
    DynamicJsonDocument doc(1024);
    doc["influxdb_url"] = influxdb_url;
    doc["influxdb_token"] = influxdb_token;
    doc["influxdb_org"] = influxdb_org;
    doc["influxdb_bucket"] = influxdb_bucket;

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

void handleInflux(){
  server.send(200, "application/json", readFile("/influx.json"));
}

void handleRoot() {   
    server.send(200, "text/html", readFile("/index.html"));
}

void Router::begin() {
  if(!LittleFS.begin()){
    Serial.println("An Error has occurred while mounting LittleFS");
//    return;
  }
  server.on("/", handleRoot);
  server.on("/save", HTTP_GET, handleSave);
  server.on("/influx.json", HTTP_GET, handleInflux);
  server.begin();
}

void Router::wait(){
  server.handleClient();
}
