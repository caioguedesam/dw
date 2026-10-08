#include "command_buffer.hpp"
#include "render.hpp"
#include "../core/debug.hpp"
#include "vulkan/vulkan_core.h"

#define COMMAND_BUFFER_DEBUG 0

#if COMMAND_BUFFER_DEBUG
#define COMMAND_BUFFER_LOG(FMT, ...) LOGLF("COMMAND_BUFFER", FMT, __VA_ARGS__)
#else
#define COMMAND_BUFFER_LOG(FMT, ...)
#endif

void initCommandBuffers(Renderer* pRenderer)
{
    ASSERT(pRenderer);

    VkCommandBuffer vkCommandBuffers[MAX_COMMAND_BUFFERS];
    VkCommandBufferAllocateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.commandPool = pRenderer->mVkCommandPool;
    info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    info.commandBufferCount = MAX_COMMAND_BUFFERS;
    VkResult ret = vkAllocateCommandBuffers(pRenderer->mVkDevice, &info, vkCommandBuffers);
    ASSERTVK(ret);

    PFN_vkCmdBindResourceHeapEXT pfn_vkCmdBindResourceHeapEXT = 
        (PFN_vkCmdBindResourceHeapEXT)vkGetDeviceProcAddr(pRenderer->mVkDevice, "vkCmdBindResourceHeapEXT");
    PFN_vkCmdBindSamplerHeapEXT pfn_vkCmdBindSamplerHeapEXT = 
        (PFN_vkCmdBindSamplerHeapEXT)vkGetDeviceProcAddr(pRenderer->mVkDevice, "vkCmdBindSamplerHeapEXT");
    PFN_vkCmdPushDataEXT pfn_vkCmdPushDataEXT = 
        (PFN_vkCmdPushDataEXT)vkGetDeviceProcAddr(pRenderer->mVkDevice, "vkCmdPushDataEXT");

    for(uint32 i = 0; i < MAX_COMMAND_BUFFERS; i++)
    {
        pRenderer->mCommandBuffers[i].mState = COMMAND_BUFFER_IDLE;
        pRenderer->mCommandBuffers[i].mVkCmd = vkCommandBuffers[i];
        pRenderer->mCommandBuffers[i].mVkFence = VK_NULL_HANDLE;
        pRenderer->mCommandBuffers[i].pfn_vkCmdBindResourceHeapEXT = pfn_vkCmdBindResourceHeapEXT;
        pRenderer->mCommandBuffers[i].pfn_vkCmdBindSamplerHeapEXT = pfn_vkCmdBindSamplerHeapEXT;
        pRenderer->mCommandBuffers[i].pfn_vkCmdPushDataEXT = pfn_vkCmdPushDataEXT;
    }
}

CommandBuffer* getCmd(Renderer* pRenderer, CommandBufferType type)
{
    VkFence fence;
    if(type == COMMAND_BUFFER_TYPE_IMMEDIATE)
    {
        // Immediate command buffers already wait right after submit
        fence = pRenderer->mVkImmediateFence;
    }
    else
    {
        // Wait for this frame's fence to signal before recording another command buffer to it
        fence = pRenderer->mVkFences[pRenderer->mActiveFrame];
        VkResult ret = vkWaitForFences(pRenderer->mVkDevice, 1, &fence, VK_TRUE, MAX_UINT64);
        ASSERTVK(ret);
    }

    CommandBuffer* pOutCmd = NULL;

    // Find an idle command buffer
    for(uint32 i = 0; i < MAX_COMMAND_BUFFERS; i++)
    {
        CommandBuffer* pCmd = &pRenderer->mCommandBuffers[i];
        if(pCmd->mState == COMMAND_BUFFER_IDLE)
        {
            COMMAND_BUFFER_LOG("getCmd (%p) from IDLE", pCmd->mVkCmd);
            pOutCmd = pCmd;
            break;
        }
    }

    if(!pOutCmd)
    {
        // Look for a submitted command buffer, then reset it to idle if finished.
        for(uint32 i = 0; i < MAX_COMMAND_BUFFERS; i++)
        {
            CommandBuffer* pCmd = &pRenderer->mCommandBuffers[i];
            if(pCmd->mState == COMMAND_BUFFER_SUBMITTED)
            {
                ASSERT(pCmd->mVkFence != VK_NULL_HANDLE);
                VkResult ret = vkGetFenceStatus(pRenderer->mVkDevice, pCmd->mVkFence);
                if(ret == VK_SUCCESS)
                {
                    COMMAND_BUFFER_LOG("getCmd (%p) from SUBMITTED", pCmd->mVkCmd);
                    pOutCmd = pCmd;
                    break;
                }
            }
        }
    }
    ASSERT(pOutCmd);

    VkResult ret = vkResetCommandBuffer(pOutCmd->mVkCmd, 0);
    ASSERTVK(ret);
    pOutCmd->mVkFence = fence;
    ret = vkResetFences(pRenderer->mVkDevice, 1, &pOutCmd->mVkFence);
    ASSERTVK(ret);
    pOutCmd->mState = COMMAND_BUFFER_IDLE;
    COMMAND_BUFFER_LOG("(%p) is IDLE", pOutCmd->mVkCmd);

    return pOutCmd;
}

void beginCmd(CommandBuffer* pCmd)
{
    ASSERT(pCmd && pCmd->mState == COMMAND_BUFFER_IDLE);

    VkCommandBufferBeginInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VkResult ret = vkBeginCommandBuffer(pCmd->mVkCmd, &info);
    ASSERTVK(ret);

    pCmd->mState = COMMAND_BUFFER_RECORDING;
    COMMAND_BUFFER_LOG("(%p) is RECORDING", pCmd->mVkCmd);
}

void endCmd(CommandBuffer* pCmd)
{
    ASSERT(pCmd && pCmd->mState == COMMAND_BUFFER_RECORDING);

    VkResult ret = vkEndCommandBuffer(pCmd->mVkCmd);
    ASSERTVK(ret);

    pCmd->mState = COMMAND_BUFFER_READY;
    COMMAND_BUFFER_LOG("(%p) is READY", pCmd->mVkCmd);
}

void submitFrameCmd(Renderer* pRenderer, CommandBuffer* pCmd)
{
    ASSERT(pRenderer);
    ASSERT(pCmd && pCmd->mState == COMMAND_BUFFER_READY);

    VkSubmitInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    info.commandBufferCount = 1;
    info.pCommandBuffers = &pCmd->mVkCmd;

    VkSemaphore vkRenderFinishedSemaphore = pRenderer->mVkRenderFinishedSemaphores[pRenderer->mSwapChain.mActiveImage];
    VkSemaphore vkImageAcquiredSemaphore = pRenderer->mVkImageAcquiredSemaphores[pRenderer->mActiveFrame];

    VkPipelineStageFlags vkWaitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    info.pWaitDstStageMask = &vkWaitStage;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &vkImageAcquiredSemaphore;
    info.signalSemaphoreCount = 1;
    info.pSignalSemaphores = &vkRenderFinishedSemaphore;

    VkResult ret = vkQueueSubmit(pRenderer->mVkQueue, 
            1, 
            &info, 
            pCmd->mVkFence);
    ASSERTVK(ret);

    pCmd->mState = COMMAND_BUFFER_SUBMITTED;
    COMMAND_BUFFER_LOG("(%p) is SUBMITTED (frame)", pCmd->mVkCmd);
}

void submitImmediateCmd(Renderer* pRenderer, CommandBuffer* pCmd)
{
    ASSERT(pRenderer);
    ASSERT(pCmd && pCmd->mState == COMMAND_BUFFER_READY);

    VkSubmitInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    info.commandBufferCount = 1;
    info.pCommandBuffers = &pCmd->mVkCmd;

    VkResult ret = vkQueueSubmit(pRenderer->mVkQueue, 1, &info, pCmd->mVkFence);
    ASSERTVK(ret);
    COMMAND_BUFFER_LOG("(%p) is SUBMITTED (immediate)", pCmd->mVkCmd);

    pCmd->mState = COMMAND_BUFFER_SUBMITTED;

    ret = vkWaitForFences(pRenderer->mVkDevice, 1, &pCmd->mVkFence, VK_TRUE, MAX_UINT64);
    ASSERTVK(ret);

    COMMAND_BUFFER_LOG("(%p) fence signaled (immediate)", pCmd->mVkCmd);
}
