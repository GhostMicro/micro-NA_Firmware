#ifndef MEMORY_PROFILER_H
#define MEMORY_PROFILER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * MemoryProfiler - Runtime Memory & CPU Profiling
 * 
 * Features:
 * - Heap fragmentation analysis
 * - Stack usage tracking
 * - Task timing analysis
 * - Memory statistics collection
 * - Buffer optimization recommendations
 * - CPU load percentage
 * 
 * @file MemoryProfiler.h
 */

/**
 * Memory Statistics Structure
 */
typedef struct {
    uint32_t heapSize;              // Total heap size (bytes)
    uint32_t heapUsed;              // Current heap used (bytes)
    uint32_t largestFreeBlock;      // Largest contiguous free block (bytes)
    uint32_t fragmentationRatio;    // Fragmentation % (0-100)
    uint32_t stackUsed;             // Stack memory used (bytes)
    uint32_t stackFree;             // Stack memory free (bytes)
    uint32_t totalMemoryUsed;       // Heap + stack used (bytes)
    uint32_t totalMemoryAvailable;  // Heap + stack total (bytes)
    float memoryUtilization;        // Memory usage % (0-100.0)
} MemoryStats;

/**
 * CPU Statistics Structure
 */
typedef struct {
    uint32_t loopExecutionTimeUs;   // Time to execute one control loop (microseconds)
    uint32_t maxLoopExecutionTimeUs;// Maximum loop execution time (microseconds)
    uint32_t minLoopExecutionTimeUs;// Minimum loop execution time (microseconds)
    float cpuLoadPercent;           // Estimated CPU load (0-100.0)
    uint32_t loopIterations;        // Total number of loop iterations
    uint32_t loopFrequencyHz;       // Measured loop frequency (should be ~50Hz)
} CPUStats;

/**
 * Task Timing Information
 */
typedef struct {
    const char* taskName;           // Name of task
    uint32_t executionTimeUs;       // Execution time (microseconds)
    float percentOfLoop;            // Percentage of loop time
    uint32_t callCount;             // Number of times called
    uint32_t maxExecutionTimeUs;    // Maximum execution time
    uint32_t minExecutionTimeUs;    // Minimum execution time
} TaskTimingInfo;

/**
 * Initialize memory profiler
 * @return true if initialization successful
 */
bool MemoryProfiler_init(void);

/**
 * Get current memory statistics
 * @return MemoryStats structure with current values
 */
MemoryStats MemoryProfiler_getMemoryStats(void);

/**
 * Get current CPU statistics
 * @return CPUStats structure with current values
 */
CPUStats MemoryProfiler_getCPUStats(void);

/**
 * Record task execution time
 * @param taskName Name of task (for reporting)
 * @param executionTimeUs Execution time in microseconds
 */
void MemoryProfiler_recordTaskTime(const char* taskName, uint32_t executionTimeUs);

/**
 * Get task timing information by index
 * @param index Task index (0 to N-1)
 * @return Task timing info, or NULL if index out of bounds
 */
TaskTimingInfo* MemoryProfiler_getTaskTiming(uint8_t index);

/**
 * Get number of tracked tasks
 * @return Number of unique tasks
 */
uint8_t MemoryProfiler_getTaskCount(void);

/**
 * Calculate recommended buffer optimizations
 * @return Recommendation string (e.g., "Reduce serial buffer by 512 bytes")
 */
const char* MemoryProfiler_getOptimizationRecommendation(void);

/**
 * Check if memory fragmentation is critical
 * @return true if fragmentation > 80%
 */
bool MemoryProfiler_isFragmentationCritical(void);

/**
 * Check if memory usage is high
 * @return true if usage > 90%
 */
bool MemoryProfiler_isMemoryUsageHigh(void);

/**
 * Check if loop is meeting 50Hz target
 * @return true if loop frequency ~50Hz
 */
bool MemoryProfiler_isLoopFrequencyOK(void);

/**
 * Reset statistics
 */
void MemoryProfiler_reset(void);

/**
 * Get profiler status string (for telemetry)
 * @return Status string like "Heap: 45%, CPU: 62Hz, Frag: 15%"
 */
const char* MemoryProfiler_getStatusString(void);

/**
 * Enable/disable profiling
 * @param enabled true to enable, false to disable
 */
void MemoryProfiler_setEnabled(bool enabled);

/**
 * Check if profiling is enabled
 * @return true if enabled
 */
bool MemoryProfiler_isEnabled(void);

#endif // MEMORY_PROFILER_H
