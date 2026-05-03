#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ================= WIFI =================
#define WIFI_SSID "Airtel_Home@8091 2.4Ghz"
#define WIFI_PASSWORD "Nitesh@7349478091"

// ================= FIREBASE =================
#define API_KEY "YOUR_API_KEY"
#define DATABASE_URL "YOUR_DB_URL"

// ================= RELAYS =================
#define R1 25
#define R2 26
#define R3 27
#define R4 14

// ================= SENSORS =================
#define DHTPIN 4
#define DHTTYPE DHT11
#define LDR_PIN 34
#define GAS_PIN 35
#define PIR_PIN 13

// ================= OUTPUT =================
#define WIFI_LED 2
#define BUZZER 15

// ================= OBJECTS =================
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// ================= STATES =================
bool r1State = false;
bool r2State = false;
bool r3State = false;
bool r4State = false;

// motion timer
unsigned long lastMotionTime = 0;
const int motionTimeout = 30000; // 30 sec

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  pinMode(R1, OUTPUT);
  pinMode(R2, OUTPUT);
  pinMode(R3, OUTPUT);
  pinMode(R4, OUTPUT);

  pinMode(WIFI_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(PIR_PIN, INPUT);

  digitalWrite(R1, LOW);
  digitalWrite(R2, LOW);
  digitalWrite(R3, LOW);
  digitalWrite(R4, LOW);
  digitalWrite(BUZZER, LOW);

  dht.begin();

  lcd.init();
  lcd.backlight();
  lcd.print("Smart Switch");

  // WiFi connect
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(WIFI_LED, !digitalRead(WIFI_LED));
    delay(300);
  }
  digitalWrite(WIFI_LED, HIGH);

  lcd.clear();
  lcd.print("WiFi Connected");

  // Firebase config
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  config.signer.test_mode = true;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  delay(2000);
  lcd.clear();
}

// ================= LOOP =================
void loop() {

  // WiFi LED
  digitalWrite(WIFI_LED, WiFi.status() == WL_CONNECTED);

  // ========= FIREBASE RELAY CONTROL =========
  if (WiFi.status() == WL_CONNECTED &&
      Firebase.RTDB.getJSON(&fbdo, "/devices/switchboard1")) {

    FirebaseJson &json = fbdo.jsonObject();
    FirebaseJsonData data;

    if (json.get(data, "relay1")) r1State = data.to<int>();
    if (json.get(data, "relay2")) r2State = data.to<int>();
    if (json.get(data, "relay3")) r3State = data.to<int>();
    if (json.get(data, "relay4")) r4State = data.to<int>();

    digitalWrite(R1, r1State);
    digitalWrite(R2, r2State);
    digitalWrite(R3, r3State);
    digitalWrite(R4, r4State);
  }

  // ========= SENSOR READ =========
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  int lightVal = analogRead(LDR_PIN);
  int gasVal = analogRead(GAS_PIN);
  int motion = digitalRead(PIR_PIN);

  // Motion tracking
  if (motion == HIGH) {
    lastMotionTime = millis();
  }

  // 🔥 Gas Alert
  digitalWrite(BUZZER, gasVal > 300);

  // 💡 Smart Light (Relay1)
  if (lightVal < 1500 && motion == HIGH) {
    digitalWrite(R1, HIGH);
  }

  // Auto OFF after no motion
  if (millis() - lastMotionTime > motionTimeout) {
    digitalWrite(R1, LOW);
  }

  // ========= LCD =========
  lcd.setCursor(0,0);
  lcd.print("T:");
  lcd.print(temp);
  lcd.print(" H:");
  lcd.print(hum);

  lcd.setCursor(0,1);
  lcd.print("L:");
  lcd.print(lightVal);
  lcd.print(" G:");
  lcd.print(gasVal);

  // ========= SEND DATA =========
  FirebaseJson json;
  json.set("temperature", temp);
  json.set("humidity", hum);
  json.set("light", lightVal);
  json.set("gas", gasVal);
  json.set("motion", motion);

  Firebase.RTDB.updateNode(&fbdo, "/devices/switchboard1", &json);

  delay(2000);
}