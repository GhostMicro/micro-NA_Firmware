# NA Firmware Build Guide

คู่มือการ Build Firmware สำหรับ ESP32-NA Framework

---

## 📋 ข้อกำหนดเบื้องต้น

### ซอฟต์แวร์ที่ต้องมี

- **PlatformIO Core** (CLI)
- **Python 3.x**
- **Git**

### ตรวจสอบการติดตั้ง

```bash
# ตรวจสอบ PlatformIO
~/.platformio/penv/bin/pio --version

# ตรวจสอบ Python
python3 --version

# ตรวจสอบ Git
git --version
```

---

## 🔧 ขั้นตอนการ Build

### 1. เข้าไปยัง Directory โปรเจกต์

```bash
cd /micro-NA_Firmware
```

### 2. Clean Build (ถ้าต้องการ Build ใหม่ทั้งหมด)

```bash
~/.platformio/penv/bin/pio run --target clean
```

### 3. Build Firmware

```bash
~/.platformio/penv/bin/pio run -e esp32dev
```

**คำอธิบายอย่างละเอียด:**

- **`pio run`**: เป็นคำสั่งหลักของ PlatformIO ในการเริ่มกระบวนการ "Build" โปรเจกต์
  - **การทำงาน**: มันจะทำการรวบรวมไฟล์ Source code (`.cpp`, `.h`), ตรวจสอบ Dependencies (`lib_deps`), และทำการ Compile จนกระทั่งได้ไฟล์ Binary (`.bin`)
- **`-e esp32dev`**: ย่อมาจาก `--environment`
  - **ความหมาย**: เป็นการบอก pio ว่าเราต้องการ Build สำหรับสภาพแวดล้อม (Environment) ไหน ซึ่งเรากำหนดค่าต่างๆ ไว้ในไฟล์ `platformio.ini` ภายใต้หัวข้อ `[env:esp32dev]`
  - **ทำไมต้องใส่?**: เพราะในหนึ่งโปรเจกต์เราอาจจะระบุได้หลายบอร์ดหรือหลายรูปแบบการตั้งค่า (เช่น `test_na_framework`) การระบุ `-e` ช่วยให้เจาะจงได้ทันที

---

### 4. ตรวจสอบไฟล์ที่ Build ได้

```bash
ls -lh .pio/build/esp32dev/firmware.bin
```

**Output ตัวอย่าง:**

```text
-rw-rw-r-- 1 devg devg 961K Jan 29 17:49 .pio/build/esp32dev/firmware.bin
```

**คำอธิบายคำสั่ง `ls -lh`**:
- `ls`: List files
- `-l`: Long format (แสดงรายละเอียด สิทธิ์ เจ้าของ ขนาด)
- `-h`: Human readable (แสดงขนาดเป็น K, M แทนที่จะเป็น Bytes)

---

### 5. Copy และ Rename ตามกฎการตั้งชื่อ

```bash
# รูปแบบ: ESP32-NA_V{version}_firmware_{YYYYMMDD}.bin
cp .pio/build/esp32dev/firmware.bin \
   docs/assets/firmware/ESP32-NA_V1.0.0_firmware_20250129.bin
```

**คำอธิบายคำสั่ง `cp`**:
- `cp [ต้นทาง] [ปลายทาง]`: Copy file จากตำแหน่งที่ pio build เสร็จ ไปยังคลังเก็บเอกสารของเรา พร้อมกับเปลี่ยนชื่อให้ตรงตามกฎที่เราตั้งไว้

---

### 6. ตรวจสอบไฟล์ที่บันทึก

```bash
ls -lh docs/assets/firmware/
```

---

## 🚀 7. การ Deploy ไปยัง Flasher

เนื่องจากตัว **micro-NA_Flasher** จะมองหาไฟล์ที่ชื่อเดียวคือ `firmware.bin` ในโฟลเดอร์ `public` ของมัน เราจึงต้องมีขั้นตอนการ "ส่งมอบ" เฟิร์มแวร์เพื่อใช้งานจริง

### ขั้นตอนการ Deploy (Manual)

```bash
cp docs/assets/firmware/ESP32-NA_V1.0.0_firmware_20250129.bin \
   ../micro-NA_Flasher/public/firmware.bin
```

**คำอธิบาย:**
- เรากำลัง Copy ไฟล์ที่เราเลือกแล้วว่า "พร้อมใช้งาน" ไปทับไฟล์ `firmware.bin` ตัวหลักใน Flasher
- เมื่อเราเปิดเว็บ Flasher ขึ้นมา มันจะโหลดตัวนี้ไป Flash ลงบอร์ดทันที

---

## 🚀 คำสั่งแบบรวม (One-liner)

### Build + Copy + Rename (แบบอัตโนมัติ)

```bash
cd /media/devg/Micro-SV7/GitHub/GhostMicro/micro-rn-platfrom/rn-firmware/micro-NA_Firmware && \
~/.platformio/penv/bin/pio run -e esp32dev && \
VERSION="1.0.0" && \
DATE=$(date +%Y%m%d) && \
cp .pio/build/esp32dev/firmware.bin \
   docs/assets/firmware/ESP32-NA_V${VERSION}_firmware_${DATE}.bin && \
ls -lh docs/assets/firmware/ESP32-NA_V${VERSION}_firmware_${DATE}.bin
```

**คำอธิบาย:**

1. เข้า Directory
2. Build firmware
3. กำหนดตัวแปร VERSION
4. สร้างตัวแปร DATE อัตโนมัติ (วันที่ปัจจุบัน)
5. Copy + Rename
6. แสดงผลไฟล์ที่สร้าง

---

## 📊 ตัวอย่าง Output การ Build

### Build สำเร็จ

```text
Processing esp32dev (platform: espressif32; board: esp32dev; framework: arduino)
-------------------------------------------------------------------------------------
Verbose mode can be enabled via `-v, --verbose` option
CONFIGURATION: https://docs.platformio.org/page/boards/espressif32/esp32dev.html
PLATFORM: Espressif 32 (6.12.0) > Espressif ESP32 Dev Module
HARDWARE: ESP32 240MHz, 320KB RAM, 4MB Flash

...

Linking .pio/build/esp32dev/firmware.elf
Checking size .pio/build/esp32dev/firmware.elf
Advanced Memory Usage is available via "PlatformIO Home > Project Inspect"
RAM:   [==        ]  15.7% (used 51356 bytes from 327680 bytes)
Flash: [=======   ]  74.5% (used 976917 bytes from 1310720 bytes)
Building .pio/build/esp32dev/firmware.bin
esptool.py v4.9.0
Creating esp32 image...
Successfully created esp32 image.
=========================================== [SUCCESS] Took 16.15 seconds ===========================================
```

### Build ล้มเหลว

```text
*** [.pio/build/esp32dev/src/main.cpp.o] Error 1
 [FAILED] Took 3.81 seconds
```

---

## 🔍 คำสั่งเพิ่มเติม

### ดู Build แบบละเอียด (Verbose)

```bash
~/.platformio/penv/bin/pio run -e esp32dev -v
```

### Build เฉพาะไฟล์ที่เปลี่ยนแปลง (Incremental Build)

```bash
~/.platformio/penv/bin/pio run -e esp32dev
```

(Default behavior - ไม่ต้องระบุ flag)

### Build ทั้งหมดใหม่ (Full Rebuild)

```bash
~/.platformio/penv/bin/pio run -e esp32dev --target clean
~/.platformio/penv/bin/pio run -e esp32dev
```

### ดูขนาด Memory Usage

```bash
~/.platformio/penv/bin/pio run -e esp32dev --target size
```

### Upload ลงบอร์ด (ถ้าต่อสาย USB)

```bash
~/.platformio/penv/bin/pio run -e esp32dev --target upload
```

### Monitor Serial Output

```bash
~/.platformio/penv/bin/pio device monitor -b 115200
```

---

## 🛠️ การแก้ปัญหา

### ปัญหา: `pio: command not found`

**วิธีแก้:**

```bash
# ใช้ full path
~/.platformio/penv/bin/pio run -e esp32dev

# หรือเพิ่ม alias ใน ~/.bashrc
echo 'alias pio="~/.platformio/penv/bin/pio"' >> ~/.bashrc
source ~/.bashrc
```

### ปัญหา: `NAPacket.h: No such file or directory`

**วิธีแก้:**
ตรวจสอบ `platformio.ini` ว่ามี:

```ini
lib_extra_dirs = ../../na-shared
```

### ปัญหา: Build ช้า

**วิธีแก้:**

```bash
# เพิ่ม -j flag สำหรับ parallel compilation
~/.platformio/penv/bin/pio run -e esp32dev -j 4
```

### ปัญหา: Out of Memory

**วิธีแก้:**
ลด Library หรือ Code ที่ไม่ใช้

---

## 📝 Checklist ก่อน Build

- [ ] Pull code ล่าสุดจาก Git
- [ ] ตรวจสอบ `platformio.ini` ถูกต้อง
- [ ] ตรวจสอบ Dependencies ใน `lib_deps`
- [ ] Clean build directory (ถ้าจำเป็น)
- [ ] เตรียม Version number และ Date

---

## 🎯 Quick Reference

| คำสั่ง                       | คำอธิบาย             |
| ------------------------- | ------------------ |
| `pio run -e esp32dev`     | Build firmware     |
| `pio run --target clean`  | ลบไฟล์ build        |
| `pio run -v`              | Build แบบ verbose  |
| `pio run --target upload` | Upload ลงบอร์ด      |
| `pio device monitor`      | เปิด Serial Monitor |
| `pio run --target size`   | ดูขนาด memory       |

---

## 📦 ตำแหน่งไฟล์สำคัญ

```text
micro-NA_Firmware/
├── .pio/build/esp32dev/
│   └── firmware.bin              # ไฟล์ที่ Build ได้
├── docs/assets/firmware/
│   └── ESP32-NA_V*.bin           # ไฟล์ที่บันทึกตามกฎ
├── src/                          # Source code
├── platformio.ini                # Build configuration
└── BUILD_GUIDE.md               # ไฟล์นี้
```

---

## 🔗 เอกสารที่เกี่ยวข้อง

- [README.md](README.md) - ข้อมูลทั่วไป
- [docs/assets/firmware/readme.md](docs/assets/firmware/readme.md) - กฎการบันทึก Firmware
- [docs/firmware-build-rules.md](docs/firmware-build-rules.md) - Quick Reference

---

**Last Updated:** 2025-01-29  
**Maintained by:** GhostMicro RN Foundation
