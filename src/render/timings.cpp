#include "timings.hpp"
#include "render.hpp"
#include "../core/debug.hpp"
#include "src/core/memory.hpp"
#include "vulkan/vulkan_core.h"
#include "ui.hpp"

#ifndef DW_DEBUG
void initGpuTimer(Renderer* pRenderer, GpuTimer* pGpuTimer) {}
void destroyGpuTimer(GpuTimer* pGpuTimer) {}

uint64 getTimestampCount(GpuTimestampParams* pParams) {}
void setTimestampCount(GpuTimestampParams* pParams, uint64 count) {}

void gpuTimerReadResults(GpuTimestampParams* pParams) {}
void gpuTimerStart(GpuTimestampParams* pParams) {}
void gpuTimestamp(String name, GpuTimestampParams* pParams) {}

void uiGpuTimingsWindow(Arena* pScratchArena, GpuTimer* pGpuTimer, float x, float y, float w, float h) {}
#else

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
        pGpuTimer->mTimestamps[i] = 0;
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
    for(uint32 i = 0; i < queryResultCount; i++)
    {
        pParams->pGpuTimer->mTimestamps[i] = queryResults[i];
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

void uiGpuTimingsWindow(Arena* pScratchArena, GpuTimer* pGpuTimer, float x, float y, float w, float h)
{
    ASSERT(pGpuTimer);

    ARENA_CHECKPOINT_SET(pScratchArena, gpuTimingsUI);

    uiStartWindow(str("GPU Timings"), x, y, w, h);
    uint64 lastTimestamp = pGpuTimer->mTimestamps[0];
    uint64 currentTimestamp = 0;
    for(uint32 i = 1; i < GPU_TIMER_MAX_TIMESTAMPS; i++)
    {
        currentTimestamp = pGpuTimer->mTimestamps[i];
        if(currentTimestamp == 0)
        {
            continue;
        }

        float period = pGpuTimer->pRenderer->mVkDeviceProperties.limits.timestampPeriod;
        double ns = (currentTimestamp - lastTimestamp) * period;
        double ms = ns / 1e6;

        String text = strf(pScratchArena, "[%s]: %.3f ms",
                cstr(pGpuTimer->mTimestampNames[i]),
                ms);
        uiText(text);

        lastTimestamp = currentTimestamp;
    }
    uiEndWindow();

    ARENA_CHECKPOINT_RESET(pScratchArena, gpuTimingsUI);
}

#endif
