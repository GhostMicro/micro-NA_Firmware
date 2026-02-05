#!/bin/bash

# Script: deploy_to_flasher.sh
# Purpose: Deploy a specific firmware version to micro-NA_Flasher

if [ -z "$1" ]; then
    echo "❌ กรุณาระบุชื่อไฟล์เฟิร์มแวร์ที่อยู่ใน docs/assets/firmware/"
    echo "ตัวอย่าง: ./deploy_to_flasher.sh ESP32-NA_V1.0.0_firmware_20250129.bin"
    exit 1
fi

SOURCE_FILE="docs/assets/firmware/$1"
TARGET_FILE="../micro-NA_Flasher/public/firmware.bin"

if [ "$1" == "--link" ]; then
    echo "🔗 กำลังสร้าง Link ไปยังโฟลเดอร์ Firmware..."
    ln -sf "$(pwd)/docs/assets/firmware" "../micro-NA_Flasher/public/firmware_source"
    if [ $? -eq 0 ]; then
        echo "✅ Link สำเร็จ! Flasher จะดึงไฟล์จาก docs/assets/firmware โดยตรง"
        exit 0
    else
        echo "❌ เกิดข้อผิดพลาดในการสร้าง Link"
        exit 1
    fi
fi

if [ ! -f "$SOURCE_FILE" ]; then
    echo "❌ ไม่พบไฟล์: $SOURCE_FILE"
    exit 1
fi

echo "🚀 กำลัง Deploy: $1..."
cp "$SOURCE_FILE" "$TARGET_FILE"

if [ $? -eq 0 ]; then
    echo "✅ Deploy สำเร็จ! ไฟล์พร้อมใช้งานที่ micro-NA_Flasher/public/firmware.bin"
else
    echo "❌ เกิดข้อผิดพลาดในการ Copy"
    exit 1
fi
