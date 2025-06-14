#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <time.h>

#define TRIG_CUP 14
#define ECHO_CUP 15

#define TRIG_TANK 5
#define ECHO_TANK 21

#define RELAY_PIN 2

const int CUP_THRESHOLD_CM = 8;
const int TANK_MIN_LEVEL_CM = 13;
const int MAX_MISS_COUNT = 5;
const int MAX_TANK_MISS = 3;
const unsigned long SENSOR_TIMEOUT_MS = 2000;

const int TANK_HISTORY_SIZE = 5;
long tankHistory[TANK_HISTORY_SIZE] = {0};
int tankIndex = 0;

int missCount = 0;
int tankMissCount = 0;
bool pumpOn = false;

long lastValidCup = -1;
long lastValidTank = -1;
unsigned long lastCupTime = 0;
unsigned long lastTankTime = 0;

long tankLevelBeforePump = -1;
unsigned long pumpStartTime = 0;

long prevCupPrinted = -1000;
long prevTankPrinted = -1000;
const int PRINT_DIFF_THRESHOLD = 1;

bool prevPumpState = false;

const char* ssid = "Omah Amalia 1.2";
const char* password = "omahamalia1.2";
const char* mqttServer = "ef7aeb66de324cb2a513eae282fc85fe.s1.eu.hivemq.cloud";  // Ganti dengan hostname kamu
const int mqttPort = 8883; // Port TLS
const char* mqtt_user = "kautsar";
const char* mqtt_pass = "Kautsar123";
const char* mqttTopicTank = "dispenser/level";
const char* mqttTopicUsage = "dispenser/penggunaan";

WiFiClientSecure secureClient;
PubSubClient client(secureClient);

unsigned long lastMQTTPublishTime = 0;
const unsigned long MQTT_PUBLISH_INTERVAL = 60000; // 1 menit


long readDistanceCM(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 50000);  // 50ms timeout
  if (duration == 0) return -1;
  return duration * 0.0343 / 2;
}

long getMedianTank() {
  long sorted[TANK_HISTORY_SIZE];
  memcpy(sorted, tankHistory, sizeof(sorted));
  for (int i = 0; i < TANK_HISTORY_SIZE - 1; i++) {
    for (int j = i + 1; j < TANK_HISTORY_SIZE; j++) {
      if (sorted[j] < sorted[i]) {
        long temp = sorted[i];
        sorted[i] = sorted[j];
        sorted[j] = temp;
      }
    }
  }
  return sorted[TANK_HISTORY_SIZE / 2];
}

void printWaterUsedAndTime() {
  if (tankLevelBeforePump > 0 && lastValidTank > 0) {
    long used = lastValidTank - tankLevelBeforePump;
    if (used < 0) used = 0;

    Serial.print("💧 Pemakaian air: ");
    Serial.print(used);
    Serial.println(" cm");

    unsigned long duration = millis() - pumpStartTime;
    Serial.print("⏱️ Durasi pemakaian: ");
    Serial.print(duration);
    Serial.println(" ms");

    // Publish to MQTT
    publishWaterUsageMQTT(duration);
  }

  tankLevelBeforePump = -1;
  pumpStartTime = 0;
}

void setupTime() {
  configTime(7 * 3600, 0, "pool.ntp.org");  // UTC+7 untuk WIB
  Serial.print("⏳ Menyinkronkan waktu...");

  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("\n✅ Waktu tersinkron!");
}

void setupWiFi() {
  delay(100);
  Serial.println("🔌 Connecting to WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n✅ WiFi connected!");
  Serial.println(WiFi.localIP());
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("🔁 Connecting to MQTT... ");
    if (client.connect("ESP32Client", mqtt_user, mqtt_pass)) {
      Serial.println("connected!");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(". Retry in 5s.");
      delay(5000);
    }
  }
}

void publishTankLevelMQTT() {
  StaticJsonDocument<128> doc;
  doc["tank_level_cm"] = lastValidTank;
  char timeStr[25];
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
    doc["timestamp"] = timeStr;
  } else {
    doc["timestamp"] = "unknown";
  }
  char buffer[128];
  serializeJson(doc, buffer);
  client.publish(mqttTopicTank, buffer);
}

void publishWaterUsageMQTT(unsigned long duration) {
  StaticJsonDocument<128> doc;
  doc["duration_ms"] = duration;
  char timeStr[25];
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
    doc["timestamp"] = timeStr;
  } else {
    doc["timestamp"] = "unknown";
  }
  char buffer[128];
  serializeJson(doc, buffer);
  client.publish(mqttTopicUsage, buffer);
}


void setup() {
  Serial.begin(115200);

  pinMode(TRIG_CUP, OUTPUT);
  pinMode(ECHO_CUP, INPUT);
  pinMode(TRIG_TANK, OUTPUT);
  pinMode(ECHO_TANK, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  setupWiFi();
  setupTime();

  secureClient.setInsecure();
  client.setServer(mqttServer, mqttPort);

  Serial.println("Water Dispenser Siap 🚰");
}


void loop() {
  long cupRaw = readDistanceCM(TRIG_CUP, ECHO_CUP);
//  delay(100);
  long tankRaw = readDistanceCM(TRIG_TANK, ECHO_TANK);
  unsigned long now = millis();

  // CUP
  if (cupRaw > 0) {
    lastValidCup = cupRaw;
    lastCupTime = now;
  } else if (now - lastCupTime > SENSOR_TIMEOUT_MS) {
    if (lastValidCup != -1) {
      Serial.println("⚠️ Sensor CUP error: timeout.");
    }
    lastValidCup = -1;
  }

  // TANK
  if (tankRaw > 0) {
    tankHistory[tankIndex] = tankRaw;
    tankIndex = (tankIndex + 1) % TANK_HISTORY_SIZE;

    long median = getMedianTank();
    lastValidTank = median;
    
    lastTankTime = now;
    tankMissCount = 0;
  } else {
    tankMissCount++;
    if (tankMissCount >= MAX_TANK_MISS && now - lastTankTime > SENSOR_TIMEOUT_MS) {
      if (lastValidTank != -1) {
        Serial.println("⚠️ Sensor TANK error: timeout.");
      }
      lastValidTank = -1;
    }
  }

  // Print only if there's a meaningful change
  if (abs(lastValidCup - prevCupPrinted) >= PRINT_DIFF_THRESHOLD ||
      abs(lastValidTank - prevTankPrinted) >= PRINT_DIFF_THRESHOLD) {
    Serial.print("Cup: "); Serial.print(lastValidCup); Serial.print(" cm | ");
    Serial.print("Tank: "); Serial.print(lastValidTank); Serial.println(" cm");

    prevCupPrinted = lastValidCup;
    prevTankPrinted = lastValidTank;
  }

  bool isCupDetected = (lastValidCup > 0 && lastValidCup <= CUP_THRESHOLD_CM);
  bool isWaterSufficient = (lastValidTank > 0 && lastValidTank <= TANK_MIN_LEVEL_CM);

  if (!isWaterSufficient) {
    if (pumpOn) {
      Serial.println("⚠️ Tangki kosong / error. Pompa dimatikan.");
      digitalWrite(RELAY_PIN, LOW);
      pumpOn = false;
      printWaterUsedAndTime();
    }
//    delay(100);
    return;
  }

  if (isCupDetected) {
    missCount = 0;
    if (!pumpOn) {
      Serial.println("✅ Gelas terdeteksi & air cukup. Pompa ON.");
      digitalWrite(RELAY_PIN, HIGH);
      pumpOn = true;
      tankLevelBeforePump = lastValidTank;
      pumpStartTime = millis();
    }
  } else {
    if (pumpOn) {
      missCount++;
      if (missCount >= MAX_MISS_COUNT) {
        Serial.println("❌ Gelas hilang. Pompa OFF.");
        digitalWrite(RELAY_PIN, LOW);
        pumpOn = false;
        printWaterUsedAndTime();
        missCount = 0;
      }
    }
  }

  prevPumpState = pumpOn;
  delay(100);
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  if (millis() - lastMQTTPublishTime >= MQTT_PUBLISH_INTERVAL) {
    publishTankLevelMQTT();
    lastMQTTPublishTime = millis();
  }
}
