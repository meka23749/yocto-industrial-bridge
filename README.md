# Industrial Protocol Bridge (Yocto ARM64)

A Yocto-based embedded Linux system that bridges legacy industrial protocols (Modbus TCP) to modern IoT protocols (MQTT), enabling Industry 4.0 connectivity.

## What it does
- Reads sensor data from industrial devices via Modbus TCP
- Converts and publishes data to MQTT broker
- Provides a REST API dashboard for real-time monitoring
- Alerts when sensor values exceed configured thresholds

## Architecture

Modbus TCP Devices        MQTT Broker / Cloud
(sensors, PLCs)              (mosquitto)
|                           ^
v                           |
+-----------+    +-----------+   |
| Modbus    |--->| Bridge    |---+
| Client    |    | Engine    |
+-----------+    +-----------+
|
+-----------+
| REST API  |
| Dashboard |
+-----------+

## Tech Stack
- **OS**: Custom Yocto Linux (Scarthgap) for ARM64
- **Application**: C (Modbus client, bridge engine, MQTT publisher)
- **Dashboard**: Python Flask REST API
- **Build**: Dockerized Yocto build environment
- **CI/CD**: GitHub Actions

## Target
- QEMU ARM64 (emulated ARM Cortex-A53)
- Portable to real hardware 

## Author
Steve Meka
