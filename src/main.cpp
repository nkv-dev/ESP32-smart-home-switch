// ============================================================================
// ESP32 SMART HOME SWITCH - HEAVILY COMMENTED FOR BEGINNERS
// ============================================================================
// This program runs on an ESP32 microcontroller to create a smart home system.
// It reads sensors (temperature, humidity, light, gas, motion) and controls
// 4 relays (switches) based on automation rules and Firebase commands.
// ============================================================================

// ============================================================================
// PART 1: INCLUDING LIBRARIES
// ============================================================================
// Libraries are pre-written code that we can use to save time.
// Think of them like ready-made tools - we just need to include them.
// Include this library to connect to WiFi networks
#include <WiFi.h>

// Include this library to communicate with Firebase (online database)
// This allows us to control the ESP32 from a phone app or website
#include <Firebase_ESP_Client.h>

// Include this library to read from DHT11 sensor (temperature & humidity)
#include <DHT.h>

// Include this library for I2C communication (used by LCD and some sensors)
// I2C is a way for different electronic parts to talk to each other using just 2 wires
#include <Wire.h>

// Include this library to control a 16x2 LCD screen via I2C
// This displays our sensor readings on a screen
#include <LiquidCrystal_I2C.h>


// ============================================================================
// PART 2: WIFI SETTINGS - CONNECT TO YOUR INTERNET
// ============================================================================
// REPLACE "Airtel_Home@8091 2.4Ghz" WITH YOUR WIFI NAME (SSID)
// REPLACE "Nitesh@7349478091" WITH YOUR WIFI PASSWORD
// The ESP32 needs to connect to WiFi to send data to Firebase and the internet
#define WIFI_SSID "Airtel_Home@8091 2.4Ghz"
#define WIFI_PASSWORD "Nitesh@7349478091"


// ============================================================================
// PART 3: FIREBASE SETTINGS - CONNECT TO ONLINE DATABASE
// ============================================================================
// REPLACE THESE WITH YOUR FIREBASE PROJECT CREDENTIALS
// You get these from your Firebase Console after creating a project
// API_KEY is like a password that identifies your Firebase project
#define API_KEY "AIzaSyA2vHVbqH6EoMuO-iXDzlh0uYL2fF1tPgw"

// DATABASE_URL is the web address where your Firebase database is stored
// Don't change this unless you created a different Firebase project
#define DATABASE_URL "https://esp32-smart-home-switch-nh-default-rtdb.asia-southeast1.firebasedatabase.app/"


// ============================================================================
// PART 4: RELAY PIN ASSIGNMENTS - WHICH PINS CONTROL WHICH RELAYS?
// ============================================================================
// The ESP32 has numbered GPIO (General Purpose Input/Output) pins.
// We assign each component to a specific pin number so we can control it.
// RELAYS are electrically controlled switches that can turn lights/appliances on/off
// R1 = Relay 1 (connected to GPIO 25) - Automatic light based on darkness
// R2 = Relay 2 (connected to GPIO 26) - Automatic light based on motion
// R3 = Relay 3 (connected to GPIO 27) - Remote control via Firebase
// R4 = Relay 4 (connected to GPIO 14) - Remote control via Firebase
#define R1 25    // GPIO pin 25 controls Relay 1
#define R2 26    // GPIO pin 26 controls Relay 2
#define R3 27    // GPIO pin 27 controls Relay 3
#define R4 14    // GPIO pin 14 controls Relay 4


// ============================================================================
// PART 5: SENSOR PIN ASSIGNMENTS - WHICH PINS READ SENSORS?
// ============================================================================
// SENSORS measure things in the environment and send signals to the ESP32.
// DHTPIN = The pin where DHT11 sensor is connected (temperature & humidity)
// LDR_PIN = The pin where LDR (Light Dependent Resistor) is connected
// GAS_PIN = The pin where Gas sensor is connected
// PIR_PIN = The pin where PIR (Passive Infrared) motion sensor is connected
#define DHTPIN 4     // GPIO pin 4 reads DHT11 sensor data
#define DHTTYPE DHT11 // We're using DHT11 sensor model (there are DHT11, DHT22, etc.)

#define LDR_PIN 34    // GPIO pin 34 reads light level (analog input)
#define GAS_PIN 35    // GPIO pin 35 reads gas level (analog input)
#define PIR_PIN 13   // GPIO pin 13 reads motion detection (digital input)


// ============================================================================
// PART 6: OUTPUT PIN ASSIGNMENTS - STATUS LED AND BUZZER
// ============================================================================
// WIFI_LED = A small LED that shows if WiFi is connected
// BUZZER = An alarm that beeps when gas level is too high
#define WIFI_LED 2  // GPIO pin 2 controls the WiFi status LED
#define BUZZER 15   // GPIO pin 15 controls the buzzer/alarm


// ============================================================================
// PART 7: CREATING OBJECTS - INITIALIZING OUR HARDWARE
// ============================================================================
// In C++, we create "objects" to represent hardware components.
// Think of objects like having a remote control for each device.

// Create a DHT object called "dht" for temperature and humidity sensor
// We pass the pin number (DHTPIN) and sensor type (DHTTYPE)
DHT dht(DHTPIN, DHTTYPE);

// Create an LCD object called "lcd" for the 16x2 screen
// 0x27 is the I2C address (where the LCD is on the I2C bus)
// 16 means 16 columns (characters per row), 2 means 2 rows
LiquidCrystal_I2C lcd(0x27, 16, 2);


// ============================================================================
// PART 8: FIREBASE OBJECTS - FOR CONNECTING TO DATABASE
// ============================================================================
// FirebaseData = Object to hold data coming from Firebase
// FirebaseAuth = Object to handle login authentication
// FirebaseConfig = Object to store Firebase settings
FirebaseData fbdo;      // Stores data from Firebase queries
FirebaseAuth auth;     // Handles user login
FirebaseConfig config;  // Stores API key and database URL


// ============================================================================
// PART 9: SETTINGS - THRESHOLDS AND TIMING
// ============================================================================
// Threshold values determine when automation actions happen.
// For example: turn on lights when it's darker than this value.

// lightThreshold: Light level below which relay 1 turns on (it gets dark)
// If light sensor reads less than 1500, it's dark - turn on the lights!
const int lightThreshold = 1500;

// gasThreshold: Gas level above which buzzer sounds alarm
// If gas sensor reads more than 750, there's a gas leak - beep the buzzer!
const int gasThreshold = 750;

// motionTimeout: How long (in milliseconds) to keep relay 2 on after motion stops
// 30000 milliseconds = 30 seconds
const unsigned long motionTimeout = 30000;

// Timing intervals: How often to do each task (in milliseconds)
// sensorInterval: Read sensors every 2000ms = 2 seconds
// firebaseInterval: Check/send to Firebase every 2000ms = 2 seconds
// lcdInterval: Update LCD screen every 1000ms = 1 second
const unsigned long sensorInterval = 2000;
const unsigned long firebaseInterval = 2000;
const unsigned long lcdInterval = 1000;


// ============================================================================
// PART 10: TIMER VARIABLES - KEEPING TRACK OF TIME
// ============================================================================
// We need to remember when we last did each task, so we don't do it too often.
// These store the "millis()" value (time since ESP32 started) from the last execution.

// previousSensorMillis: When we last read the sensors
unsigned long previousSensorMillis = 0;

// previousFirebaseMillis: When we last communicated with Firebase
unsigned long previousFirebaseMillis = 0;

// previousLCDMillis: When we last updated the LCD screen
unsigned long previousLCDMillis = 0;

// lastMotionTime: When motion was last detected (for turning off after timeout)
unsigned long lastMotionTime = 0;


// ============================================================================
// PART 11: VARIABLES - STORING SENSOR READINGS
// ============================================================================
// These variables store the current values from our sensors.
// We update them continuously in the loop.

// temp = Current temperature reading (in Celsius)
// hum = Current humidity reading (in percentage)
float temp = 0;
float hum = 0;

// lightVal = Current light level (0-4095 on ESP32 analog read)
// gasVal = Current gas level (0-4095 on ESP32 analog read)
// motion = Is motion detected? (0 = no, 1 = yes)
int lightVal = 0;
int gasVal = 0;
int motion = 0;

// r3State = Current on/off state of Relay 3 (from Firebase)
// r4State = Current on/off state of Relay 4 (from Firebase)
bool r3State = false;
bool r4State = false;


// ============================================================================
// PART 12: SETUP FUNCTION - RUNS ONCE WHEN ESP32 STARTS
// ============================================================================
// The setup() function runs exactly once when you power on the ESP32.
// This is where we initialize everything and get ready to run.
void setup() {

  // ==========================================================================
  // STEP 1: Start Serial Monitor (for debugging)
  // ==========================================================================
  // Serial.begin(115200) starts communication with computer's Serial Monitor
  // 115200 is the baud rate (communication speed)
  // This lets us see sensor readings on the computer screen
  Serial.begin(115200);


  // ==========================================================================
  // STEP 2: Set Pin Modes - Tell ESP32 if each pin is input or output
  // ==========================================================================
  // Pins connected to relays, LED, and buzzer need to be OUTPUTS (we control them)
  // Pins connected to sensors need to be INPUTS (they read values)
  // digitalWrite() will only work on pins set as OUTPUT

  // Set relay pins as OUTPUT (we control them with HIGH/LOW signals)
  pinMode(R1, OUTPUT);  // Relay 1
  pinMode(R2, OUTPUT);  // Relay 2
  pinMode(R3, OUTPUT);  // Relay 3
  pinMode(R4, OUTPUT);  // Relay 4

  // Set PIR motion sensor as INPUT (it sends signals to us)
  pinMode(PIR_PIN, INPUT);

  // Set status LED and buzzer as OUTPUT (we control them)
  pinMode(WIFI_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);


  // ==========================================================================
  // STEP 3: Set Initial States - Start with everything OFF
  // ==========================================================================
  // When ESP32 starts, we set all relays to OFF (LOW = 0 volts = off)
  // This prevents appliances from turning on unexpectedly
  digitalWrite(R1, LOW);  // Turn Relay 1 OFF
  digitalWrite(R2, LOW);  // Turn Relay 2 OFF
  digitalWrite(R3, LOW);  // Turn Relay 3 OFF
  digitalWrite(R4, LOW);  // Turn Relay 4 OFF

  digitalWrite(BUZZER, LOW);  // Turn buzzer OFF (don't beep yet)


  // ==========================================================================
  // STEP 4: Start the DHT Sensor
  // ==========================================================================
  // dht.begin() initializes the temperature/humidity sensor
  // Without this, the sensor won't work
  dht.begin();


  // ==========================================================================
  // STEP 5: Initialize the LCD Screen
  // ==========================================================================
  // lcd.init() sets up the LCD for use
  // lcd.backlight() turns on the backlight so we can see the screen
  lcd.init();
  lcd.backlight();

  // Display a welcome message on the LCD
  // setCursor(0, 0) moves to first column of first row
  // print() displays text on the screen
  lcd.setCursor(0, 0);
  lcd.print("Smart Switch");  // Show "Smart Switch" on top line


  // ==========================================================================
  // STEP 6: Connect to WiFi
  // ==========================================================================
  // WiFi.begin(ssid, password) tries to connect to the WiFi network
  // We use the credentials from lines 8-9 at the top of this file
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // ==========================================================================
  // WAIT for WiFi to connect (blink LED while waiting)
  // ==========================================================================
  // WiFi.status() returns WL_CONNECTED when成功 connected
  // While waiting, we blink the LED to show we're connecting
  while (WiFi.status() != WL_CONNECTED) {

    // Toggle the LED on/off to create blinking effect
    // digitalRead() gets current state, ! means NOT (toggle it)
    digitalWrite(WIFI_LED, !digitalRead(WIFI_LED));

    // Wait 300 milliseconds between blinks
    delay(300);
  }

  // WiFi is connected! Turn the LED on permanently
  digitalWrite(WIFI_LED, HIGH);

  // Clear the LCD and show "WiFi Connected"
  lcd.clear();
  lcd.print("WiFi Connected");


  // ==========================================================================
  // STEP 7: Connect to Firebase
  // ==========================================================================
  // First, store the API key in our config object
  config.api_key = API_KEY;

  // Store the database URL in our config object
  config.database_url = DATABASE_URL;

  // Set up the login credentials (email and password)
  // This is the user account that can control the Firebase database
  auth.user.email = "msrcasc.project@gmail.com";
  auth.user.password = "050809";

  // Initialize Firebase with our config and auth objects
  // This establishes the connection to Firebase
  Firebase.begin(&config, &auth);

  // Enable automatic WiFi reconnection if connection is lost
  // This helps maintain the connection over time
  Firebase.reconnectWiFi(true);

  // Wait 1 second for everything to settle
  delay(1000);

  // Clear the LCD - we're done with setup!
  lcd.clear();
}


// ============================================================================
// PART 13: LOOP FUNCTION - RUNS OVER AND OVER FOREVER
// ============================================================================
// The loop() function runs continuously after setup() finishes.
// This is where all the real work happens - reading sensors, controlling relays, etc.
// The ESP32 runs this loop forever as long as it's powered on.
void loop() {

  // Get the current time (milliseconds since ESP32 started)
  // We'll use this to decide when to do each task
  unsigned long currentMillis = millis();


  // ==========================================================================
  // TASK 1: Update WiFi Status LED
  // ==========================================================================
  // This LED shows if WiFi is connected or not
  // If WiFi.status() == WL_CONNECTED, write HIGH (on)
  // Otherwise, write LOW (off)
  // This runs every loop iteration (very fast)
  digitalWrite(WIFI_LED, WiFi.status() == WL_CONNECTED);


  // ==========================================================================
  // TASK 2: Read Sensors - Get Data from All Sensors
  // ==========================================================================
  // We read sensors every 2 seconds (not every loop - too fast!)
  // currentMillis - previousSensorMillis calculates time since last reading
  if (currentMillis - previousSensorMillis >= sensorInterval) {

    // Update the timer so we know when we read next
    previousSensorMillis = currentMillis;

    // ===== READ DHT11 TEMPERATURE & HUMIDITY =====
    // dht.readTemperature() returns temperature in Celsius
    // dht.readHumidity() returns humidity in percentage
    // These are built-in functions from the DHT library
    temp = dht.readTemperature();  // Read temperature
    hum = dht.readHumidity();      // Read humidity

    // ===== READ LIGHT SENSOR (LDR) =====
    // analogRead() reads an analog voltage (0-3.3V mapped to 0-4095)
    // Higher number = more light (0 = very dark, 4095 = very bright)
    // LDR: When light hits it, resistance decreases, voltage increases
    lightVal = analogRead(LDR_PIN);  // Read light level

    // ===== READ GAS SENSOR =====
    // MQ-series gas sensors output voltage based on gas concentration
    // Higher number = more gas detected
    gasVal = analogRead(GAS_PIN);  // Read gas level

    // ===== READ MOTION SENSOR (PIR) =====
    // digitalRead() reads a digital signal (0 or 1)
    // HIGH (or 1) = motion detected
    // LOW (or 0) = no motion
    motion = digitalRead(PIR_PIN);  // Read if motion detected


    // ===== PRINT TO SERIAL MONITOR (for debugging) =====
    // Serial.print() sends text to the computer via USB
    // This helps us see what the sensors are reading
    Serial.print("Temp: ");
    Serial.print(temp);

    Serial.print("  Humidity: ");
    Serial.print(hum);

    Serial.print("  Light: ");
    Serial.print(lightVal);

    Serial.print("  Gas: ");
    Serial.print(gasVal);

    Serial.print("  Motion: ");
    Serial.println(motion);  // println adds a new line at the end
  }


  // ==========================================================================
  // TASK 3: Light Automation - Control Relay 1 Based on Light Level
  // ==========================================================================
  // AUTOMATION RULE: If it's dark (light level < 1500), turn on the lights
  // If it's bright (light level >= 1500), turn off the lights

  // Check if light level is below our threshold (it's dark)
  if (lightVal < lightThreshold) {

    // It's dark - turn Relay 1 ON to activate lights
    digitalWrite(R1, HIGH);

  } else {

    // It's bright - turn Relay 1 OFF (no lights needed)
    digitalWrite(R1, LOW);
  }


  // ==========================================================================
  // TASK 4: Motion Automation - Control Relay 2 Based on Motion
  // ==========================================================================
  // AUTOMATION RULE: When motion is detected, turn on the lights
  // Keep lights on for 30 seconds after last motion, then turn off

  // Check if motion is detected
  if (motion == HIGH) {

    // Motion detected! Save the current time
    // This marks when we last saw movement
    lastMotionTime = currentMillis;

    // Turn Relay 2 ON (activate lights)
    digitalWrite(R2, HIGH);
  }

  // Check if we've waited long enough without motion
  // If current time minus last motion time > 30 seconds
  if (currentMillis - lastMotionTime > motionTimeout) {

    // No motion for 30 seconds - turn off the lights
    digitalWrite(R2, LOW);
  }


  // ==========================================================================
  // TASK 5: Gas Alert - Sound Buzzer When Gas Level is High
  // ==========================================================================
  // AUTOMATION RULE: If gas level exceeds threshold, sound the alarm

  // Check if gas level is above our danger threshold
  if (gasVal > gasThreshold) {

    // Gas detected! Turn the buzzer ON (it will beep)
    digitalWrite(BUZZER, HIGH);

  } else {

    // Gas level is normal - keep buzzer OFF
    digitalWrite(BUZZER, LOW);
  }


  // ==========================================================================
  // TASK 6: Firebase Control - Read Relay States from Firebase
  // ==========================================================================
  // Check Firebase every 2 seconds to see if we should control relays remotely
  if (currentMillis - previousFirebaseMillis >= firebaseInterval) {

    // Update the timer
    previousFirebaseMillis = currentMillis;

    // ===== CHECK IF CONNECTED AND GET DATA =====
    // Only try if WiFi is connected
    if (WiFi.status() == WL_CONNECTED &&
        Firebase.RTDB.getJSON(&fbdo, "/devices/switchboard1")) {

      // ===== PARSE THE JSON DATA =====
      // Get the JSON object from Firebase response
      FirebaseJson &json = fbdo.jsonObject();

      // Create a variable to extract each value
      FirebaseJsonData data;

      // ===== READ RELAY 3 STATE =====
      // Try to get "relay3" from the JSON
      // If it exists, save it to r3State
      if (json.get(data, "relay3")) {

        // Convert the value to integer and save
        r3State = data.to<int>();
      }

      // ===== READ RELAY 4 STATE =====
      // Try to get "relay4" from the JSON
      // If it exists, save it to r4State
      if (json.get(data, "relay4")) {

        // Convert the value to integer and save
        r4State = data.to<int>();
      }

      // ===== APPLY THE RELAY STATES =====
      // Write the states to the actual relays
      // Note: We only control Relay 3 and Relay 4 from Firebase
      // Relay 1 and Relay 2 are controlled automatically by sensors
      digitalWrite(R3, r3State);  // Apply Relay 3 state
      digitalWrite(R4, r4State);  // Apply Relay 4 state
    }


    // ==========================================================================
    // TASK 7: Send Sensor Data to Firebase
    // ==========================================================================
    // Create a JSON object to hold all our sensor readings
    FirebaseJson sensorData;

    // ===== ADD SENSOR READINGS =====
    // sensorData.set("name", value) adds data to the JSON
    sensorData.set("temperature", temp);      // Add temperature
    sensorData.set("humidity", hum);         // Add humidity
    sensorData.set("light", lightVal);      // Add light level
    sensorData.set("gas", gasVal);          // Add gas level
    sensorData.set("motion", motion);      // Add motion status

    // ===== ADD RELAY STATUSES =====
    // digitalRead() gets the current state of each relay
    sensorData.set("relay1", digitalRead(R1));  // Add Relay 1 state
    sensorData.set("relay2", digitalRead(R2));  // Add Relay 2 state
    sensorData.set("relay3", digitalRead(R3));  // Add Relay 3 state
    sensorData.set("relay4", digitalRead(R4));  // Add Relay 4 state

    // ===== UPLOAD TO FIREBASE =====
    // Firebase.RTDB.updateNode() sends data to Firebase
    // We send to "/devices/switchboard1" path in the database
    // Now you can see all sensor readings on the Firebase console!
    Firebase.RTDB.updateNode(
      &fbdo,                  // The Firebase object
      "/devices/switchboard1", // The path in database
      &sensorData              // The data to send
    );
  }


  // ==========================================================================
  // TASK 8: Update LCD Display
  // ==========================================================================
  // Update the LCD screen every 1 second to show current readings
  if (currentMillis - previousLCDMillis >= lcdInterval) {

    // Update the timer
    previousLCDMillis = currentMillis;

    // ===== CLEAR THE SCREEN =====
    // Clear any previous text
    lcd.clear();

    // ===== DISPLAY ON FIRST ROW =====
    // Row 0 is the first row (top)
    // Move to start of first row
    lcd.setCursor(0, 0);

    // Show temperature: "T:xx.x"
    lcd.print("T:");
    lcd.print(temp);

    // Show humidity after temperature
    lcd.print(" H:");
    lcd.print(hum);


    // ===== DISPLAY ON SECOND ROW =====
    // Row 1 is the second row (bottom)
    // Move to start of second row
    lcd.setCursor(0, 1);

    // Show light level: "L:xxxx"
    lcd.print("L:");
    lcd.print(lightVal);

    // Show gas level after light
    lcd.print(" G:");
    lcd.print(gasVal);
  }

  // ==========================================================================
  // END OF LOOP - It repeats from the beginning!
  // ==========================================================================
  // The loop() function repeats forever, checking sensors and updating outputs.
  // This is the main "heartbeat" of the smart home system.
}


// ============================================================================
// END OF PROGRAM
// ============================================================================
// Congratulations! You've read through the entire ESP32 Smart Home Switch code.
//
// SUMMARY OF WHAT THIS PROGRAM DOES:
// 1. Reads 5 sensors: Temperature, Humidity, Light, Gas, Motion
// 2. Controls 4 relays automatically or via Firebase
// 3. Displays readings on LCD screen
// 4. Sounds alarm if gas is detected
// 5. Sends all data to Firebase (viewable online)
//
// HOW TO USE:
// - Relay 1: Turns on automatically when it's dark
// - Relay 2: Turns on when motion is detected, off after 30 seconds
// - Relay 3: Control from Firebase (write 1 or 0)
// - Relay 4: Control from Firebase (write 1 or 0)
// - Buzzer: Sounds when gas level > 750
//
// Now your friends can understand this code too!
// ============================================================================