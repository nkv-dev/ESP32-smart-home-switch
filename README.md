# ESP32 Smart Home Switch

A WiFi-enabled smart home switch system built on ESP32, featuring 4-channel relay control, environmental monitoring, and Firebase integration for remote access.

## Features
- 4-channel relay control (local + Firebase remote)
- Real-time environmental monitoring: temperature, humidity, ambient light, gas levels, motion detection
- 16x2 I2C LCD display for sensor readings
- Smart automation: motion-activated lighting, auto-off after 30s, gas leak buzzer alerts
- WiFi connectivity with status LED

## Hardware Requirements
| Component | Quantity | Notes |
|-----------|----------|-------|
| ESP32 Development Board | 1 | Generic esp32dev |
| 4-Channel Relay Module | 1 | Active HIGH |
| DHT11 Sensor | 1 | Temperature/humidity |
| LDR (Light Dependent Resistor) | 1 | With 10kΩ pull-down resistor |
| MQ-Series Gas Sensor | 1 | MQ-2/MQ-5/MQ-135 |
| PIR Motion Sensor | 1 | HC-SR501 or similar |
| 16x2 I2C LCD | 1 | I2C address 0x27 |
| Buzzer | 1 | Active or passive |
| Connecting Wires | - | Jumper wires |
| Power Supply | 1 | 5V 2A minimum |

## Pin Configuration
| ESP32 Pin | Connected Component |
|-----------|---------------------|
| GPIO 25 | Relay 1 |
| GPIO 26 | Relay 2 |
| GPIO 27 | Relay 3 |
| GPIO 14 | Relay 4 |
| GPIO 4 | DHT11 Sensor |
| GPIO 34 | LDR (Analog Input) |
| GPIO 35 | Gas Sensor (Analog Input) |
| GPIO 13 | PIR Motion Sensor |
| GPIO 2 | WiFi Status LED |
| GPIO 15 | Buzzer |

## Software Requirements
- [PlatformIO](https://platformio.org/) (IDE or CLI)
- VS Code with PlatformIO extension (recommended)
- Dependencies (auto-installed via PlatformIO):
  - Firebase Arduino Client Library (deprecated, see Notes)
  - DHT sensor library
  - Adafruit Unified Sensor
  - LiquidCrystal_I2C @ ^1.1.4

## Wiring Guide
1. Connect relay module inputs to GPIO 25, 26, 27, 14 (VCC to 5V, GND to GND)
2. Connect DHT11 data pin to GPIO 4 (VCC to 3.3V, GND to GND)
3. Connect LDR voltage divider output to GPIO 34
4. Connect gas sensor output to GPIO 35
5. Connect PIR sensor output to GPIO 13 (VCC to 5V, GND to GND)
6. Connect I2C LCD SDA to GPIO 21, SCL to GPIO 22 (VCC to 5V, GND to GND)
7. Connect buzzer positive to GPIO 15, negative to GND
8. Connect WiFi LED (with 220Ω resistor) to GPIO 2, other end to GND

## Pre-Setup Configuration
Edit `src/main.cpp` to update the following:
1. **WiFi Credentials** (Lines 8-9):
   ```cpp
   #define WIFI_SSID "YOUR_WIFI_SSID"
   #define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
   ```
2. **Firebase Configuration** (Lines 12-13):
   ```cpp
   #define API_KEY "YOUR_FIREBASE_API_KEY"
   #define DATABASE_URL "YOUR_FIREBASE_DATABASE_URL"
   ```

> ⚠️ **Security Note**: Hardcoded credentials are a security risk. Move these to a `config.h` file and add it to `.gitignore` before committing.

## Build & Upload
### Using PlatformIO IDE (VS Code):
1. Open the project folder in VS Code
2. Click the PlatformIO icon in the sidebar
3. Under **Project Tasks > esp32dev**:
   - Click **Build** to compile
   - Click **Upload** to flash the ESP32
   - Click **Monitor** to view serial output (115200 baud)

### Using PlatformIO CLI:
```bash
cd "/home/user/Coding/esp32 project/Esp32-smart-home-switch"
pio run               # Build project
pio run -t upload     # Upload to ESP32
pio device monitor    # Open serial monitor
```

## Firebase Setup
1. Create a Firebase project at [Firebase Console](https://console.firebase.google.com/)
2. Enable Realtime Database in test mode
3. The ESP32 will automatically create the following database structure at `/devices/switchboard1`:
   ```json
   {
     "gas": 89,
     "humidity": 41.9,
     "light": 38,
     "motion": 0,
     "relay1": 1,
     "relay2": 0,
     "relay3": 0,
     "relay4": 1,
     "temperature": 34.5
   }
   ```
4. Write relay values (0=OFF, 1=ON) to `relay3` or `relay4` to control them remotely. Sensor data is automatically pushed every 2 seconds.

## Usage
- Control relays via Firebase Realtime Database writes
- View real-time sensor data in the Firebase console
- Smart automation rules:
  - Relay 1 (GPIO 25) turns on automatically when ambient light < 1500 (dark), off when bright
  - Relay 2 (GPIO 26) turns on when motion is detected, turns off 30 seconds after last motion
  - Buzzer activates when gas levels exceed 300
- LCD displays temperature, humidity, light, and gas readings in real-time

## Notes
- The Firebase Arduino Client library used in this project is **deprecated**. Migrate to [FirebaseClient](https://github.com/mobizt/FirebaseClient) for future updates.
- Default I2C LCD address is 0x27. Use an I2C scanner if your LCD address differs.
- Analog pins (34, 35) are input-only on ESP32, do not use for output.
