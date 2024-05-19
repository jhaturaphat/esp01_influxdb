// WiFi credentials
const char* ssid = "kid_2.4GHz";
const char* password = "xx3xx3xx";

// InfluxDB settings
#define INFLUXDB_URL "http://192.168.1.142:8086"
#define INFLUXDB_TOKEN "DjVqNpgUOYUGFUCkZ4bsV3dnNwjFxfPXpaxMbVBAnhG23KAUsVflZCu4Ge5vT22Q1nreRa0R3FDh48Zwp2knCw=="
#define INFLUXDB_ORG "f6f536a6307c4982"
#define INFLUXDB_BUCKET "DHT11-SENSOR"
#define TZ_INFO "UTC+7"

// DHT Sensor
#define DHTPIN 2 //IO02 = D4
#define DHTTYPE DHT11