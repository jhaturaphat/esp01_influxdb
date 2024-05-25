#if defined(ESP32)
#include <WiFi.h>
#define DEVICE "ESP32"
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#define DEVICE "ESP8266"
#endif

#include "FS.h"
#include <LittleFS.h>
#include <time.h>
#include <WiFiManager.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
#include <Adafruit_Sensor.h>
#include <TridentTD_LineNotify.h>
#include <DHT.h>
#include <DHT_U.h>
#include "router.h"
#include "param.h"

#define TZ_INFO "GMT+7"

unsigned long previousMillis_1 = 0;
unsigned long previousMillis_2 = 0;
const long interval_1 = 60000;       // กำหนด interval เป็น 60 วินาที
const long interval_2 = 20000;       // กำหนด interval เป็น 20 วินาที

DHT dht(DHTPIN, DHTTYPE);


// Function prototypes
void checkTemperature();
void checkTime();
void taskAt1600();
void taskAt0000();
void taskAt0800();


// InfluxDB client instance
InfluxDBClient client;
Point sensor("DHT11");

WiFiManager wifiManager;
Router router;
void setup() {  
  Serial.begin(115200);
//  if(!LittleFS.begin()) Serial.println("LittleFS Mount Failed");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Not connected to WiFi. Starting WiFiManager...");
    // เริ่มต้นการตั้งค่า WiFi Manager
    // สร้าง Access Point ชื่อ "AutoConnectAP" (ไม่มีรหัสผ่าน)
    
//    wifiManager.resetSettings();
    wifiManager.setTimeout(180); 
    if (!wifiManager.autoConnect("AutoConnectAP")) {
      Serial.println("Failed to connect and hit timeout");
      delay(3000);
      // รีสตาร์ทอุปกรณ์หลังจากล้มเหลวในการเชื่อมต่อ WiFi
      ESP.restart();
      delay(5000);
    }
  }
  // เมื่อเชื่อมต่อสำเร็จ จะแสดง IP Address
  Serial.println("Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // แสดงค่า SSID ที่บันทึกไว้
  Serial.print("Connected to SSID: ");
  Serial.println(WiFi.SSID());
  delay(10);
  configTime(7 * 3600, 0, "pool.ntp.org", "time.google.com"); // Offset of 7 hours for GMT+7 7 * 3600 = 25200 วินาที.
//  timeSync(TZ_INFO, "pool.ntp.org", "time.google.com");
  Serial.print("Waiting for time");
  while (!time(nullptr)) {
    Serial.print(".");
    delay(1000);
  }
  
  dht.begin();
  
  LINE.setToken(line_token);

  // Check InfluxDB connection
  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }
   
  router.begin();
  // InfluxDB settings
  loadConfig();
  client.setConnectionParams(influxdb_url, influxdb_org, influxdb_bucket, influxdb_token);
}

void loop() {
  unsigned long currentMillis = millis();
  router.wait();    
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print(" °C, Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");
    return;
  }


  if (currentMillis - previousMillis_1 >= interval_1) {
    previousMillis_1 = currentMillis;
    // Add data to InfluxDB
  sensor.clearFields();
  sensor.addField("temperature", temperature);
  sensor.addField("humidity", humidity);
    // Print data to Serial
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" °C, Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");
  // Write data to InfluxDB
  if (!client.writePoint(sensor)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
    }   
  }
  if (currentMillis - previousMillis_2 >= interval_2){
    previousMillis_2 = currentMillis;
    checkTime();
    checkTemperature();    
  }
  
//  delay(30000); // Wait for 30 seconds before sending the next data
}

void checkTemperature() {
  float Temperature = dht.readTemperature();
  Serial.println("checkTemperature()");

  // Check if any reads failed and exit early (to try again).
  if (isnan(Temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    LINE.notify("Failed to read from DHT sensor!");
    return;
  }

  // Check if temperature is out of the range 15-23 degrees Celsius
  if (temperature < min_temp || temperature > max_temp) {    
    LINE.notify(String(location)+"\n🌡 ค่าที่กำหนด"+min_temp+"-"+max_temp+"\nอุณหภูมิขณะนี้ "+temperature+" องศา \n ความชื้นขณะนี้ "+humidity+" %");
  }
}


void checkTime() {
  time_t now = time(nullptr);
  struct tm* timeInfo = localtime(&now);

  // Check if the current time matches the target times
  if (timeInfo->tm_hour == 16 && timeInfo->tm_min == 0 && timeInfo->tm_sec >= 0) {
    taskAt1600();
  }
  if (timeInfo->tm_hour == 0 && timeInfo->tm_min == 0 && timeInfo->tm_sec >= 0) {
    taskAt0000();
  }
  if (timeInfo->tm_hour == 8 && timeInfo->tm_min == 0 && timeInfo->tm_sec >= 0) {
    taskAt0800();
  }
}

void taskAt1600() {
  Serial.println("Task at 16:00 executed.");
  LINE.notify(String(location)+"\nอุณหภูมิขณะนี้ "+temperature+" องศา \nความชื้นขณะนี้ "+humidity+" %");
}

void taskAt0000() {
  Serial.println("Task at 00:00 executed.");
  LINE.notify(String(location)+"\nอุณหภูมิขณะนี้ "+temperature+" องศา \nความชื้นขณะนี้ "+humidity+" %");
}
void taskAt0800() {
  Serial.println("Task at 08:00 executed.");
  LINE.notify(String(location)+"\nอุณหภูมิขณะนี้ "+temperature+" องศา \nความชื้นขณะนี้ "+humidity+" %");
}
