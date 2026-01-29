#include "MemoryProfiler.h"
#include <Arduino.h>
#include <esp_heap_caps.h>
#include <stdio.h>
#include <string.h>

// Memory Profiler Configuration
#define MAX_TRACKED_TASKS 20
#define STATS_BUFFER_SIZE 256
#define LOOP_FREQUENCY_TARGET 50   // 50Hz control loop
#define LOOP_FREQUENCY_TOLERANCE 5 // +/- 5Hz acceptable

// Internal state
static struct {
  bool enabled;
  bool initialized;

  // Memory stats
  uint32_t lastHeapSize;
  uint32_t lastHeapUsed;
  uint32_t maxHeapUsed;

  // CPU stats
  uint32_t lastLoopStartTime;
  uint32_t loopExecutionTimeUs;
  uint32_t maxLoopTimeUs;
  uint32_t minLoopTimeUs;
  uint32_t loopIterations;
  uint32_t lastFrequencyCheckTime;

  // Task tracking
  TaskTimingInfo tasks[MAX_TRACKED_TASKS];
  uint8_t taskCount;

  // Status string buffer
  char statusBuffer[STATS_BUFFER_SIZE];
} profilerState = {.enabled = true,
                   .initialized = false,
                   .lastHeapSize = 0,
                   .lastHeapUsed = 0,
                   .maxHeapUsed = 0,
                   .loopExecutionTimeUs = 0,
                   .maxLoopTimeUs = 0,
                   .minLoopTimeUs = UINT32_MAX,
                   .loopIterations = 0,
                   .lastFrequencyCheckTime = 0,
                   .taskCount = 0};

// Forward declarations
static uint32_t getHeapSize(void);
static uint32_t getHeapUsed(void);
static uint32_t getStackUsed(void);
static uint32_t calculateFragmentation(void);

/**
 * Initialize memory profiler
 */
bool MemoryProfiler_init(void) {
  if (profilerState.initialized)
    return true;

  // Initialize task array
  memset(profilerState.tasks, 0, sizeof(profilerState.tasks));
  profilerState.taskCount = 0;

  // Initialize timing
  profilerState.lastLoopStartTime = micros();
  profilerState.lastFrequencyCheckTime = millis();

  profilerState.initialized = true;
  Serial.println("[MemoryProfiler] Initialized");
  return true;
}

/**
 * Get current memory statistics
 */
MemoryStats MemoryProfiler_getMemoryStats(void) {
  if (!profilerState.initialized) {
    return (MemoryStats){0};
  }

  uint32_t heapSize = getHeapSize();
  uint32_t heapUsed = getHeapUsed();
  uint32_t stackUsed = getStackUsed();
  uint32_t largestFreeBlock =
      heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);
  uint32_t fragmentation = calculateFragmentation();

  // Update max heap used
  if (heapUsed > profilerState.maxHeapUsed) {
    profilerState.maxHeapUsed = heapUsed;
  }

  uint32_t totalUsed = heapUsed + stackUsed;
  uint32_t totalAvailable = heapSize + (8192); // Assume 8KB stack (ESP32)
  float utilization = (totalUsed * 100.0f) / totalAvailable;

  return (MemoryStats){.heapSize = heapSize,
                       .heapUsed = heapUsed,
                       .largestFreeBlock = largestFreeBlock,
                       .fragmentationRatio = fragmentation,
                       .stackUsed = stackUsed,
                       .stackFree = (8192 - stackUsed),
                       .totalMemoryUsed = totalUsed,
                       .totalMemoryAvailable = totalAvailable,
                       .memoryUtilization = utilization};
}

/**
 * Get current CPU statistics
 */
CPUStats MemoryProfiler_getCPUStats(void) {
  if (!profilerState.initialized) {
    return (CPUStats){0};
  }

  uint32_t now = millis();
  uint32_t elapsedMs = now - profilerState.lastFrequencyCheckTime;

  // Calculate loop frequency (100ms sample)
  float loopFrequency =
      profilerState.loopIterations * 1000.0f / (elapsedMs ? elapsedMs : 1);

  // Estimate CPU load based on loop execution time
  // At 50Hz, each iteration should take ~20ms
  // If actual time is less, CPU has spare capacity
  float cpuLoad = (profilerState.loopExecutionTimeUs / 20000.0f) * 100.0f;
  cpuLoad = cpuLoad > 100.0f ? 100.0f : cpuLoad; // Cap at 100%

  return (CPUStats){.loopExecutionTimeUs = profilerState.loopExecutionTimeUs,
                    .maxLoopExecutionTimeUs = profilerState.maxLoopTimeUs,
                    .minLoopExecutionTimeUs = profilerState.minLoopTimeUs,
                    .cpuLoadPercent = cpuLoad,
                    .loopIterations = profilerState.loopIterations,
                    .loopFrequencyHz = (uint32_t)loopFrequency};
}

/**
 * Record task execution time
 */
void MemoryProfiler_recordTaskTime(const char *taskName,
                                   uint32_t executionTimeUs) {
  if (!profilerState.enabled || !profilerState.initialized)
    return;

  // Find or create task entry
  int taskIndex = -1;
  for (int i = 0; i < profilerState.taskCount; i++) {
    if (strcmp(profilerState.tasks[i].taskName, taskName) == 0) {
      taskIndex = i;
      break;
    }
  }

  // Create new task entry if not found
  if (taskIndex == -1 && profilerState.taskCount < MAX_TRACKED_TASKS) {
    taskIndex = profilerState.taskCount++;
    profilerState.tasks[taskIndex].taskName = taskName;
    profilerState.tasks[taskIndex].maxExecutionTimeUs = 0;
    profilerState.tasks[taskIndex].minExecutionTimeUs = UINT32_MAX;
    profilerState.tasks[taskIndex].callCount = 0;
  }

  if (taskIndex >= 0 && taskIndex < MAX_TRACKED_TASKS) {
    TaskTimingInfo *task = &profilerState.tasks[taskIndex];
    task->executionTimeUs = executionTimeUs;
    task->callCount++;

    if (executionTimeUs > task->maxExecutionTimeUs) {
      task->maxExecutionTimeUs = executionTimeUs;
    }
    if (executionTimeUs < task->minExecutionTimeUs) {
      task->minExecutionTimeUs = executionTimeUs;
    }
  }
}

/**
 * Get task timing information by index
 */
TaskTimingInfo *MemoryProfiler_getTaskTiming(uint8_t index) {
  if (index >= profilerState.taskCount)
    return NULL;
  return &profilerState.tasks[index];
}

/**
 * Get number of tracked tasks
 */
uint8_t MemoryProfiler_getTaskCount(void) { return profilerState.taskCount; }

/**
 * Calculate recommended buffer optimizations
 */
const char *MemoryProfiler_getOptimizationRecommendation(void) {
  MemoryStats stats = MemoryProfiler_getMemoryStats();

  static char recommendation[STATS_BUFFER_SIZE];
  memset(recommendation, 0, STATS_BUFFER_SIZE);

  if (stats.fragmentationRatio > 80) {
    snprintf(recommendation, STATS_BUFFER_SIZE,
             "CRITICAL: Heap fragmentation at %u%%. Consider firmware "
             "defragmentation.",
             stats.fragmentationRatio);
  } else if (stats.memoryUtilization > 85) {
    snprintf(recommendation, STATS_BUFFER_SIZE,
             "Memory usage at %.1f%%. Consider reducing buffer sizes.",
             stats.memoryUtilization);
  } else if (stats.largestFreeBlock < 2048) {
    snprintf(recommendation, STATS_BUFFER_SIZE,
             "Largest free block: %u bytes. Risk of allocation failures.",
             stats.largestFreeBlock);
  } else {
    snprintf(recommendation, STATS_BUFFER_SIZE,
             "Memory status OK. Utilization: %.1f%%, Fragmentation: %u%%",
             stats.memoryUtilization, stats.fragmentationRatio);
  }

  return recommendation;
}

/**
 * Check if memory fragmentation is critical
 */
bool MemoryProfiler_isFragmentationCritical(void) {
  MemoryStats stats = MemoryProfiler_getMemoryStats();
  return stats.fragmentationRatio > 80;
}

/**
 * Check if memory usage is high
 */
bool MemoryProfiler_isMemoryUsageHigh(void) {
  MemoryStats stats = MemoryProfiler_getMemoryStats();
  return stats.memoryUtilization > 90.0f;
}

/**
 * Check if loop is meeting 50Hz target
 */
bool MemoryProfiler_isLoopFrequencyOK(void) {
  CPUStats stats = MemoryProfiler_getCPUStats();
  int32_t deviation = stats.loopFrequencyHz - LOOP_FREQUENCY_TARGET;
  return (deviation >= -LOOP_FREQUENCY_TOLERANCE &&
          deviation <= LOOP_FREQUENCY_TOLERANCE);
}

/**
 * Reset statistics
 */
void MemoryProfiler_reset(void) {
  profilerState.loopIterations = 0;
  profilerState.maxLoopTimeUs = 0;
  profilerState.minLoopTimeUs = UINT32_MAX;
  profilerState.taskCount = 0;
  memset(profilerState.tasks, 0, sizeof(profilerState.tasks));
  profilerState.lastFrequencyCheckTime = millis();
}

/**
 * Get profiler status string
 */
const char *MemoryProfiler_getStatusString(void) {
  MemoryStats memStats = MemoryProfiler_getMemoryStats();
  CPUStats cpuStats = MemoryProfiler_getCPUStats();

  snprintf(profilerState.statusBuffer, STATS_BUFFER_SIZE,
           "Heap:%.0f%% CPU:%uHz Frag:%u%%", memStats.memoryUtilization,
           cpuStats.loopFrequencyHz, memStats.fragmentationRatio);

  return profilerState.statusBuffer;
}

/**
 * Enable/disable profiling
 */
void MemoryProfiler_setEnabled(bool enabled) {
  profilerState.enabled = enabled;
}

/**
 * Check if profiling is enabled
 */
bool MemoryProfiler_isEnabled(void) { return profilerState.enabled; }

// ============================================================================
// Internal Helpers
// ============================================================================

/**
 * Get heap size (total available)
 */
static uint32_t getHeapSize(void) {
  // ESP32 default heap size is ~200KB (varies by partition)
  return heap_caps_get_total_size(MALLOC_CAP_DEFAULT);
}

/**
 * Get heap used
 */
static uint32_t getHeapUsed(void) {
  return heap_caps_get_total_size(MALLOC_CAP_DEFAULT) -
         heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
}

/**
 * Get stack used (approximate)
 */
static uint32_t getStackUsed(void) {
  // Get current stack pointer
  volatile uint32_t sp;
  asm("mov %0, sp" : "=r"(sp));

  // ESP32 stack grows downward from high address
  // This is a rough estimate
  return (8192 - (sp & 0x1FFF)) / 8192; // Return as rough percentage
}

/**
 * Calculate heap fragmentation ratio
 */
static uint32_t calculateFragmentation(void) {
  uint32_t heapSize = getHeapSize();
  uint32_t heapUsed = getHeapUsed();
  uint32_t heapFree = heapSize - heapUsed;
  uint32_t largestFree = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);

  if (heapFree == 0)
    return 0;

  // Fragmentation = (total free - largest contiguous) / total free * 100%
  uint32_t fragmented = heapFree - largestFree;
  return (fragmented * 100) / heapFree;
}
