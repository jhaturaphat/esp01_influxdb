#if defined(ESP32)
#include <WiFi.h>
#include <ESPmDNS.h>
#define DEVICE "ESP32"
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
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
unsigned long previousMillis_3 = 0;
const long interval_1 = 60000;       // กำหนด interval เป็น 60 วินาที
const long interval_2 = 60000;       // กำหนด interval เป็น 20 วินาที
const long interval_3 = 20000;       // กำหนด interval เป็น 10 วินาที

unsigned long startAttemptTime = 0;
const unsigned long wifiTimeout = 30000; // 10 วินาที (10000 มิลลิวินาที)

DHT dht(DHTPIN, DHTTYPE);


// Function prototypes
void checkTemperature();
void checkTime();
void taskAt1600();
void taskAt0000();
void taskAt0800();
void startAP();



// InfluxDB client instance
InfluxDBClient client;
Point sensor("DHT11");

Router router;

void setup() {  
  Serial.begin(115200); 
  loadConfig();
  startAttemptTime = millis();
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED){
    if (millis() - startAttemptTime >= wifiTimeout) {
      Serial.println("ไม่สามารถเชื่อมต่อ WiFi ได้ เปลี่ยนไปเป็นโหมด AP");
      startAP();
      router.begin();
      return;
    }
    delay(500);
    Serial.print(".");
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
          
          // Check InfluxDB connection
          if (client.validateConnection()) {
            Serial.print("Connected to InfluxDB: ");
            Serial.println(client.getServerUrl());
          } else {
            Serial.print("InfluxDB connection failed: ");
            Serial.println(client.getLastErrorMessage());
          }
          
         
          client.setConnectionParams(influxdb_url, influxdb_org, influxdb_bucket, influxdb_token);

          LINE.setToken(line_token);  
          String IP = WiFi.localIP().toString();  
          LINE.notify(String(location)+" "+IP);
  
   router.begin();
}


void loop() {
  unsigned long currentMillis = millis();
  router.wait();    

  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("ไม่พบ sensor DHT!");
    // Serial.println(wifi_ssid);  
    // Serial.println(wifi_password);  
    if (currentMillis - previousMillis_3 >= interval_3) {
      previousMillis_3 = currentMillis;
      LINE.notify(String(location)+" ไม่พบ sensor DHT!");
    }
    return; 
  }
  


  if (currentMillis - previousMillis_1 >= interval_1) {
    previousMillis_1 = currentMillis;
    // Add data to InfluxDB
  sensor.clearFields();
  sensor.addField("temperature", temperature);
  sensor.addField("humidity", humidity);
    // Print data to Serial
    /*
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" °C, Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");*/
  // Write data to InfluxDB
  if (!client.writePoint(sensor)) {
    //Serial.print("InfluxDB write failed: ");
    //Serial.println(client.getLastErrorMessage());
    logs += "InfluxDB write failed:";    
    }   
  }
  if (currentMillis - previousMillis_2 >= interval_2){
    previousMillis_2 = currentMillis;
    checkTime();
    checkAlarm();    
  }
  
//  delay(30000); // Wait for 30 seconds before sending the next data
}

void startAP(){
  // สร้าง Access Point ชื่อ "AutoConnectAP" (ไม่มีรหัสผ่าน)
    MDNS.addService("http", "tcp", 80);
    WiFi.mode(WIFI_AP);
    WiFi.softAP("esp-"+router.chipID(), "");
    delay(100);
    Serial.println("Access Point started");
}

void checkAlarm() { 
    // Check if temperature is out of the range 15-23 degrees Celsius
  if (temperature < min_temp || temperature > max_temp) {    
    LINE.notify(WiFi.localIP().toString()+"\n"+String(location)+"\n🌡 ค่าที่กำหนด"+min_temp+"-"+max_temp+"\nอุณหภูมิขณะนี้ "+temperature+" องศา \n ความชื้นขณะนี้ "+humidity+" %");
  }
}


void checkTime() {
  time_t now = time(nullptr);
  struct tm* timeInfo = localtime(&now);

  // ตรวจสอบว่าเวลาปัจจุบันตรงกับเวลาเป้าหมายหรือไม่
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
  //Serial.println("Task at 16:00 executed.");
  LINE.notify(WiFi.localIP().toString()+"\n"+String(location)+"\nอุณหภูมิขณะนี้ "+temperature+" องศา \nความชื้นขณะนี้ "+humidity+" % \n err:"+logs);
}

void taskAt0000() {
  //Serial.println("Task at 00:00 executed.");
  LINE.notify(WiFi.localIP().toString()+"\n"+String(location)+"\nอุณหภูมิขณะนี้ "+temperature+" องศา \nความชื้นขณะนี้ "+humidity+" % \n err:"+logs);
}
void taskAt0800() {
  //Serial.println("Task at 08:00 executed.");
  LINE.notify(WiFi.localIP().toString()+"\n"+String(location)+"\nอุณหภูมิขณะนี้ "+temperature+" องศา \nความชื้นขณะนี้ "+humidity+" % \n err:"+logs);
}

