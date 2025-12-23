ESP32 – Azure IoT Hub Integration (ESP-IDF / PlatformIO)
📌 Overview

This project demonstrates how to connect an ESP32 device to Azure IoT Hub using MQTT over TLS, built with ESP-IDF and PlatformIO.

The device publishes sensor telemetry (temperature, humidity, etc.) securely to Azure IoT Hub and is designed following production-ready IoT practices:

Secure authentication

Time synchronization (SNTP)

Modular architecture

Secret-safe source control

This repository is intended as a portfolio project for IoT / Embedded / Cloud Engineering roles.

🧱 Architecture
ESP32
 ├─ WiFi
 ├─ SNTP (time sync)
 ├─ MQTT over TLS
 └─ Azure IoT Hub
        ├─ Device Identity
        ├─ SAS Token Auth
        └─ Telemetry Ingestion


Key points:

TLS requires correct system time → SNTP is mandatory

Azure IoT Hub uses SAS token–based authentication

Root CA certificate is embedded in firmware

🛠 Tech Stack

Hardware: ESP32

Framework: ESP-IDF

Build System: PlatformIO

Protocol: MQTT over TLS

Cloud: Azure IoT Hub

Language: C

OS: FreeRTOS

📂 Project Structure
.
├── main/                 # Application entry & logic 
│   ├── wifi/
│   ├── Oled/
|   ├── fonts/
|   ├── I2c/
│   ├── azure_iot/
│   └── sensors/
├── include/
│   ├── config.h          # Public config (NO secrets) 
├── certs/
│   └── azure_ca.pem      # Azure Root CA
├── platformio.ini
├── README.md 

🔐 Security & Secrets Handling

No secrets are committed to this repository

Azure IoT connection strings are stored locally only

config_local.h is excluded via .gitignore

GitHub Push Protection is respected

All leaked keys (if any) are rotated immediately

Example:

// include/config.h (safe to commit)
#define IOT_HUB_CONN_STR "<SET_IN_LOCAL_CONFIG>" 

⏱ Time Synchronization (Why It Matters)

Azure IoT Hub authentication uses SAS tokens with expiration time.

Therefore:

ESP32 must synchronize time using SNTP

Incorrect system time → MQTT authentication will fail

This project ensures time sync before connecting to Azure IoT Hub.

🚀 Features

✅ WiFi connection management

✅ SNTP time synchronization

✅ Secure MQTT connection to Azure IoT Hub

✅ Telemetry publishing

✅ Modular & scalable code structure

✅ Production-style secret management

🧪 How to Build & Run
1️⃣ Prerequisites

VS Code

PlatformIO extension

ESP32 board

Azure IoT Hub + registered device

2️⃣ Configure local secrets

Create:

include/config_local.h

#define IOT_HUB_CONN_STR "HostName=..."

3️⃣ Build & Upload
pio run
pio run --target upload

📈 Future Improvements

Device Twin support

Cloud-to-Device (C2D) commands

OTA firmware update

Telemetry batching

Azure DPS provisioning

👤 Author

Tien Nguyen
IoT / Embedded / Cloud Engineer
GitHub: https://github.com/zarack06