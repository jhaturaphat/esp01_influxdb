#if defined(ESP32)
#include <WiFi.h>
#define DEVICE "ESP32"
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#define DEVICE "ESP8266"
#endif

#include <WiFiManager.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include "router.h"
#include "param.h"

DHT dht(DHTPIN, DHTTYPE);

// InfluxDB client instance
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN);

// Data point
Point sensor("DHT11");
Router router;
void setup() {
  Serial.begin(115200);
  if(!LittleFS.begin()) Serial.println("An Error has occurred while mounting LittleFS");
  WiFi.begin(ssid, password);
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Not connected to WiFi. Starting WiFiManager...");
    // เริ่มต้นการตั้งค่า WiFi Manager
    // สร้าง Access Point ชื่อ "AutoConnectAP" (ไม่มีรหัสผ่าน)
    WiFiManager wifiManager;
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
 
  // Initialize DHT sensor
  dht.begin();

  timeSync(TZ_INFO, "pool.ntp.org", "time.google.com");

  // Check InfluxDB connection
  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }

  router.begin();
}

void loop() {
  router.wait();
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

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

  delay(10000); // Wait for 30 seconds before sending the next data
}
