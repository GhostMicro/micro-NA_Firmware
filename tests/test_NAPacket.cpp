/**
 * Unit Tests for NAPacket
 * Tests CRC-16, packet validation, and serialization
 *
 * @file test_NAPacket.cpp
 * @framework Unity Test Framework (PlatformIO)
 */

#include <NAPacket.h>
#include <string.h>
#include <unity.h>

// ============================================================================
// Test Fixtures
// ============================================================================

NAPacket testPacket;
NATelemetry testTelemetry;

void setUp(void) {
  // Initialize test packet with known values
  memset(&testPacket, 0, sizeof(NAPacket));
  testPacket.protocolVersion = PROTOCOL_VERSION;
  testPacket.vehicleType = VEHICLE_TYPE_ROVER;
  testPacket.throttle = 500;
  testPacket.roll = -200; // Steering left
  testPacket.mode = MODE_ARMED;
  testPacket.buttons = 0x00;
  testPacket.sequenceNumber = 1;

  // Initialize test telemetry
  memset(&testTelemetry, 0, sizeof(NATelemetry));
  testTelemetry.protocolVersion = PROTOCOL_VERSION;
  testTelemetry.batteryVoltage = 12.5f;
  testTelemetry.rssi = -65;
  testTelemetry.uptime = 5000;
  testTelemetry.status = 0;
}

void tearDown(void) {
  // No cleanup needed
}

// ============================================================================
// CRC-16 Tests
// ============================================================================

void test_CRC16_consistency(void) {
  // Same data should produce same CRC
  uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
  uint16_t crc1 = NA_CRC16(data, sizeof(data));
  uint16_t crc2 = NA_CRC16(data, sizeof(data));

  TEST_ASSERT_EQUAL_UINT16(crc1, crc2);
}

void test_CRC16_different_data(void) {
  // Different data should produce different CRC (with high probability)
  uint8_t data1[] = {0x01, 0x02, 0x03, 0x04, 0x05};
  uint8_t data2[] = {0x01, 0x02, 0x03, 0x04, 0x06};

  uint16_t crc1 = NA_CRC16(data1, sizeof(data1));
  uint16_t crc2 = NA_CRC16(data2, sizeof(data2));

  TEST_ASSERT_NOT_EQUAL(crc1, crc2);
}

void test_CRC16_empty_data(void) {
  // Empty buffer should produce CRC of 0xFFFF (initial value)
  uint16_t crc = NA_CRC16((const uint8_t *)nullptr, 0);
  // Actually, this will be FFFF since we don't process any bytes
  TEST_ASSERT_EQUAL_UINT16(0xFFFF, crc);
}

void test_CRC16_packet(void) {
  // Compute CRC of first 14 bytes (excluding checksum)
  uint16_t crc = NA_CRC16_PACKET(&testPacket);
  TEST_ASSERT_GREATER_THAN(0, crc);
  // CRC should be deterministic - same for same data
  uint16_t crc2 = NA_CRC16_PACKET(&testPacket);
  TEST_ASSERT_EQUAL_UINT16(crc, crc2);
}

// ============================================================================
// Packet Validation Tests
// ============================================================================

void test_packet_checksum_update(void) {
  NA_UPDATE_PACKET_CHECKSUM(&testPacket);

  // Checksum should be set
  TEST_ASSERT_NOT_EQUAL(0, testPacket.checksum);

  // Verify the checksum
  TEST_ASSERT_TRUE(NA_VERIFY_PACKET(&testPacket));
}

void test_packet_validation_invalid_version(void) {
  testPacket.protocolVersion = 0x99; // Invalid version
  NA_UPDATE_PACKET_CHECKSUM(&testPacket);

  // Should fail validation due to wrong protocol version
  TEST_ASSERT_FALSE(NA_PACKET_IS_VALID(&testPacket));
}

void test_packet_validation_corrupted_crc(void) {
  NA_UPDATE_PACKET_CHECKSUM(&testPacket);

  // Corrupt the checksum
  testPacket.checksum ^= 0xFFFF;

  // Should fail validation
  TEST_ASSERT_FALSE(NA_PACKET_IS_VALID(&testPacket));
}

void test_packet_validation_valid(void) {
  // Set correct checksum
  NA_UPDATE_PACKET_CHECKSUM(&testPacket);

  // Should pass validation
  TEST_ASSERT_TRUE(NA_PACKET_IS_VALID(&testPacket));
}

void test_packet_validate_strict_all_vehicles(void) {
  NA_UPDATE_PACKET_CHECKSUM(&testPacket);

  // Test all valid vehicle types
  for (int vtype = 1; vtype <= 4; vtype++) {
    testPacket.vehicleType = vtype;
    NA_UPDATE_PACKET_CHECKSUM(&testPacket);
    TEST_ASSERT_TRUE(NA_VALIDATE_PACKET_STRICT(&testPacket));
  }
}

void test_packet_validate_strict_invalid_vehicle(void) {
  NA_UPDATE_PACKET_CHECKSUM(&testPacket);

  testPacket.vehicleType = 0; // Invalid
  NA_UPDATE_PACKET_CHECKSUM(&testPacket);
  TEST_ASSERT_FALSE(NA_VALIDATE_PACKET_STRICT(&testPacket));

  testPacket.vehicleType = 5; // Invalid
  NA_UPDATE_PACKET_CHECKSUM(&testPacket);
  TEST_ASSERT_FALSE(NA_VALIDATE_PACKET_STRICT(&testPacket));
}

// ============================================================================
// Telemetry Tests
// ============================================================================

void test_telemetry_checksum_update(void) {
  NA_UPDATE_TELEMETRY_CHECKSUM(&testTelemetry);

  TEST_ASSERT_NOT_EQUAL(0, testTelemetry.checksum);
  TEST_ASSERT_TRUE(NA_VERIFY_TELEMETRY(&testTelemetry));
}

void test_telemetry_validation_valid(void) {
  NA_UPDATE_TELEMETRY_CHECKSUM(&testTelemetry);

  TEST_ASSERT_TRUE(NA_TELEMETRY_IS_VALID(&testTelemetry));
}

void test_telemetry_validation_invalid_version(void) {
  testTelemetry.protocolVersion = 0x99;
  NA_UPDATE_TELEMETRY_CHECKSUM(&testTelemetry);

  TEST_ASSERT_FALSE(NA_TELEMETRY_IS_VALID(&testTelemetry));
}

// ============================================================================
// Packet Structure Tests
// ============================================================================

void test_packet_size(void) {
  // NAPacket must be exactly 20 bytes
  TEST_ASSERT_EQUAL_INT(20, sizeof(NAPacket));
}

void test_telemetry_size(void) {
  // NATelemetry must be exactly 22 bytes (plaintext)
  TEST_ASSERT_EQUAL_INT(22, sizeof(NATelemetry));
}

void test_packet_packed_attribute(void) {
  // Verify no padding was added
  TEST_ASSERT_EQUAL_INT(1 + 1 + 2 + 2 + 2 + 2 + 1 + 1 + 2 + 4,
                        sizeof(NAPacket));
}

// ============================================================================
// Test Runner
// ============================================================================

int main(int argc, char **argv) {
  UNITY_BEGIN();

  // CRC Tests
  RUN_TEST(test_CRC16_consistency);
  RUN_TEST(test_CRC16_different_data);
  RUN_TEST(test_CRC16_empty_data);
  RUN_TEST(test_CRC16_packet);

  // Packet Validation Tests
  RUN_TEST(test_packet_checksum_update);
  RUN_TEST(test_packet_validation_invalid_version);
  RUN_TEST(test_packet_validation_corrupted_crc);
  RUN_TEST(test_packet_validation_valid);
  RUN_TEST(test_packet_validate_strict_all_vehicles);
  RUN_TEST(test_packet_validate_strict_invalid_vehicle);

  // Telemetry Tests
  RUN_TEST(test_telemetry_checksum_update);
  RUN_TEST(test_telemetry_validation_valid);
  RUN_TEST(test_telemetry_validation_invalid_version);

  // Structure Tests
  RUN_TEST(test_packet_size);
  RUN_TEST(test_telemetry_size);
  RUN_TEST(test_packet_packed_attribute);

  return UNITY_END();
}
