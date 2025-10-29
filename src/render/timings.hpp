#pragma once
#include "../core/string.hpp"
#include "command_buffer.hpp"
#include "vulkan/vulkan_core.h"

struct Renderer;

// Each timestamp has a 120 frame history.
#define GPU_TIMER_MAX_HISTORY 120
struct GpuTimestamp
{
    double mHistory[GPU_TIMER_MAX_HISTORY]; // Last time diff for the ts in ms
    uint32 mOffset = 0; // Next time diff will be pushed to this offset
};

void pushTimestamp(GpuTimestamp* pTimestamp, double value);
double getLastTimestampMS(GpuTimestamp* pTimestamp);

#define GPU_TIMER_MAX_TIMESTAMPS 50
#define GPU_TIMER_MAX_POOLS 8
struct GpuTimer
{
    Renderer* pRenderer = NULL;

    String mTimestampNames[GPU_TIMER_MAX_TIMESTAMPS];
    GpuTimestamp mTimestamps[GPU_TIMER_MAX_TIMESTAMPS];

    VkQueryPool mVkQueryPools[GPU_TIMER_MAX_POOLS];
    uint32 mTimestampsPerQuery[GPU_TIMER_MAX_POOLS];
};

struct GpuTimestampParams
{
    GpuTimer* pGpuTimer = NULL;
    CommandBuffer* pCmd = NULL;
    uint32 queryPool = MAX_UINT32;
    uint32 currentTimestamp = 0;
};

void initGpuTimer(Renderer* pRenderer, GpuTimer* pGpuTimer);
void destroyGpuTimer(GpuTimer* pGpuTimer);

uint64 getTimestampCount(GpuTimestampParams* pParams);
void setTimestampCount(GpuTimestampParams* pParams, uint64 count);

void gpuTimerReadResults(GpuTimestampParams* pParams);
void gpuTimerStart(GpuTimestampParams* pParams);
void gpuTimestamp(String name, GpuTimestampParams* pParams);

void uiGpuTimingsWindow(Arena* pScratchArena, GpuTimer* pGpuTimer, float x, float y, float w, float h);
