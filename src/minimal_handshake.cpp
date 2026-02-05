#include <Arduino.h>

// [REAL HARDWARE HANDSHAKE v0.0.1]
// บันทึก: ตรวจสอบ Built-in LED Pin สำหรับ S3 (ปกติ 2 หรือ 48)
#define LED_PIN 2 

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  
  delay(2000); // รอให้ Serial Bridge พร้อม
  
  Serial.println("\n\n========================================");
  Serial.println("🚀 GHOSTMICRO: REAL HARDWARE DETECTED");
  Serial.println("STATUS: [PHASE 0 - HANDSHAKE SUCCESS]");
  Serial.println("========================================\n");
}

void loop() {
  // Heartbeat Blink
  digitalWrite(LED_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);
  delay(900);
  
  // Status Ping
  Serial.printf("[HEARTBEAT] Uptime: %lu ms\n", millis());
}
