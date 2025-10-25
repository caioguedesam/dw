#pragma once
#include "../core/base.hpp"
#include "vulkan/vulkan_core.h"

struct Renderer;

enum CommandBufferState
{
    COMMAND_BUFFER_INVALID = 0,
    COMMAND_BUFFER_IDLE,
    COMMAND_BUFFER_RECORDING,
    COMMAND_BUFFER_READY,
    COMMAND_BUFFER_SUBMITTED,
};

struct CommandBuffer
{
    CommandBufferState mState = COMMAND_BUFFER_INVALID;

    VkCommandBuffer mVkCmd = VK_NULL_HANDLE;
    VkFence mVkFence = VK_NULL_HANDLE;
};

#define MAX_COMMAND_BUFFERS 16

void initCommandBuffers(Renderer* pRenderer);

CommandBuffer* getCmd(Renderer* pRenderer, bool immediate = false);
void beginCmd(CommandBuffer* pCmd);
void endCmd(CommandBuffer* pCmd);
void submitFrameCmd(Renderer* pRenderer, CommandBuffer* pCmd);
void submitImmediateCmd(Renderer* pRenderer, CommandBuffer* pCmd);
