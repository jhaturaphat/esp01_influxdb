#include "router.h"

ESP8266WebServer server(80);

bool LoadFromLittleFS(const char* path, const char* contentType) {
  File file = LittleFS.open(path, "r");
  if (!file) {
    Serial.println("Failed to open file");
    return false;
  }

  server.streamFile(file, contentType);
  file.close();
  return true;
}

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

void writeFile(const char *path, const char *message) {
  Serial.printf("Writing file: %s\n", path);

  File file = LittleFS.open(path, "w");
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  delay(2000);  // Make sure the CREATE and LASTWRITE times are different
  file.close();
}

void listDir(const char *dirname) {
  Serial.printf("Listing directory: %s\n", dirname);

  Dir root = LittleFS.openDir(dirname);

  while (root.next()) {
    File file = root.openFile("r");
    Serial.print("  FILE: ");
    Serial.print(root.fileName());
    Serial.print("  SIZE: ");
    Serial.print(file.size());
    time_t cr = file.getCreationTime();
    time_t lw = file.getLastWrite();
    file.close();
    struct tm *tmstruct = localtime(&cr);
    Serial.printf("    CREATION: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
    tmstruct = localtime(&lw);
    Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
  }
}

void handleRoot() { 
  listDir("/");
//  writeFile("/index.html", "<h1>1122334455667788</h> ");
  String html = readFile("/index.html");
    server.send(200, "text/html", html);
}

void Router::begin() {
  if(!LittleFS.begin()){
    Serial.println("An Error has occurred while mounting LittleFS");
//    return;
  }
  server.on("/", handleRoot);
  server.begin();
}

void Router::wait(){
  server.handleClient();
}
