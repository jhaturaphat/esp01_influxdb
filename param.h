// WiFi credentials
const char* ssid = "kid_2.4GHz";
const char* password = "xx3xx3xx";


#define TZ_INFO "UTC+7"

// DHT Sensor
#define DHTPIN 2 //IO02 = D4
#define DHTTYPE DHT11

char* influxdb_url = nullptr;
char* influxdb_token = nullptr;
char* influxdb_org = nullptr;
char* influxdb_bucket = nullptr;

// ฟังก์ชันเพื่ออ่านไฟล์และกำหนดค่าตัวแปร
bool loadConfig() {
  File file = LittleFS.open("/influx.json", "r");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return false;
  }

  size_t size = file.size();
  if (size > 1024) {
    Serial.println("File size is too large");
    file.close();
    return false;
  }

  std::unique_ptr<char[]> buf(new char[size]);
  file.readBytes(buf.get(), size);
  file.close();

  DynamicJsonDocument doc(1024);
  auto error = deserializeJson(doc, buf.get());

  if (error) {
    Serial.println("Failed to parse JSON");
    return false;
  }

  influxdb_url = strdup(doc["influxdb_url"]);
  influxdb_token = strdup(doc["influxdb_token"]);
  influxdb_org = strdup(doc["influxdb_org"]);
  influxdb_bucket = strdup(doc["influxdb_bucket"]);

  return true;
}
