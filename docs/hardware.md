# Hardware & Architecture

## 🧠 Main Controller: ESP32
We use the **ESP32 DevKit V1 (30 pins)** for its dual-core performance and integrated wireless capabilities.

## 🔌 Core Pinout
| GPIO | Function      | Type                |
| ---- | ------------- | ------------------- |
| 2    | Status LED    | Digital Out         |
| 4    | Config Button | Digital In (Pullup) |
| 21   | I2C SDA       | Bus                 |
| 22   | I2C SCL       | Bus                 |

## 📦 Component Choice
- **IMU**: MPU-6050 (for stability and path tracking)
- **PWM**: PCA9685 (to offload timing-critical PWM from the ESP32)
- **OLED**: SSD1306 (for field telemetry)
