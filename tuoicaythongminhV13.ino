#include <WiFi.h>
#include <PubSubClient.h>
#include <WebServer.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ================== KHAI BÁO CHÂN ==================
#define DHTPIN 27
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define IN1 32        // Điều khiển động cơ (nếu có)
#define IN2 33
#define SOIL_AO 34    // Cảm biến độ ẩm đất analog
#define SOIL_DO 35    // Digital (không dùng ở đây)

#define RELAY_PUMP 25   // Relay chính - bơm tưới
#define RELAY_EXTRA 26  // Relay phụ mới (đèn, quạt, bơm phụ...)

int soilValue = 0;
int soilPercent = 0;
int threshold = 40;     // Ngưỡng độ ẩm mặc định
bool pumpState = false; // Trạng thái bơm (relay chính)
bool extraState = false; // Trạng thái relay phụ
bool motorState = false;
String mode = "auto";   // "auto" hoặc "manual"

// ================== WIFI & MQTT ==================
const char* ssid = "FPT L2";
const char* password = "toto662005";
const char* mqtt_server = "test.mosquitto.org";

WiFiClient espClient;
PubSubClient client(espClient);
WebServer server(80);  // Web server chạy song song với MQTT

// ================== SETUP WIFI ==================
void setup_wifi() {
  Serial.println("Đang kết nối WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi đã kết nối!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

// ================== CẬP NHẬT OLED ==================
void updateOLED(float t, float airHum, int soilHum) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.print("Nhiet do: ");
  display.print(t, 1);
  display.println("C");

  display.setCursor(0, 12);
  display.print("Do am KK: ");
  display.print(h, 1);
  display.println("%");

  display.setCursor(0, 24);
  display.print("Do am dat: ");
  display.print(soilPercent);
  display.println("%");

  display.setCursor(0, 36);
  display.print("Mode: ");
  display.println(mode);

  display.setCursor(0, 48);
  display.print("Bom: ");
  display.print(pumpState ? "ON " : "OFF");
  display.print(" | Phu: ");
  display.println(extraState ? "ON" : "OFF");

  display.display();
}

// ================== ĐIỀU KHIỂN RELAY QUA HTTP ==================
void handlePumpOn() {
  if (mode == "manual") {
    pumpState = true;
    digitalWrite(RELAY_PUMP, HIGH);
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
  }
  server.send(200, "text/plain", pumpState ? "Pump ON" : "Blocked (Auto mode)");
  Serial.println("HTTP: Pump ON requested");
}

void handlePumpOff() {
  if (mode == "manual") {
    pumpState = false;
    digitalWrite(RELAY_PUMP, LOW);
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
  }
  server.send(200, "text/plain", pumpState ? "Pump ON" : "Pump OFF");
  Serial.println("HTTP: Pump OFF requested");
}

void handleExtraOn() {
  extraState = true;
  digitalWrite(RELAY_EXTRA, HIGH);
  server.send(200, "text/plain", "Relay Extra ON");
  Serial.println("HTTP: Relay Extra ON");
}

void handleExtraOff() {
  extraState = false;
  digitalWrite(RELAY_EXTRA, LOW);
  server.send(200, "text/plain", "Relay Extra OFF");
  Serial.println("HTTP: Relay Extra OFF");
}

// ================== MQTT CALLBACK ==================
void callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (int i = 0; i < length; i++) msg += (char)payload[i];

  Serial.print("MQTT nhận [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(msg);

  if (String(topic) == "relay/control") {  // Điều khiển bơm chính
    if (mode == "manual") {
      if (msg == "true" || msg == "1" || msg == "on") {
        pumpState = true;
        digitalWrite(RELAY_PUMP, HIGH);
        digitalWrite(IN1, HIGH);
        digitalWrite(IN2, LOW);
      } else {
        pumpState = false;
        digitalWrite(RELAY_PUMP, LOW);
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, LOW);
      }
      client.publish("relay/state", pumpState ? "1" : "0");
    } else {
      Serial.println("⚠️ Chế độ AUTO → bỏ qua lệnh thủ công bơm!");
    }
  }

  if (String(topic) == "relay/extra") {  // Điều khiển relay phụ mới
    if (msg == "true" || msg == "1" || msg == "on") {
      extraState = true;
      digitalWrite(RELAY_EXTRA, HIGH);
    } else {
      extraState = false;
      digitalWrite(RELAY_EXTRA, LOW);
    }
    client.publish("relay/extra/state", extraState ? "1" : "0");
  }

  if (String(topic) == "cambiendat/tuychon") {
    threshold = msg.toInt();
    if (threshold < 10) threshold = 10;
    if (threshold > 90) threshold = 90;
    Serial.print("Ngưỡng mới: ");
    Serial.println(threshold);
  }

  if (String(topic) == "control/mode") {
    msg.toLowerCase();
    if (msg == "auto" || msg == "manual") {
      mode = msg;
      Serial.print("Chuyển chế độ: ");
      Serial.println(mode);
      client.publish("control/mode/state", mode.c_str());

      // Khi chuyển sang auto → tắt bơm thủ công
      if (mode == "auto") {
        pumpState = false;
        digitalWrite(RELAY_PUMP, LOW);
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, LOW);
      }
    }
  }
}

// ================== MQTT RECONNECT ==================
void reconnect() {
  while (!client.connected()) {
    Serial.print("Đang kết nối MQTT...");
    if (client.connect("ESP32Client_TuoiCay")) {
      Serial.println("✅ MQTT Connected!");
      client.subscribe("relay/control");
      client.subscribe("relay/extra");
      client.subscribe("cambiendat/tuychon");
      client.subscribe("control/mode");

      client.publish("control/mode/state", mode.c_str());
      client.publish("relay/state", pumpState ? "1" : "0");
      client.publish("relay/extra/state", extraState ? "1" : "0");
    } else {
      Serial.print("Lỗi, rc=");
      Serial.print(client.state());
      Serial.println(" → thử lại sau 5s");
      delay(5000);
    }
  }
}

// ================== SETUP ==================
void setup() {
  Serial.begin(115200);

  // Khởi tạo các chân
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(RELAY_PUMP, OUTPUT);
  pinMode(RELAY_EXTRA, OUTPUT);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(RELAY_PUMP, LOW);
  digitalWrite(RELAY_EXTRA, LOW);

  dht.begin();
  setup_wifi();

  // MQTT
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  // Web Server routes
  server.on("/pump/on", HTTP_GET, handlePumpOn);
  server.on("/pump/off", HTTP_GET, handlePumpOff);
  server.on("/extra/on", HTTP_GET, handleExtraOn);
  server.on("/extra/off", HTTP_GET, handleExtraOff);
  server.begin();
  Serial.println("HTTP server started");

  // OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("❌ Không tìm thấy OLED!");
    for (;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 20);
  display.println("Smart Plant Ready!");
  display.display();
  delay(1500);

    WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
  Serial.println(WiFi.localIP());

  server.on("/relay/on", relayOn);
  server.on("/relay/off", relayOff);
  server.begin();
}

// ================== LOOP ==================
void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  soilValue = analogRead(SOIL_AO);
  soilPercent = map(soilValue, 0, 4095, 100, 0);

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (!isnan(h) && !isnan(t)) {
    Serial.printf("🌡️ %.1f°C | 💧 %.1f%% | 🌱 %d%% | Mode: %s\n",
                  t, h, soilPercent, mode.c_str());
    client.publish("Ehab/DHT/Temp", String(t).c_str());
    client.publish("Ehab/DHT/Humidity", String(h).c_str());
    client.publish("Ehab/doamdat/Temp", String(soilPercent).c_str());
    updateOLED(t, soilPercent);  // Cập nhật lên OLED
  }

  if (soilPercent == 0) {
    if (mode != "manual") {
      mode = "manual";
      digitalWrite(RELAY_PUMP, LOW);
      Serial.println("⚠️ Cảm biến lỗi → Chuyển MANUAL & tắt bơm!");
      client.publish("control/mode/state", mode.c_str());
      motorState = false;
    }
  }

  else if (soilPercent < threshold) {
    if (mode != "auto") {
      mode = "auto";
      Serial.println("🌱 Độ ẩm < 40% → Chuyển sang AUTO!");
      client.publish("control/mode/state", mode.c_str());
    }
    motorState = true;  // Bật bơm tự động
  }

  else if (soilPercent >= threshold) {
    // Chỉ tắt bơm nếu đang ở AUTO
    if (mode == "auto" && motorState) {
      motorState = false;
      Serial.println("💧 Độ ẩm > 40% → AUTO tự tắt bơm!");
      client.publish("relay/state", "0");
    }

    // Sau đó chuyển sang manual
    if (mode != "manual") {
      mode = "manual";
      Serial.println("⚙️ Chuyển sang MANUAL → có thể bật bơm thủ công!");
      client.publish("control/mode/state", mode.c_str());
    }
  }

  if (mode == "auto") {
    if (soilPercent < threshold) {
      motorState = true;
      digitalWrite(RELAY_PUMP, HIGH);
    } else {
      motorState = false;
      digitalWrite(RELAY_PUMP, LOW);
    }
    client.publish("relay/state", motorState ? "1" : "0");

  }
  
  delay(2000);
}