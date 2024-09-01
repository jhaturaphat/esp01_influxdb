
// DHT Sensor
#define DHTPIN 2 //IO02 = D4
#define DHTTYPE DHT11

const long Previous_restart = 0;
const long Interval_restart = 1000 * 60 * 60 * (24 * 60);  // 2 เดือน

float humidity        = 0.0;
float temperature     = 0.0;

String logs = "";

char* wifi_ssid       = nullptr;
char* wifi_password   = nullptr;
char* influxdb_url    = nullptr;
char* influxdb_token  = nullptr;
char* influxdb_org    = nullptr;
char* influxdb_bucket = nullptr;
char* location        = nullptr;
char* line_token      = nullptr;
int min_temp          = 0;
int max_temp          = 0;


// ฟังก์ชัน trim() เพื่อตัดช่องว่าง
void trim(char* str) {
  // ตัดช่องว่างจากด้านหน้า
  int start = 0;
  while (isspace(str[start])) {
    start++;
  }

  // ตัดช่องว่างจากด้านหลัง
  int end = strlen(str) - 1;
  while (end >= start && isspace(str[end])) {
    end--;
  }

  // ย้ายสตริงที่ตัดแล้วไปที่ตำแหน่งเริ่มต้นของอาร์เรย์
  int length = end - start + 1;
  memmove(str, str + start, length);

  // เติม null terminator เพื่อสิ้นสุดสตริง
  str[length] = '\0';
}


// ฟังก์ชันเพื่ออ่านไฟล์และกำหนดค่าตัวแปร
bool loadConfig() {
  if(!LittleFS.begin()) Serial.println("LittleFS Mount Failed");
  
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

  wifi_ssid       = strdup(doc["wifi_ssid"]);
  wifi_password   = strdup(doc["wifi_password"]);
  influxdb_url    = strdup(doc["influxdb_url"]);
  influxdb_token  = strdup(doc["influxdb_token"]);
  influxdb_org    = strdup(doc["influxdb_org"]);
  influxdb_bucket = strdup(doc["influxdb_bucket"]);
  line_token      = strdup(doc["line_token"]);
  location        = strdup(doc["location"]);
  min_temp        = doc["min_temp"];
  max_temp        = doc["max_temp"];

  Serial.println(wifi_ssid);
  Serial.println(wifi_password);
  Serial.println(influxdb_url);
  Serial.println(influxdb_token);
  Serial.println(influxdb_org);
  Serial.println(influxdb_bucket);  
  Serial.println(line_token);
  Serial.println(location);
  Serial.println(min_temp, DEC);
  Serial.println(max_temp, DEC);

  return true;
}
