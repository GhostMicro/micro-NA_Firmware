/**
 * Unit Tests for FailsafeManager
 * Tests state machine transitions and timeout behavior
 * 
 * @file test_FailsafeManager.cpp
 * @framework Unity Test Framework (PlatformIO)
 */

#include <unity.h>
#include "../FailsafeManager.h"
#include <cstring>

// ============================================================================
// Test Fixtures
// ============================================================================

FailsafeManager fsm;

void setUp(void) {
    // Initialize fresh state machine
    fsm.reset();
}

void tearDown(void) {
    // No cleanup needed
}

// ============================================================================
// State Transition Tests
// ============================================================================

void test_initial_state_is_idle(void) {
    FailsafeState state = fsm.getState();
    TEST_ASSERT_EQUAL_INT(FAILSAFE_IDLE, state);
}

void test_transition_idle_to_armed(void) {
    fsm.arm();
    FailsafeState state = fsm.getState();
    TEST_ASSERT_EQUAL_INT(FAILSAFE_ARMED, state);
}

void test_transition_armed_to_idle(void) {
    fsm.arm();
    fsm.disarm();
    FailsafeState state = fsm.getState();
    TEST_ASSERT_EQUAL_INT(FAILSAFE_IDLE, state);
}

void test_transition_armed_to_signal_loss(void) {
    fsm.arm();
    fsm.signalLost();
    FailsafeState state = fsm.getState();
    TEST_ASSERT_EQUAL_INT(FAILSAFE_SIGNAL_LOSS, state);
}

void test_transition_signal_loss_to_emergency(void) {
    fsm.arm();
    fsm.signalLost();
    delay(1100);  // Wait > 1 second
    fsm.update();  // Process timeout
    FailsafeState state = fsm.getState();
    TEST_ASSERT_EQUAL_INT(FAILSAFE_EMERGENCY, state);
}

void test_cannot_arm_during_signal_loss(void) {
    fsm.arm();
    fsm.signalLost();
    fsm.arm();  // Try to arm again
    FailsafeState state = fsm.getState();
    // Should stay in SIGNAL_LOSS, not return to ARMED
    TEST_ASSERT_EQUAL_INT(FAILSAFE_SIGNAL_LOSS, state);
}

void test_emergency_stop_forces_emergency_state(void) {
    fsm.arm();
    fsm.emergencyStop();
    FailsafeState state = fsm.getState();
    TEST_ASSERT_EQUAL_INT(FAILSAFE_EMERGENCY, state);
}

void test_recovery_from_signal_loss(void) {
    fsm.arm();
    fsm.signalLost();
    fsm.signalRestored();
    FailsafeState state = fsm.getState();
    TEST_ASSERT_EQUAL_INT(FAILSAFE_ARMED, state);
}

// ============================================================================
// Timeout Tests
// ============================================================================

void test_signal_loss_timeout_duration(void) {
    fsm.arm();
    fsm.signalLost();
    
    // Should not transition to EMERGENCY immediately
    fsm.update();
    TEST_ASSERT_EQUAL_INT(FAILSAFE_SIGNAL_LOSS, fsm.getState());
    
    // Wait for timeout
    delay(1100);
    fsm.update();
    
    // Should now be in EMERGENCY state
    TEST_ASSERT_EQUAL_INT(FAILSAFE_EMERGENCY, fsm.getState());
}

void test_multiple_signal_loss_calls(void) {
    fsm.arm();
    fsm.signalLost();
    delay(500);
    fsm.signalLost();  // Call again within timeout
    delay(700);
    fsm.update();
    
    // Should still be in SIGNAL_LOSS (timeout reset by second call)
    TEST_ASSERT_EQUAL_INT(FAILSAFE_SIGNAL_LOSS, fsm.getState());
}

// ============================================================================
// Failsafe Status Tests
// ============================================================================

void test_motors_stopped_in_idle(void) {
    bool stopped = fsm.shouldCutMotors();
    TEST_ASSERT_FALSE(stopped);
}

void test_motors_running_when_armed(void) {
    fsm.arm();
    bool stopped = fsm.shouldCutMotors();
    TEST_ASSERT_FALSE(stopped);
}

void test_motors_stopped_on_signal_loss(void) {
    fsm.arm();
    fsm.signalLost();
    bool stopped = fsm.shouldCutMotors();
    TEST_ASSERT_TRUE(stopped);
}

void test_motors_stopped_in_emergency(void) {
    fsm.arm();
    fsm.emergencyStop();
    bool stopped = fsm.shouldCutMotors();
    TEST_ASSERT_TRUE(stopped);
}

void test_is_armed_returns_correct_state(void) {
    TEST_ASSERT_FALSE(fsm.isArmed());
    
    fsm.arm();
    TEST_ASSERT_TRUE(fsm.isArmed());
    
    fsm.disarm();
    TEST_ASSERT_FALSE(fsm.isArmed());
}

void test_is_failsafe_active(void) {
    TEST_ASSERT_FALSE(fsm.isFailsafeActive());
    
    fsm.arm();
    TEST_ASSERT_FALSE(fsm.isFailsafeActive());
    
    fsm.signalLost();
    TEST_ASSERT_TRUE(fsm.isFailsafeActive());
    
    fsm.signalRestored();
    TEST_ASSERT_FALSE(fsm.isFailsafeActive());
}

// ============================================================================
// Reset Tests
// ============================================================================

void test_reset_clears_state(void) {
    fsm.arm();
    fsm.signalLost();
    fsm.reset();
    
    TEST_ASSERT_EQUAL_INT(FAILSAFE_IDLE, fsm.getState());
    TEST_ASSERT_FALSE(fsm.isArmed());
    TEST_ASSERT_FALSE(fsm.isFailsafeActive());
}

// ============================================================================
// Test Runner
// ============================================================================

int main(int argc, char** argv) {
    UNITY_BEGIN();
    
    // State Transition Tests
    RUN_TEST(test_initial_state_is_idle);
    RUN_TEST(test_transition_idle_to_armed);
    RUN_TEST(test_transition_armed_to_idle);
    RUN_TEST(test_transition_armed_to_signal_loss);
    RUN_TEST(test_transition_signal_loss_to_emergency);
    RUN_TEST(test_cannot_arm_during_signal_loss);
    RUN_TEST(test_emergency_stop_forces_emergency_state);
    RUN_TEST(test_recovery_from_signal_loss);
    
    // Timeout Tests
    RUN_TEST(test_signal_loss_timeout_duration);
    RUN_TEST(test_multiple_signal_loss_calls);
    
    // Status Tests
    RUN_TEST(test_motors_stopped_in_idle);
    RUN_TEST(test_motors_running_when_armed);
    RUN_TEST(test_motors_stopped_on_signal_loss);
    RUN_TEST(test_motors_stopped_in_emergency);
    RUN_TEST(test_is_armed_returns_correct_state);
    RUN_TEST(test_is_failsafe_active);
    
    // Reset Tests
    RUN_TEST(test_reset_clears_state);
    
    return UNITY_END();
}
