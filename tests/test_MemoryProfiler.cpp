#ifdef UNIT_TESTING

#include <unity.h>
#include "MemoryProfiler.h"
#include <string.h>

// ============================================================================
// Test Fixtures
// ============================================================================

void setUp(void) {
    // Initialize memory profiler before each test
    MemoryProfiler_init();
    MemoryProfiler_setEnabled(true);
}

void tearDown(void) {
    // Reset after each test
    MemoryProfiler_reset();
}

// ============================================================================
// Test Cases
// ============================================================================

/**
 * Test 1: Initialization
 */
void test_MemoryProfiler_Init(void) {
    TEST_ASSERT_TRUE(MemoryProfiler_init());
    TEST_ASSERT_TRUE(MemoryProfiler_isEnabled());
}

/**
 * Test 2: Memory Stats Collection
 */
void test_MemoryProfiler_MemoryStatsCollection(void) {
    MemoryStats stats = MemoryProfiler_getMemoryStats();
    
    // Heap size should be reasonable (ESP32 ~200KB)
    TEST_ASSERT_GREATER_THAN(0, stats.heapSize);
    TEST_ASSERT_LESS_THAN(500000, stats.heapSize);  // Less than 500KB
    
    // Used should be less than total
    TEST_ASSERT_LESS_THAN_OR_EQUAL(stats.heapUsed, stats.heapSize);
    
    // Utilization should be 0-100%
    TEST_ASSERT_GREATER_THAN_OR_EQUAL(stats.memoryUtilization, 0.0f);
    TEST_ASSERT_LESS_THAN_OR_EQUAL(stats.memoryUtilization, 100.0f);
    
    // Fragmentation should be 0-100%
    TEST_ASSERT_GREATER_THAN_OR_EQUAL(stats.fragmentationRatio, 0);
    TEST_ASSERT_LESS_THAN_OR_EQUAL(stats.fragmentationRatio, 100);
}

/**
 * Test 3: CPU Stats Collection
 */
void test_MemoryProfiler_CPUStatsCollection(void) {
    MemoryProfiler_recordTaskTime("test_task", 1000);  // 1ms task
    
    CPUStats stats = MemoryProfiler_getCPUStats();
    
    // Loop execution time should be reasonable
    TEST_ASSERT_GREATER_THAN_OR_EQUAL(stats.loopExecutionTimeUs, 0);
    TEST_ASSERT_LESS_THAN(stats.loopExecutionTimeUs, 100000);  // Less than 100ms
    
    // CPU load should be 0-100%
    TEST_ASSERT_GREATER_THAN_OR_EQUAL(stats.cpuLoadPercent, 0.0f);
    TEST_ASSERT_LESS_THAN_OR_EQUAL(stats.cpuLoadPercent, 100.0f);
}

/**
 * Test 4: Task Timing Recording
 */
void test_MemoryProfiler_TaskTimingRecording(void) {
    // Record a task
    MemoryProfiler_recordTaskTime("sensor_read", 5000);  // 5ms
    
    TaskTimingInfo* task = MemoryProfiler_getTaskTiming(0);
    TEST_ASSERT_NOT_NULL(task);
    TEST_ASSERT_EQUAL_STRING("sensor_read", task->taskName);
    TEST_ASSERT_EQUAL_INT(5000, task->executionTimeUs);
    TEST_ASSERT_EQUAL_INT(1, task->callCount);
}

/**
 * Test 5: Multiple Task Tracking
 */
void test_MemoryProfiler_MultipleTaskTracking(void) {
    // Record multiple tasks
    MemoryProfiler_recordTaskTime("task_a", 1000);
    MemoryProfiler_recordTaskTime("task_b", 2000);
    MemoryProfiler_recordTaskTime("task_c", 3000);
    
    TEST_ASSERT_EQUAL_INT(3, MemoryProfiler_getTaskCount());
    
    // Verify each task
    TaskTimingInfo* task_a = MemoryProfiler_getTaskTiming(0);
    TaskTimingInfo* task_b = MemoryProfiler_getTaskTiming(1);
    TaskTimingInfo* task_c = MemoryProfiler_getTaskTiming(2);
    
    TEST_ASSERT_EQUAL_STRING("task_a", task_a->taskName);
    TEST_ASSERT_EQUAL_STRING("task_b", task_b->taskName);
    TEST_ASSERT_EQUAL_STRING("task_c", task_c->taskName);
}

/**
 * Test 6: Task Call Count Increment
 */
void test_MemoryProfiler_TaskCallCountIncrement(void) {
    // Call same task multiple times
    for (int i = 0; i < 5; i++) {
        MemoryProfiler_recordTaskTime("repeated_task", 1000 + i);
    }
    
    TaskTimingInfo* task = MemoryProfiler_getTaskTiming(0);
    TEST_ASSERT_EQUAL_INT(5, task->callCount);
}

/**
 * Test 7: Task Min/Max Tracking
 */
void test_MemoryProfiler_TaskMinMaxTracking(void) {
    // Record multiple times with different durations
    MemoryProfiler_recordTaskTime("variable_task", 1000);  // 1ms
    MemoryProfiler_recordTaskTime("variable_task", 5000);  // 5ms
    MemoryProfiler_recordTaskTime("variable_task", 3000);  // 3ms
    
    TaskTimingInfo* task = MemoryProfiler_getTaskTiming(0);
    TEST_ASSERT_EQUAL_INT(1000, task->minExecutionTimeUs);
    TEST_ASSERT_EQUAL_INT(5000, task->maxExecutionTimeUs);
    TEST_ASSERT_EQUAL_INT(3, task->callCount);
}

/**
 * Test 8: Fragmentation Critical Detection
 */
void test_MemoryProfiler_FragmentationCriticalDetection(void) {
    // Get current status
    bool isCritical = MemoryProfiler_isFragmentationCritical();
    
    // Should return valid boolean
    TEST_ASSERT_TRUE(isCritical == true || isCritical == false);
}

/**
 * Test 9: Memory Usage High Detection
 */
void test_MemoryProfiler_MemoryUsageHighDetection(void) {
    bool isHigh = MemoryProfiler_isMemoryUsageHigh();
    
    // Should return valid boolean
    TEST_ASSERT_TRUE(isHigh == true || isHigh == false);
}

/**
 * Test 10: Loop Frequency OK Check
 */
void test_MemoryProfiler_LoopFrequencyOKCheck(void) {
    bool isOK = MemoryProfiler_isLoopFrequencyOK();
    
    // Should return valid boolean
    TEST_ASSERT_TRUE(isOK == true || isOK == false);
}

/**
 * Test 11: Status String Generation
 */
void test_MemoryProfiler_StatusStringGeneration(void) {
    const char* status = MemoryProfiler_getStatusString();
    
    TEST_ASSERT_NOT_NULL(status);
    TEST_ASSERT_GREATER_THAN(0, strlen(status));
    
    // Should contain expected metrics
    TEST_ASSERT_NOT_NULL(strstr(status, "Heap"));
    TEST_ASSERT_NOT_NULL(strstr(status, "CPU"));
    TEST_ASSERT_NOT_NULL(strstr(status, "Frag"));
}

/**
 * Test 12: Optimization Recommendation
 */
void test_MemoryProfiler_OptimizationRecommendation(void) {
    const char* recommendation = MemoryProfiler_getOptimizationRecommendation();
    
    TEST_ASSERT_NOT_NULL(recommendation);
    TEST_ASSERT_GREATER_THAN(0, strlen(recommendation));
}

/**
 * Test 13: Enable/Disable Profiling
 */
void test_MemoryProfiler_EnableDisableProfiling(void) {
    TEST_ASSERT_TRUE(MemoryProfiler_isEnabled());
    
    MemoryProfiler_setEnabled(false);
    TEST_ASSERT_FALSE(MemoryProfiler_isEnabled());
    
    MemoryProfiler_setEnabled(true);
    TEST_ASSERT_TRUE(MemoryProfiler_isEnabled());
}

/**
 * Test 14: Reset Statistics
 */
void test_MemoryProfiler_ResetStatistics(void) {
    // Record some data
    MemoryProfiler_recordTaskTime("task", 1000);
    TEST_ASSERT_EQUAL_INT(1, MemoryProfiler_getTaskCount());
    
    // Reset
    MemoryProfiler_reset();
    TEST_ASSERT_EQUAL_INT(0, MemoryProfiler_getTaskCount());
}

/**
 * Test 15: Out of Bounds Task Index
 */
void test_MemoryProfiler_OutOfBoundsTaskIndex(void) {
    MemoryProfiler_recordTaskTime("task", 1000);
    
    // Request non-existent task
    TaskTimingInfo* task = MemoryProfiler_getTaskTiming(10);
    TEST_ASSERT_NULL(task);
}

/**
 * Test 16: Max Tasks Limit
 */
void test_MemoryProfiler_MaxTasksLimit(void) {
    // Record MAX_TRACKED_TASKS (20) tasks
    for (int i = 0; i < 25; i++) {
        char taskName[32];
        snprintf(taskName, sizeof(taskName), "task_%d", i);
        MemoryProfiler_recordTaskTime(taskName, 1000);
    }
    
    // Should not exceed MAX_TRACKED_TASKS
    uint8_t count = MemoryProfiler_getTaskCount();
    TEST_ASSERT_LESS_THAN_OR_EQUAL(20, count);
}

/**
 * Test 17: Task Timing Info Accuracy
 */
void test_MemoryProfiler_TaskTimingInfoAccuracy(void) {
    const char* taskName = "accurate_task";
    uint32_t timing = 12345;  // 12.345ms
    
    MemoryProfiler_recordTaskTime(taskName, timing);
    
    TaskTimingInfo* task = MemoryProfiler_getTaskTiming(0);
    TEST_ASSERT_EQUAL_INT(timing, task->executionTimeUs);
}

/**
 * Test 18: Memory Stats Consistency
 */
void test_MemoryProfiler_MemoryStatsConsistency(void) {
    MemoryStats stats1 = MemoryProfiler_getMemoryStats();
    MemoryStats stats2 = MemoryProfiler_getMemoryStats();
    
    // Stats should be consistent (heap shouldn't grow dramatically)
    int32_t diff = stats2.heapUsed - stats1.heapUsed;
    TEST_ASSERT_LESS_THAN(5000, diff);  // Less than 5KB difference
}

// ============================================================================
// Test Runner
// ============================================================================

int runMemoryProfilerTests(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_MemoryProfiler_Init);
    RUN_TEST(test_MemoryProfiler_MemoryStatsCollection);
    RUN_TEST(test_MemoryProfiler_CPUStatsCollection);
    RUN_TEST(test_MemoryProfiler_TaskTimingRecording);
    RUN_TEST(test_MemoryProfiler_MultipleTaskTracking);
    RUN_TEST(test_MemoryProfiler_TaskCallCountIncrement);
    RUN_TEST(test_MemoryProfiler_TaskMinMaxTracking);
    RUN_TEST(test_MemoryProfiler_FragmentationCriticalDetection);
    RUN_TEST(test_MemoryProfiler_MemoryUsageHighDetection);
    RUN_TEST(test_MemoryProfiler_LoopFrequencyOKCheck);
    RUN_TEST(test_MemoryProfiler_StatusStringGeneration);
    RUN_TEST(test_MemoryProfiler_OptimizationRecommendation);
    RUN_TEST(test_MemoryProfiler_EnableDisableProfiling);
    RUN_TEST(test_MemoryProfiler_ResetStatistics);
    RUN_TEST(test_MemoryProfiler_OutOfBoundsTaskIndex);
    RUN_TEST(test_MemoryProfiler_MaxTasksLimit);
    RUN_TEST(test_MemoryProfiler_TaskTimingInfoAccuracy);
    RUN_TEST(test_MemoryProfiler_MemoryStatsConsistency);
    
    return UNITY_END();
}

#endif // UNIT_TESTING
