#include "timings.hpp"
#include "render.hpp"
#include "../core/debug.hpp"
#include "src/core/memory.hpp"
#include "vulkan/vulkan_core.h"
#include "ui.hpp"

#ifndef DW_DEBUG
void pushTimestamp(GpuTimestamp* pTimestamp, double value) {}
double getLastTimestampMS(GpuTimestamp* pTimestamp) { return 0.0 }
double getTimestampMS(GpuTimestamp* pTimestamp, uint32 index) { return 0.0 }

void initGpuTimer(Renderer* pRenderer, GpuTimer* pGpuTimer) {}
void destroyGpuTimer(GpuTimer* pGpuTimer) {}

uint64 getTimestampCount(GpuTimestampParams* pParams) { return 0; }
void setTimestampCount(GpuTimestampParams* pParams, uint64 count) {}

void gpuTimerReadResults(GpuTimestampParams* pParams) {}
void gpuTimerStart(GpuTimestampParams* pParams) {}
void gpuTimestamp(String name, GpuTimestampParams* pParams) {}

void uiGpuTimingsWindow(Arena* pScratchArena, GpuTimer* pGpuTimer, float x, float y, float w, float h) {}
#else

void pushTimestamp(GpuTimestamp* pTimestamp, double value)
{
    ASSERT(pTimestamp);
    pTimestamp->mHistory[pTimestamp->mOffset] = value;
    pTimestamp->mOffset = (pTimestamp->mOffset + 1) % GPU_TIMER_MAX_HISTORY;
}

double getLastTimestampMS(GpuTimestamp* pTimestamp)
{
    ASSERT(pTimestamp);
    uint32 offset = pTimestamp->mOffset;
    if(offset == 0) offset = GPU_TIMER_MAX_HISTORY;
    return pTimestamp->mHistory[offset - 1];
}

void initGpuTimer(Renderer* pRenderer, GpuTimer* pGpuTimer)
{
    ASSERT(pRenderer && pGpuTimer);
    for(uint32 i = 0; i < GPU_TIMER_MAX_POOLS; i++)
    {
        VkQueryPoolCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        info.queryType = VK_QUERY_TYPE_TIMESTAMP;
        info.queryCount = GPU_TIMER_MAX_TIMESTAMPS;
        
        VkQueryPool vkQueryPool;
        VkResult ret = vkCreateQueryPool(pRenderer->mVkDevice, 
                &info,
                NULL, 
                &vkQueryPool);
        ASSERTVK(ret);
        pGpuTimer->mVkQueryPools[i] = vkQueryPool;
    }

    for(uint32 i = 0; i < GPU_TIMER_MAX_TIMESTAMPS; i++)
    {
        pGpuTimer->mTimestampNames[i] = {};
        GpuTimestamp ts = {};
        for(uint32 j = 0; j < GPU_TIMER_MAX_HISTORY; j++)
        {
            ts.mHistory[i] = 0.0;
            ts.mOffset = 0;
        }
        pGpuTimer->mTimestamps[i] = ts;
    }

    pGpuTimer->pRenderer = pRenderer;
}

void destroyGpuTimer(GpuTimer* pGpuTimer)
{
    ASSERT(pGpuTimer);

    for(uint32 i = 0; i < GPU_TIMER_MAX_POOLS; i++)
    {
        vkDestroyQueryPool(pGpuTimer->pRenderer->mVkDevice,
                pGpuTimer->mVkQueryPools[i],
                NULL);
    }

    *pGpuTimer = {};
}

uint64 getTimestampCount(GpuTimestampParams* pParams)
{
    return pParams->pGpuTimer->mTimestampsPerQuery[pParams->queryPool];
}

void setTimestampCount(GpuTimestampParams* pParams, uint64 count)
{
    pParams->pGpuTimer->mTimestampsPerQuery[pParams->queryPool] = count;
}

void gpuTimerReadResults(GpuTimestampParams* pParams)
{
    ASSERT(pParams->pGpuTimer);
    ASSERT(pParams->queryPool < GPU_TIMER_MAX_POOLS);
    VkQueryPool vkQueryPool = pParams->pGpuTimer->mVkQueryPools[pParams->queryPool];

    uint64 sizePerResult = sizeof(uint64);
    uint64 queryResultCount = getTimestampCount(pParams);
    if(!queryResultCount)
    {
        return;
    }

    uint64 queryResults[GPU_TIMER_MAX_TIMESTAMPS];

    VkResult ret = vkGetQueryPoolResults(pParams->pGpuTimer->pRenderer->mVkDevice, 
            vkQueryPool, 
            0, queryResultCount,
            sizePerResult * GPU_TIMER_MAX_TIMESTAMPS, 
            queryResults, 
            sizePerResult, 
            VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
    ASSERTVK(ret);

    // Write back results if available
    for(uint32 i = 1; i < queryResultCount; i++)
    {
        uint64 lastQueryResult = queryResults[i - 1];
        uint64 queryResult = queryResults[i];
        float period = pParams->pGpuTimer->pRenderer->mVkDeviceProperties.limits.timestampPeriod;
        double ns = (queryResult - lastQueryResult) * period;
        double ms = ns / 1e6;

        pushTimestamp(&pParams->pGpuTimer->mTimestamps[i], ms);
    }
}

void gpuTimerStart(GpuTimestampParams* pParams)
{
    ASSERT(pParams->pGpuTimer && pParams->pCmd);
    ASSERT(pParams->queryPool < GPU_TIMER_MAX_POOLS);
    ASSERT(pParams->pCmd->mState == COMMAND_BUFFER_RECORDING);

    VkQueryPool vkQueryPool = pParams->pGpuTimer->mVkQueryPools[pParams->queryPool];

    vkCmdResetQueryPool(pParams->pCmd->mVkCmd, 
            vkQueryPool, 
            0, GPU_TIMER_MAX_TIMESTAMPS);

    // After reset, emit the first timestamp.
    // This is the start so all other timestamps have a reference.
    gpuTimestamp(str("GpuTimingStart"), pParams);
}

void gpuTimestamp(String name, GpuTimestampParams* pParams)
{
    ASSERT(pParams->pGpuTimer && pParams->pCmd);
    ASSERT(pParams->queryPool < GPU_TIMER_MAX_POOLS);

    VkQueryPool vkQueryPool = pParams->pGpuTimer->mVkQueryPools[pParams->queryPool];

    vkCmdWriteTimestamp(pParams->pCmd->mVkCmd, 
            (VkPipelineStageFlagBits)PIPELINE_STAGE_BOTTOM, 
            vkQueryPool,
            pParams->currentTimestamp);

    pParams->pGpuTimer->mTimestampNames[pParams->currentTimestamp] = name;

    pParams->currentTimestamp++;
    setTimestampCount(pParams, pParams->currentTimestamp);
}

void uiGpuTimingsGetPlotData(GpuTimestamp* pTimestamp, float* pX, float* pY, uint32 count)
{
    ASSERT(pTimestamp);
    uint32 currentOffset = pTimestamp->mOffset;
    if(currentOffset == 0) currentOffset = count;
    for(int32 j = count - 1; j >= 0; j--)
    {
        pX[j] = (float)j;
        pY[j] = (float)pTimestamp->mHistory[currentOffset - 1];
        currentOffset--;
        if(currentOffset == 0) currentOffset = count;
    }
}

void uiGpuTimingsWindow(Arena* pScratchArena, GpuTimer* pGpuTimer, float x, float y, float w, float h)
{
    ASSERT(pGpuTimer);

    ARENA_CHECKPOINT_SET(pScratchArena, gpuTimingsUI);

    uiStartWindow(str("GPU Timings"), x, y, w, h);
    GpuTimestamp* pLastTimestamp = &pGpuTimer->mTimestamps[0];
    GpuTimestamp* pCurrentTimestamp = NULL;

    static bool detailed = false;
    static float yMaxLimit = 100.f;
    uiCheckbox(str("Detailed"), &detailed);
    if(detailed)
    {
        uiDragf(str("Y Scale (MS)"), &yMaxLimit, 1.f, 20.f, 200.f);
    }
    uiSeparator();

    for(uint32 i = 1; i < GPU_TIMER_MAX_TIMESTAMPS; i++)
    {
        pCurrentTimestamp = &pGpuTimer->mTimestamps[i];

        double ms = getLastTimestampMS(pCurrentTimestamp);
        if(ms == 0.0)
        {
            continue;
        }

        // Timing data
        String tsName = pGpuTimer->mTimestampNames[i];
        String text = strf(pScratchArena, "[%s]: %.3f ms",
                cstr(tsName),
                ms);
        uiText(text);

        // Detailed plot data (line plot with frame history, up to GPU_TIMER_MAX_HISTORY frames)
        if(detailed)
        {

            float dataX[GPU_TIMER_MAX_HISTORY];
            float dataY[GPU_TIMER_MAX_HISTORY];
            uiGpuTimingsGetPlotData(pCurrentTimestamp, dataX, dataY, GPU_TIMER_MAX_HISTORY);

            char label[256];
            strf(label, "##Plot(%s)", cstr(tsName));

            UILinePlotDesc desc = {};
            desc.mShaded = true;
            desc.mSize = {-1, 150};
            desc.mMinLimit = {0, 0};
            desc.mMaxLimit = {GPU_TIMER_MAX_HISTORY, yMaxLimit};
            desc.mLinePointCount = GPU_TIMER_MAX_HISTORY;
            desc.mLineCount = 1;
            desc.mDataX[0] = dataX;
            desc.mDataY[0] = dataY;

            uiLinePlot(str(label), desc);
        }

        pLastTimestamp = pCurrentTimestamp;
        uiSeparator();
    }
    uiEndWindow();

    ARENA_CHECKPOINT_RESET(pScratchArena, gpuTimingsUI);
}

#endif
