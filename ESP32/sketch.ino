#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_BMP085.h>

// ---------- Cấu hình ----------
const char* WIFI_SSID = "Wokwi-GUEST";   // WiFi mặc định của Wokwi
const char* WIFI_PASS = "";
const char* MQTT_SERVER = "broker.hivemq.com";
const int   MQTT_PORT = 1883;


#define TOPIC_DATA   "runner_NguyenVanCuong/data"
#define TOPIC_LED    "unner_NguyenVanCuong/led"
#define TOPIC_BUZZER "runner_NguyenVanCuong/buzzer"
#define TOPIC_ALERT  "runner_NguyenVanCuong/alert"

#define LED_PIN    26
#define BUZZER_PIN 25

// Ngưỡng cảnh báo
const float TEMP_HIGH = 35.0;
const float TEMP_LOW  = 15.0;
const float ALT_JUMP  = 5.0;   // thay đổi độ cao đột ngột (m) giữa 2 lần đọc

// ---------- Cấu trúc dữ liệu ----------
struct SensorData { float temp; float pressure; float altitude; };
struct AlertData  { float temp; float altitude; };

QueueHandle_t dataQueue;
QueueHandle_t alertQueue;

volatile bool ledManual = false;
volatile bool buzzerManual = false;

Adafruit_BMP085 bmp;
WiFiClient espClient;
PubSubClient mqtt(espClient);

// ---------- Nhận lệnh từ Web ----------
void mqttCallback(char* topic, byte* payload, unsigned int len) {
  String msg;
  for (unsigned int i = 0; i < len; i++) msg += (char)payload[i];
  bool on = (msg == "ON");
  if (String(topic) == TOPIC_LED)    ledManual = on;
  if (String(topic) == TOPIC_BUZZER) buzzerManual = on;
}

// ---------- Task truyền data (Core 0) ----------
void TaskSend(void* pv) {
  // 1. Kết nối WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASS, 6);
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(500 / portTICK_PERIOD_MS);
    Serial.print(".");
  }
  Serial.println("\nWiFi OK");

  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt.setCallback(mqttCallback);

  SensorData d;
  for (;;) {
    // 2. Giữ kết nối MQTT
    if (!mqtt.connected()) {
      String id = "esp32-runner-" + String(random(0xffff), HEX);
      if (mqtt.connect(id.c_str())) {
        mqtt.subscribe(TOPIC_LED);
        mqtt.subscribe(TOPIC_BUZZER);
        Serial.println("MQTT OK");
      } else {
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        continue;
      }
    }
    mqtt.loop();

    // 3. Lấy dữ liệu từ queue và publish
    if (xQueueReceive(dataQueue, &d, 100 / portTICK_PERIOD_MS) == pdTRUE) {
      char buf[128];
      snprintf(buf, sizeof(buf),
               "{\"temp\":%.2f,\"pressure\":%.2f,\"altitude\":%.2f}",
               d.temp, d.pressure, d.altitude);
      mqtt.publish(TOPIC_DATA, buf);
    }
  }
}

// ---------- Task đọc data (Core 1) ----------
void TaskRead(void* pv) {
  SensorData d;
  AlertData  a;
  for (;;) {
    d.temp     = bmp.readTemperature();
    d.pressure = bmp.readPressure() / 100.0;          // Pa -> hPa
    d.altitude = bmp.readAltitude(101325);            // so với mực nước biển
    a.temp = d.temp;
    a.altitude = d.altitude;

    Serial.printf("T=%.2f C | P=%.2f hPa | Alt=%.2f m\n", d.temp, d.pressure, d.altitude);

    xQueueSend(dataQueue, &d, 0);
    xQueueSend(alertQueue, &a, 0);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

// ---------- Task cảnh báo (Core 1) ----------
void TaskAlert(void* pv) {
  AlertData a;
  float lastAlt = NAN;
  bool alerting = false;
  TickType_t lastAlertTick = 0;

  for (;;) {
    // Chờ dữ liệu tối đa 100ms để còn xử lý nháy LED/chế độ manual
    if (xQueueReceive(alertQueue, &a, 100 / portTICK_PERIOD_MS) == pdTRUE) {
      bool tempBad = (a.temp > TEMP_HIGH || a.temp < TEMP_LOW);
      bool altBad  = (!isnan(lastAlt) && fabs(a.altitude - lastAlt) > ALT_JUMP);
      lastAlt = a.altitude;

      if (tempBad || altBad) {
        alerting = true;
        lastAlertTick = xTaskGetTickCount();
        Serial.println(tempBad ? "!!! CANH BAO NHIET DO" : "!!! CANH BAO DO CAO");
      }
    }
    // Giữ cảnh báo 2 giây sau lần phát hiện cuối
    if (alerting && (xTaskGetTickCount() - lastAlertTick) > 2000 / portTICK_PERIOD_MS)
      alerting = false;

    bool blink = alerting && ((millis() / 250) % 2 == 0);
    digitalWrite(LED_PIN,    (ledManual    || blink) ? HIGH : LOW);
    digitalWrite(BUZZER_PIN, (buzzerManual || blink) ? HIGH : LOW);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  if (!bmp.begin()) {
    Serial.println("Khong tim thay BMP180!");
    while (1) delay(1000);
  }

  dataQueue  = xQueueCreate(10, sizeof(SensorData));
  alertQueue = xQueueCreate(10, sizeof(AlertData));

  xTaskCreatePinnedToCore(TaskSend,  "TaskSend",  6144, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(TaskRead,  "TaskRead",  4096, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(TaskAlert, "TaskAlert", 3072, NULL, 2, NULL, 1);
}

void loop() { vTaskDelete(NULL); }