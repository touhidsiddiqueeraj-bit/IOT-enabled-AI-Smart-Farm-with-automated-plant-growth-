# IOT Enabled AI Smart Farm with Automated Plant Growth

This repository contains the complete firmware and documentation for an AI-powered automated farming system built on the **ESP32-S3 WROOM**. This project was developed as a Final Year Project for the Department of EEE, Leading University, Sylhet.

## 🚀 Overview
The system integrates **TinyML (Edge Impulse)** with IoT to monitor plant health and automate environmental conditions. It uses an ESP32-S3 to process real-time imagery for nutrient deficiency detection while managing irrigation and humidification via a shared reservoir system.

## 🛠️ Hardware Features
- **Processor:** ESP32-S3 WROOM (8MB PSRAM).
- **Sensors:** DHT11 (Temp/Hum), Capacitive Soil Moisture v1.2.
- **Actuators:** 4-Channel Relay (Water Pump, Humidifier, Grow Lights, Fans).
- **Power:** 12V 7A Lead-Acid Battery with LM2596 Buck Converter.

## 💻 Software Features
- **AI Inference:** Real-time detection of Potassium and Sodium deficiencies.
- **Dual-Core Processing:** Core 0 for MJPEG Video Streaming; Core 1 for Sensor Logic & Control.
- **WiFi Manager:** Captive portal for easy network configuration.
- **Web Dashboard:** Real-time monitoring and manual override portal.

## 📂 Repository Structure
- `/Firmware`: Main Arduino/C++ source code.
- `/Schematics`: Circuit diagrams and pin mapping.
- `/AI_Model`: Edge Impulse deployment files.
- `/Documentation`: Project report and cost analysis.

## 🔧 Setup
1. Clone the repository.
2. Install the `DHT`, `ESP_Camera`, and `WiFiManager` libraries in Arduino IDE.
3. Configure the ESP32-S3 for "OPI PSRAM".
4. Upload the code and connect to the "SmartFarm-Setup" WiFi AP.

## ⚖️ License
This project is for academic purposes under the EEE-4800 course requirements.
