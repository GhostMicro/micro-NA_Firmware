# NA Framework (Navigation Autonomous Framework)

The ultimate firmware for autonomous navigation, built on ESP32, designed for high-performance motor control and intelligence for Rovers, Boats, and Planes.

---

## 🛠️ Technical Specifications

- **Microcontroller**: ESP32 DevKit V1 (Dual Core, 240MHz)
- **Communication Protocols**:
  - **Serial**: Web Serial API with Short-Key JSON (`{"c": "..."}`)
  - **Wireless**: ESP-NOW (Binary Struct)
- **I2C Bus**: SDA (21), SCL (22)
- **Supported Sensors**: MPU-6050 (Address 0x68)
- **Actuator Controller**: PCA9685 (Address 0x40)

---

## 🔌 Pin Mapping (Standardized)

| Pin         | Function      | Description                     |
| ----------- | ------------- | ------------------------------- |
| **GPIO 2**  | Status LED    | Heartbeat / Connection Status   |
| **GPIO 4**  | Config Button | Reset to defaults / Entry point |
| **GPIO 21** | I2C SDA       | Data line for MPU, PCA, OLED    |
| **GPIO 22** | I2C SCL       | Clock line for MPU, PCA, OLED   |

---

## 📡 Serial Protocol (Short-Key JSON)

The NA Framework uses a minimalist JSON protocol to save RAM and ensure low latency.

### Example Commands

- `{"c": "sm", "t": 50, "s": 0}`: Set Motor (Throttle 50%, Steering 0)
- `{"c": "sp", "ax": 0, "p": 45.0}`: Set PID (Axis Roll, P=45.0)
- `{"c": "ca"}`: Calibrate Accelerometer

---

## 🚀 Getting Started

1. Open in **VS Code** with **PlatformIO** extension.
2. Select your environment (usually `esp32dev`).
3. Click **Upload** to flash the firmware.
4. Use the [NA Mission Configurator](https://github.com/GhostMicro/micro-rn-platfrom/tree/main/rn-configurator) to tune your vehicle.

---

**Standard:** NA_BLUEPRINT.md v2026.1.1
