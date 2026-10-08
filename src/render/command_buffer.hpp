#pragma once
#include "../core/base.hpp"
#include "descriptor.hpp"
#include "vulkan/vulkan_core.h"

struct Renderer;

enum CommandBufferState : uint32
{
    COMMAND_BUFFER_INVALID = 0,
    COMMAND_BUFFER_IDLE,
    COMMAND_BUFFER_RECORDING,
    COMMAND_BUFFER_READY,
    COMMAND_BUFFER_SUBMITTED,
};

enum CommandBufferType : uint32
{
    COMMAND_BUFFER_TYPE_FRAME = 0,
    COMMAND_BUFFER_TYPE_IMMEDIATE,
};

struct CommandBuffer
{
    CommandBufferState mState = COMMAND_BUFFER_INVALID;

    VkCommandBuffer mVkCmd = VK_NULL_HANDLE;
    VkFence mVkFence = VK_NULL_HANDLE;

    // Shader constants in the command buffer issuing the commands
    byte mShaderConstantData[MAX_SHADER_CONSTANT_SIZE];
    uint32 mShaderConstantSize = 0;

    // Vulkan function pointers for extension commands
    PFN_vkCmdBindResourceHeapEXT pfn_vkCmdBindResourceHeapEXT;
    PFN_vkCmdBindSamplerHeapEXT pfn_vkCmdBindSamplerHeapEXT;
    PFN_vkCmdPushDataEXT pfn_vkCmdPushDataEXT;
};

#define MAX_COMMAND_BUFFERS 16

void initCommandBuffers(Renderer* pRenderer);

CommandBuffer* getCmd(Renderer* pRenderer, CommandBufferType type = COMMAND_BUFFER_TYPE_FRAME);
void beginCmd(CommandBuffer* pCmd);
void endCmd(CommandBuffer* pCmd);
void submitFrameCmd(Renderer* pRenderer, CommandBuffer* pCmd);
void submitImmediateCmd(Renderer* pRenderer, CommandBuffer* pCmd);
