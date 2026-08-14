# Meteoria

Android/AOSP hardware integration project connecting an **ESP32 weather station** to Android through a custom Linux USB driver, native C++ components and Binder/AIDL.

## Architecture

```text
ESP32 + Sensors
      │ USB
      ▼
Linux Kernel Driver (C)
      │ sysfs
      ▼
Native C++ Layer
      │
      ▼
AIDL / Binder Service
      │
      ▼
Android Manager / Client
```

## Tech Stack

* AOSP / Android 13
* C / C++
* Linux Kernel Module
* USB / CP2102
* AIDL & Binder IPC
* SELinux
* Android Soong (`Android.bp`)
* ESP32

## Highlights

* Custom Linux USB kernel driver using bulk transfers
* VINTF-stable AIDL hardware interface
* Native C++ Android components
* Binder IPC between system components
* Android Manager API using `ServiceManager`
* SELinux policies for the vendor service and hardware interface
* ESP32 firmware and sensor integration

## Project Structure

```text
interfaces/weatherstation/      AIDL interface
weatherstation-module/          Linux USB driver
weatherstation_lib/             Native C++ hardware layer
weatherstation_service_client/  Native Binder client
WeatherstationManager/          Android Manager API
sepolicy/                       SELinux policies
app/                            Android application
```

## About

Built as part of the **DevTITANS Android Embedded Systems program**.

I implemented the complete integration stack, from the ESP32 firmware and Linux driver to the Android AIDL/Binder interface and client components.
