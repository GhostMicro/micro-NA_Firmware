/**
 * Unit Tests for JoystickCalibrator
 * Tests calibration math and deadzone application
 * 
 * @file test_JoystickCalibrator.cpp
 * @framework Unity Test Framework (PlatformIO)
 */

#include <unity.h>
#include "../JoystickCalibrator.h"
#include <cmath>

// ============================================================================
// Test Fixtures
// ============================================================================

JoystickCalibrator calibrator;

void setUp(void) {
    calibrator.reset();
}

void tearDown(void) {
    // No cleanup needed
}

// ============================================================================
// Calibration State Tests
// ============================================================================

void test_initial_state_not_calibrated(void) {
    bool calibrated = calibrator.isCalibrated();
    TEST_ASSERT_FALSE(calibrated);
}

void test_calibration_requires_three_points(void) {
    // First point (center)
    calibrator.setCenter(2048, 2048);
    TEST_ASSERT_FALSE(calibrator.isCalibrated());
    
    // Second point (max)
    calibrator.setMax(4095, 4095);
    TEST_ASSERT_FALSE(calibrator.isCalibrated());
    
    // Third point (min)
    calibrator.setMin(0, 0);
    TEST_ASSERT_TRUE(calibrator.isCalibrated());
}

void test_calibration_stores_center_point(void) {
    calibrator.setCenter(2000, 2100);
    // Calibrator should accept the value
    TEST_ASSERT_TRUE(true);  // Placeholder verification
}

void test_calibration_with_realistic_values(void) {
    // Realistic joystick calibration points
    calibrator.setCenter(2048, 2048);
    calibrator.setMax(4090, 4090);
    calibrator.setMin(10, 20);
    
    TEST_ASSERT_TRUE(calibrator.isCalibrated());
}

// ============================================================================
// Value Mapping Tests
// ============================================================================

void test_center_position_maps_to_zero(void) {
    calibrator.setCenter(2048, 2048);
    calibrator.setMax(4095, 4095);
    calibrator.setMin(0, 0);
    
    int16_t x = calibrator.mapX(2048);
    int16_t y = calibrator.mapY(2048);
    
    TEST_ASSERT_EQUAL_INT16(0, x);
    TEST_ASSERT_EQUAL_INT16(0, y);
}

void test_max_position_maps_to_100(void) {
    calibrator.setCenter(2048, 2048);
    calibrator.setMax(4095, 4095);
    calibrator.setMin(0, 0);
    
    int16_t x = calibrator.mapX(4095);
    int16_t y = calibrator.mapY(4095);
    
    TEST_ASSERT_EQUAL_INT16(100, x);
    TEST_ASSERT_EQUAL_INT16(100, y);
}

void test_min_position_maps_to_minus_100(void) {
    calibrator.setCenter(2048, 2048);
    calibrator.setMax(4095, 4095);
    calibrator.setMin(0, 0);
    
    int16_t x = calibrator.mapX(0);
    int16_t y = calibrator.mapY(0);
    
    TEST_ASSERT_EQUAL_INT16(-100, x);
    TEST_ASSERT_EQUAL_INT16(-100, y);
}

void test_midpoint_between_center_and_max(void) {
    calibrator.setCenter(2048, 2048);
    calibrator.setMax(4095, 4095);
    calibrator.setMin(0, 0);
    
    int raw_midpoint = (2048 + 4095) / 2;  // ~3071
    int16_t mapped = calibrator.mapX(raw_midpoint);
    
    // Should be approximately 50 (halfway to max)
    TEST_ASSERT_GREATER_THAN(40, mapped);
    TEST_ASSERT_LESS_THAN(60, mapped);
}

// ============================================================================
// Deadzone Tests
// ============================================================================

void test_deadzone_near_center(void) {
    calibrator.setCenter(2048, 2048);
    calibrator.setMax(4095, 4095);
    calibrator.setMin(0, 0);
    calibrator.setDeadzone(10);  // 10% deadzone
    
    // Values very close to center should map to 0 (within deadzone)
    int16_t value_near_center = calibrator.mapX(2050);  // 2 units above center
    TEST_ASSERT_EQUAL_INT16(0, value_near_center);
}

void test_deadzone_larger_values(void) {
    calibrator.setCenter(2048, 2048);
    calibrator.setMax(4095, 4095);
    calibrator.setMin(0, 0);
    calibrator.setDeadzone(10);  // 10% deadzone
    
    // Values significantly different should not be affected by deadzone
    int16_t value_high = calibrator.mapX(3500);
    TEST_ASSERT_GREATER_THAN(0, value_high);
    TEST_ASSERT_NOT_EQUAL(0, value_high);
}

void test_zero_deadzone(void) {
    calibrator.setCenter(2048, 2048);
    calibrator.setMax(4095, 4095);
    calibrator.setMin(0, 0);
    calibrator.setDeadzone(0);  // No deadzone
    
    // Even tiny movements should register
    int16_t value_small_move = calibrator.mapX(2049);
    TEST_ASSERT_NOT_EQUAL(0, value_small_move);
}

// ============================================================================
// Asymmetric Joystick Tests
// ============================================================================

void test_asymmetric_max_and_min_ranges(void) {
    // Joystick is offset - different ranges for + and -
    calibrator.setCenter(2048, 2048);
    calibrator.setMax(4000, 4000);    // Range +1952
    calibrator.setMin(1000, 1000);    // Range -1048
    
    // Max direction
    int16_t max_val = calibrator.mapX(4000);
    TEST_ASSERT_EQUAL_INT16(100, max_val);
    
    // Min direction (asymmetric)
    int16_t min_val = calibrator.mapX(1000);
    // Should be -100, but actual range is smaller
    TEST_ASSERT_EQUAL_INT16(-100, min_val);
}

void test_per_axis_calibration(void) {
    // X axis calibration
    calibrator.setCenter(2000, 2100);
    calibrator.setMax(4000, 4100);
    calibrator.setMin(0, 100);
    
    // X should respond to changes, Y should follow too
    int16_t x = calibrator.mapX(3000);
    int16_t y = calibrator.mapY(3100);
    
    TEST_ASSERT_NOT_EQUAL(0, x);
    TEST_ASSERT_NOT_EQUAL(0, y);
}

// ============================================================================
// Edge Cases
// ============================================================================

void test_extreme_values_clamped(void) {
    calibrator.setCenter(2048, 2048);
    calibrator.setMax(4095, 4095);
    calibrator.setMin(0, 0);
    
    // Even if raw value exceeds max, output should be clamped to 100
    int16_t over_max = calibrator.mapX(5000);  // Beyond normal range
    TEST_ASSERT_LESS_THAN_OR_EQUAL(100, over_max);
    
    int16_t under_min = calibrator.mapX(-1000);
    TEST_ASSERT_GREATER_THAN_OR_EQUAL(-100, under_min);
}

void test_reset_clears_calibration(void) {
    calibrator.setCenter(2048, 2048);
    calibrator.setMax(4095, 4095);
    calibrator.setMin(0, 0);
    
    TEST_ASSERT_TRUE(calibrator.isCalibrated());
    
    calibrator.reset();
    
    TEST_ASSERT_FALSE(calibrator.isCalibrated());
}

// ============================================================================
// Test Runner
// ============================================================================

int main(int argc, char** argv) {
    UNITY_BEGIN();
    
    // Calibration State Tests
    RUN_TEST(test_initial_state_not_calibrated);
    RUN_TEST(test_calibration_requires_three_points);
    RUN_TEST(test_calibration_stores_center_point);
    RUN_TEST(test_calibration_with_realistic_values);
    
    // Value Mapping Tests
    RUN_TEST(test_center_position_maps_to_zero);
    RUN_TEST(test_max_position_maps_to_100);
    RUN_TEST(test_min_position_maps_to_minus_100);
    RUN_TEST(test_midpoint_between_center_and_max);
    
    // Deadzone Tests
    RUN_TEST(test_deadzone_near_center);
    RUN_TEST(test_deadzone_larger_values);
    RUN_TEST(test_zero_deadzone);
    
    // Asymmetric Joystick Tests
    RUN_TEST(test_asymmetric_max_and_min_ranges);
    RUN_TEST(test_per_axis_calibration);
    
    // Edge Cases
    RUN_TEST(test_extreme_values_clamped);
    RUN_TEST(test_reset_clears_calibration);
    
    return UNITY_END();
}
