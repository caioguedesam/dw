#include "command_buffer.hpp"
#include "render.hpp"
#include "../core/debug.hpp"
#include "vulkan/vulkan_core.h"

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

    for(uint32 i = 0; i < MAX_COMMAND_BUFFERS; i++)
    {
        pRenderer->mCommandBuffers[i].mState = COMMAND_BUFFER_IDLE;
        pRenderer->mCommandBuffers[i].mVkCmd = vkCommandBuffers[i];
        pRenderer->mCommandBuffers[i].mVkFence = VK_NULL_HANDLE;
    }
}

CommandBuffer* getCmd(Renderer* pRenderer, bool immediate)
{
    VkFence fence;
    if(immediate)
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
                    pOutCmd = pCmd;
                    break;
                }
            }
        }
    }

    VkResult ret = vkResetCommandBuffer(pOutCmd->mVkCmd, 0);
    ASSERTVK(ret);
    pOutCmd->mVkFence = fence;
    ret = vkResetFences(pRenderer->mVkDevice, 1, &pOutCmd->mVkFence);
    ASSERTVK(ret);
    pOutCmd->mState = COMMAND_BUFFER_IDLE;

    ASSERT(pOutCmd);
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
}

void endCmd(CommandBuffer* pCmd)
{
    ASSERT(pCmd && pCmd->mState == COMMAND_BUFFER_RECORDING);

    VkResult ret = vkEndCommandBuffer(pCmd->mVkCmd);
    ASSERTVK(ret);

    pCmd->mState = COMMAND_BUFFER_READY;
}

void submitFrameCmd(Renderer* pRenderer, CommandBuffer* pCmd)
{
    ASSERT(pRenderer);
    ASSERT(pCmd && pCmd->mState == COMMAND_BUFFER_READY);

    VkSubmitInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    info.commandBufferCount = 1;
    info.pCommandBuffers = &pCmd->mVkCmd;

    VkSemaphore vkRenderSemaphore = pRenderer->mVkRenderSemaphores[pRenderer->mActiveFrame];
    VkSemaphore vkPresentSemaphore = pRenderer->mVkPresentSemaphores[pRenderer->mActiveFrame];

    VkPipelineStageFlags vkWaitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    info.pWaitDstStageMask = &vkWaitStage;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &vkPresentSemaphore;
    info.signalSemaphoreCount = 1;
    info.pSignalSemaphores = &vkRenderSemaphore;

    VkResult ret = vkQueueSubmit(pRenderer->mVkQueue, 
            1, 
            &info, 
            pCmd->mVkFence);
    ASSERTVK(ret);

    pCmd->mState = COMMAND_BUFFER_SUBMITTED;
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

    pCmd->mState = COMMAND_BUFFER_SUBMITTED;

    ret = vkWaitForFences(pRenderer->mVkDevice, 1, &pCmd->mVkFence, VK_TRUE, MAX_UINT64);
    ASSERTVK(ret);
}
