# Mobile Android-Embedded Weather Station 🌦️

A high-performance embedded system that integrates a **NodeMCU ESP32** with the **Android ecosystem** to provide real-time environmental monitoring. This project demonstrates low-level hardware integration, data acquisition, and Serial-over-USB communication.

## 🛠️ Technical Overview

This station collects data from a diverse array of sensors and transmits it to an Android device via a stable USB interface. It showcases the bridge between **Embedded C** and **Android System** logic, mirroring the complexities of real-world mobile hardware integration.

### Key Features
- **Real-time Data Acquisition:** Monitors ambient light, air/soil humidity, barometric pressure, temperature, and precipitation.
- **Robust Communication:** Implements a custom Serial-over-USB protocol for data integrity between the MCU and the Android application.
- **Low-Level Engineering:** Optimized C code for the ESP32 to handle sensor polling and power efficiency.

## 🧱 Hardware Stack

- **Microcontroller:** NodeMCU ESP32 (Xtensa® Dual-Core 32-bit LX6).
- **Communication:** USB-to-Serial interface.
- **Sensors Integrated:**
  - **Luminosity:** LDR (Light Dependent Resistor).
  - **Air Temperature & Humidity:** DHT series.
  - **Soil Moisture:** Capacitive/Resistive soil sensor.
  - **Barometric Pressure:** BMP/BME sensor.
  - **Precipitation:** Rain gauge sensor.

## 💻 Software & Architecture

- **Firmware:** Developed in **C/C++** focusing on asynchronous sensor reading.
- **Protocol:** Serial data stream optimized for mobile device parsing.
- **Android Integration:** Handles USB Host mode to receive and visualize sensor telemetry.

## 🚀 How it Works

1. The **ESP32** initializes the sensor array and establishes a Serial connection.
2. Data is encapsulated into a lightweight packet format.
3. The **Android device** acts as a host, reading the stream via a USB-OTG or standard USB interface.
4. The system provides immediate feedback on environmental changes, ideal for mobile agricultural or research applications.

---
*Developed as part of my deep-dive into Hardware-Software integration and Embedded Systems.*

How to install Multi-HAL components on Android 13:
hals.conf goes to ./out/target/product/emulator_x86_64/vendor/etc/sensors/hals.conf and ./device/devtitans/teia/hals.conf

tests folder goes to /home/devtitans-2/aosp/hardware/interfaces/sensors/common/default/2.X/multihal/

after compiling, a file named android.hardware.sensors@2.X-devsensors-config2.so will be generated at /home/devtitans-2/aosp/out/target/product/teia/vendor/lib64/
copy that file to /home/devtitans-2/aosp/out/target/product/emulator_x86_64/vendor/lib64/ so that the multihal can be loaded in the emulator
