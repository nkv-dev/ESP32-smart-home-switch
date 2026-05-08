#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ================= WIFI =================
#define WIFI_SSID "Airtel_Home@8091 2.4Ghz"
#define WIFI_PASSWORD "Nitesh@7349478091"

// ================= FIREBASE =================
#define API_KEY "AIzaSyA2vHVbqH6EoMuO-iXDzlh0uYL2fF1tPgw"

#define DATABASE_URL "https://esp32-smart-home-switch-nh-default-rtdb.asia-southeast1.firebasedatabase.app/"

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

// ================= SETTINGS =================
const int lightThreshold = 1500;
const int gasThreshold = 750;

const unsigned long motionTimeout = 30000;

const unsigned long sensorInterval = 2000;
const unsigned long firebaseInterval = 2000;
const unsigned long lcdInterval = 1000;

// ================= TIMERS =================
unsigned long previousSensorMillis = 0;
unsigned long previousFirebaseMillis = 0;
unsigned long previousLCDMillis = 0;

unsigned long lastMotionTime = 0;

// ================= VARIABLES =================
float temp = 0;
float hum = 0;

int lightVal = 0;
int gasVal = 0;
int motion = 0;

bool r3State = false;
bool r4State = false;

// ================= SETUP =================
void setup() {

  Serial.begin(115200);

  // ================= PIN MODES =================
  pinMode(R1, OUTPUT);
  pinMode(R2, OUTPUT);
  pinMode(R3, OUTPUT);
  pinMode(R4, OUTPUT);

  pinMode(PIR_PIN, INPUT);

  pinMode(WIFI_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  // ================= INITIAL STATES =================
  digitalWrite(R1, LOW);
  digitalWrite(R2, LOW);
  digitalWrite(R3, LOW);
  digitalWrite(R4, LOW);

  digitalWrite(BUZZER, LOW);

  // ================= START DHT =================
  dht.begin();

  // ================= LCD =================
  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Smart Switch");

  // ================= WIFI =================
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {

    digitalWrite(WIFI_LED, !digitalRead(WIFI_LED));

    delay(300);
  }

  digitalWrite(WIFI_LED, HIGH);

  lcd.clear();
  lcd.print("WiFi Connected");

  // ================= FIREBASE =================
  config.api_key = API_KEY;

  config.database_url = DATABASE_URL;

  auth.user.email = "msrcasc.project@gmail.com";
  auth.user.password = "050809";

  Firebase.begin(&config, &auth);

  Firebase.reconnectWiFi(true);

  delay(1000);

  lcd.clear();
}

// ================= LOOP =================
void loop() {

  unsigned long currentMillis = millis();

  // ================= WIFI STATUS LED =================
  digitalWrite(WIFI_LED, WiFi.status() == WL_CONNECTED);

  // =====================================================
  // SENSOR READING TASK
  // =====================================================
  if (currentMillis - previousSensorMillis >= sensorInterval) {

    previousSensorMillis = currentMillis;

    // Read DHT11
    temp = dht.readTemperature();
    hum = dht.readHumidity();

    // Read Sensors
    lightVal = analogRead(LDR_PIN);

    gasVal = analogRead(GAS_PIN);

    motion = digitalRead(PIR_PIN);

    // ================= SERIAL MONITOR =================
    Serial.print("Temp: ");
    Serial.print(temp);

    Serial.print("  Humidity: ");
    Serial.print(hum);

    Serial.print("  Light: ");
    Serial.print(lightVal);

    Serial.print("  Gas: ");
    Serial.print(gasVal);

    Serial.print("  Motion: ");
    Serial.println(motion);
  }

  // =====================================================
  // LDR AUTOMATION -> RELAY1
  // =====================================================

  // Dark -> ON
  if (lightVal < lightThreshold) {

    digitalWrite(R1, HIGH);

  } else {

    // Bright -> OFF
    digitalWrite(R1, LOW);
  }

  // =====================================================
  // PIR AUTOMATION -> RELAY2
  // =====================================================

  // Motion detected
  if (motion == HIGH) {

    lastMotionTime = currentMillis;

    digitalWrite(R2, HIGH);
  }

  // No motion timeout
  if (currentMillis - lastMotionTime > motionTimeout) {

    digitalWrite(R2, LOW);
  }

  // =====================================================
  // GAS ALERT -> BUZZER
  // =====================================================

  if (gasVal > gasThreshold) {

    digitalWrite(BUZZER, HIGH);

  } else {

    digitalWrite(BUZZER, LOW);
  }

  // =====================================================
  // FIREBASE CONTROL -> RELAY3 & RELAY4
  // =====================================================
  if (currentMillis - previousFirebaseMillis >= firebaseInterval) {

    previousFirebaseMillis = currentMillis;

    if (WiFi.status() == WL_CONNECTED &&
        Firebase.RTDB.getJSON(&fbdo, "/devices/switchboard1")) {

      FirebaseJson &json = fbdo.jsonObject();

      FirebaseJsonData data;

      // Relay3
      if (json.get(data, "relay3")) {

        r3State = data.to<int>();
      }

      // Relay4
      if (json.get(data, "relay4")) {

        r4State = data.to<int>();
      }

      digitalWrite(R3, r3State);

      digitalWrite(R4, r4State);
    }

    // =====================================================
    // SEND SENSOR + RELAY DATA TO FIREBASE
    // =====================================================

    FirebaseJson sensorData;

    // Sensor Data
    sensorData.set("temperature", temp);
    sensorData.set("humidity", hum);

    sensorData.set("light", lightVal);
    sensorData.set("gas", gasVal);

    sensorData.set("motion", motion);

    // Relay Status
    sensorData.set("relay1", digitalRead(R1));
    sensorData.set("relay2", digitalRead(R2));

    sensorData.set("relay3", digitalRead(R3));
    sensorData.set("relay4", digitalRead(R4));

    // Upload to Firebase
    Firebase.RTDB.updateNode(
      &fbdo,
      "/devices/switchboard1",
      &sensorData
    );
  }

  // =====================================================
  // LCD UPDATE
  // =====================================================
  if (currentMillis - previousLCDMillis >= lcdInterval) {

    previousLCDMillis = currentMillis;

    lcd.clear();

    // First Row
    lcd.setCursor(0, 0);

    lcd.print("T:");
    lcd.print(temp);

    lcd.print(" H:");
    lcd.print(hum);

    // Second Row
    lcd.setCursor(0, 1);

    lcd.print("L:");
    lcd.print(lightVal);

    lcd.print(" G:");
    lcd.print(gasVal);
  }
}