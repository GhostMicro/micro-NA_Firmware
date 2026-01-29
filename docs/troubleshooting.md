# Troubleshooting Guide

## 🔌 Connection Issues
- **Problem**: Serial port not found.
- **Solution**: Ensure your browser supports Web Serial (Chrome/Edge). Check if another app (like Arduino IDE) is using the port.

## 🔋 Power Issues
- **Problem**: ESP32 brownout (restarting).
- **Solution**: Ensure your battery can supply enough current for the motors. Use a separate BEC for the ESP32 logic.

## 📡 Control Issues
- **Problem**: Motors spin the wrong way.
- **Solution**: Check wiring or use the "Motor Reverse" toggle in the Setup tab.
