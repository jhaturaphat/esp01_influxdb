//For esp 32
#if defined(ESP32)
#include <WiFi.h>
#include <ESPmDNS.h>
#define DEVICE "ESP32"
//For Esp 8266
#elif defined(ESP8266) 
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#define DEVICE "ESP8266"
#endif
//Global include
#include "FS.h"
#include <LittleFS.h>
#include <time.h>
// #include <WiFiManager.h>
#include <ArduinoJson.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
#include <Adafruit_Sensor.h>
// #include <TridentTD_LineNotify.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <DHT.h>
#include <DHT_U.h>
#include "router.h"
#include "param.h"

#define TZ_INFO "GMT+7"

unsigned long previousMillis_1 = 0;
unsigned long previousMillis_2 = 0;
unsigned long previousMillis_3 = 0;
const long interval_60 = 60000;       // กำหนด interval เป็น 60 วินาที
const long interval_30 = 30000;       // กำหนด interval เป็น 30 วินาที
const long interval_20 = 30000;       // กำหนด interval เป็น 20 วินาที
const long interval_10 = 10000;       // กำหนด interval เป็น 10 วินาที

unsigned long startAttemptTime = 0;
const unsigned long wifiTimeout = 30000; // 10 วินาที (10000 มิลลิวินาที)

bool alarm_stat = true;

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

const char* BOT_TOKEN = "8099954188:AAGYPRNbmwzUVgmVVERNXj9z2OONFcuwX6w";
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client); // เริ่มต้นด้วย Token ปลอม

void setup() {  
  Serial.begin(115200); 
  loadCfgWifi();
  loadConfig();
  startAttemptTime = millis();
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED){
    if (millis() - startAttemptTime >= wifiTimeout) {
      Serial.println("ไม่สามารถเชื่อมต่อ WiFi ได้ เปลี่ยนไปเป็นโหมด AP");      
      router.modeAP(WiFi);
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
          // ตั้งค่า Secure Client (จำเป็นสำหรับ HTTPS)
          #ifdef ESP8266
            secured_client.setInsecure(); // สำหรับ ESP8266 (ไม่แนะนำใน Production)
          #elif defined(ESP32)
            secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // สำหรับ ESP32
          #endif

           // อัปเดต Token จริงภายหลัง (โหลดจาก EEPROM/Web/ไฟล์)         
          bot.updateToken(line_token); // เปลี่ยน Token ได้ทันที!
          // ทดสอบส่งข้อความ
          bot.sendMessage(chanel, "Bot เริ่มทำงานแล้ว!");

           
          sensor = Point(influxdb_point);
          if(String(influxdb_url).startsWith("https")){
            client.setConnectionParams(influxdb_url, influxdb_org, influxdb_bucket, influxdb_token, InfluxDbCloud2CACert);
          }else{
            client.setConnectionParams(influxdb_url, influxdb_org, influxdb_bucket, influxdb_token);
          }
          
          if (client.validateConnection()) {
            Serial.print("Connected to InfluxDB: ");
            Serial.println(client.getServerUrl());
          } else {
            Serial.print("InfluxDB connection failed: ");
            Serial.println(client.getLastErrorMessage());
            String err = client.getLastErrorMessage();
            delay(1000);
            // LINE.notify("InfluxDB connection failed:"+err);
            bot.sendMessage(chanel, "InfluxDB connection failed:"+err);
          }
          
          // LINE.setToken(line_token);  
          String IP = WiFi.localIP().toString();  
          // LINE.notify(String(location)+" "+IP);
          bot.sendMessage(chanel, String(location)+" "+IP);
          Serial.println(String(location)+" "+IP);
  
   router.begin();   

}


void loop() {
  unsigned long currentMillis = millis();
  router.start();    

if (WiFi.getMode() == WIFI_AP) return;
  
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  
  if (isnan(humidity) || isnan(temperature)) {    
    // Serial.println(wifi_ssid);  
    // Serial.println(wifi_password);  
    if (currentMillis - previousMillis_3 >= interval_10) {
      previousMillis_3 = currentMillis;
      // Serial.println("ไม่พบ sensor DHT!");
      // LINE.notify(String(location)+" ไม่พบ sensor DHT!");
      bot.sendMessage(chanel, String(location)+" ไม่พบ sensor DHT!");
    }
    return; 
  }
  


  if (currentMillis - previousMillis_1 >= interval_10) {
    previousMillis_1 = currentMillis;
    // Add data to InfluxDB
    sensor.clearFields();
    sensor.addField("temperature", temperature);
    sensor.addField("humidity", humidity);    
      // Write data to InfluxDB
      if (!client.writePoint(sensor)) {
        //Serial.print("InfluxDB write failed: ");
        //Serial.println(client.getLastErrorMessage());
        logs = "InfluxDB write failed:";    
      }else{
        logs = "";
    }
  }
  if (currentMillis - previousMillis_2 >= interval_30){
    previousMillis_2 = currentMillis;    
    checkAlarm();  
    alarm_stat = true;  //reset ค่า
  }
checkTime();
  
//  delay(30000); // Wait for 30 seconds before sending the next data
}

void checkAlarm() { 
    // Check if temperature is out of the range 15-23 degrees Celsius
  if (temperature < min_temp || temperature > max_temp) {    
    // LINE.notify(WiFi.localIP().toString()+"\n🚨🚨"+String(location)+"🚨🚨\n ค่าที่กำหนด"+min_temp+"-"+max_temp+"\n🌡️อุณหภูมิขณะนี้ "+temperature+" องศา \n☔ ความชื้นขณะนี้ "+humidity+" %");
    bot.sendMessage(chanel, WiFi.localIP().toString()+"\n🚨🚨"+String(location)+"🚨🚨\n ค่าที่กำหนด"+min_temp+"-"+max_temp+"\n🌡️อุณหภูมิขณะนี้ "+temperature+" องศา \n☔ ความชื้นขณะนี้ "+humidity+" %");
  }
}


void checkTime() {
  if(!alarm_stat) return;
  time_t now = time(nullptr);
  struct tm* timeInfo = localtime(&now);
  int tm_hour = timeInfo->tm_hour;
  int tm_min = timeInfo->tm_min;
  int tm_sec = timeInfo->tm_sec;

  // // ตรวจสอบว่าเวลาปัจจุบันตรงกับเวลาเป้าหมายหรือไม่

  if ((tm_hour == 16) && (tm_min == 0) && (tm_sec <= 1)) {
    taskAt1600();
    alarm_stat = false;
  }else if ((tm_hour == 0) && (tm_min == 0) && (tm_sec <= 1)) {
    taskAt0000();
    alarm_stat = false;
  }else if ((tm_hour == 8) && (tm_min == 0) && (tm_sec <= 1)) {
    taskAt0800();
    alarm_stat = false;
  }
}

void taskAt1600() {
  //Serial.println("Task at 16:00 executed.");
  // LINE.notify(WiFi.localIP().toString()+"\n"+String(location)+"\n🌡 อุณหภูมิขณะนี้ "+temperature+" องศา \n☔ ความชื้นขณะนี้ "+humidity+" % \n error:"+logs);
  bot.sendMessage(chanel, WiFi.localIP().toString()+"\n"+String(location)+"\n🌡 อุณหภูมิขณะนี้ "+temperature+" องศา \n☔ ความชื้นขณะนี้ "+humidity+" %");
}

void taskAt0000() {
  //Serial.println("Task at 00:00 executed.");
  // LINE.notify(WiFi.localIP().toString()+"\n"+String(location)+"\n🌡 อุณหภูมิขณะนี้ "+temperature+" องศา \n☔ ความชื้นขณะนี้ "+humidity+" % \n error:"+logs);
  bot.sendMessage(chanel, WiFi.localIP().toString()+"\n"+String(location)+"\n🌡 อุณหภูมิขณะนี้ "+temperature+" องศา \n☔ ความชื้นขณะนี้ "+humidity+" %");
}
void taskAt0800() {
  //Serial.println("Task at 08:00 executed.");
  // LINE.notify(WiFi.localIP().toString()+"\n"+String(location)+"\n🌡 อุณหภูมิขณะนี้ "+temperature+" องศา \n☔ ความชื้นขณะนี้ "+humidity+" % \n error:"+logs);
  bot.sendMessage(chanel, WiFi.localIP().toString()+"\n"+String(location)+"\n🌡 อุณหภูมิขณะนี้ "+temperature+" องศา \n☔ ความชื้นขณะนี้ "+humidity+" %");
}

