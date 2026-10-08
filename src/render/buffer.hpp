#pragma once
#include "../core/base.hpp"
#include "vulkan/vulkan_core.h"
#include "vma/vk_mem_alloc.h"

struct Renderer;
struct CommandBuffer;

// --------------------------------------
// Buffer
enum BufferType : uint32
{
    BUFFER_TYPE_INVALID         = 0,
    BUFFER_TYPE_VERTEX          = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    BUFFER_TYPE_INDEX           = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
    BUFFER_TYPE_UNIFORM         = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    BUFFER_TYPE_STORAGE         = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
    BUFFER_TYPE_TRANSFER_SRC    = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    BUFFER_TYPE_TRANSFER_DST    = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
    BUFFER_TYPE_INDIRECT        = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT
                                | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
                                | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
};

enum BufferFlags : uint32
{
    BUFFER_FLAGS_NONE = 0,
    BUFFER_FLAGS_HOST_MAPPED = BIT(1),
};

struct BufferDesc
{
    uint32 mType    = BUFFER_TYPE_INVALID;
    uint64 mSize    = 0;    // Total buffer size in bytes
    uint64 mStride  = 0;    // Size in bytes between elements
    uint64 mCount   = 0;    // Number of elements in buffer
    uint64 mAlign   = 0;    // Alignment (0 defaults to API alignment requirements)
    uint32 mFlags   = BUFFER_FLAGS_NONE;
};

struct Buffer
{
    BufferDesc mDesc = {};

    VkBuffer        mVkBuffer           = VK_NULL_HANDLE;
    VmaAllocation   mVkAllocation       = VK_NULL_HANDLE;
    VkDeviceAddress mVkDeviceAddr       = 0;

    HND mGPUHandle = HND_INVALID;
};

void addBuffer(Renderer* pRenderer, BufferDesc desc, Buffer** ppBuffer, void* pSrc = NULL, uint64 srcSize = 0);
void removeBuffer(Renderer* pRenderer, Buffer** ppBuffer);
HND getHandle(Buffer* pBuffer);
uint64 getBufferAddress(Buffer* pBuffer);

void* mapBufferMemory(Renderer* pRenderer, Buffer* pBuffer);
void  unmapBufferMemory(Renderer* pRenderer, Buffer* pBuffer);

uint32  getBufferAlignment(Renderer* pRenderer, BufferDesc bufferDesc);
void    copyHostDataToBuffer(Renderer* pRenderer, Buffer* pDst, uint64 dstOffset, void* srcData, uint64 srcSize);

// Render commands
void cmdCopyBuffer(CommandBuffer* pCmd, Buffer* pSrc, Buffer* pDst,
        uint64 srcOffset, uint64 srcSize, uint64 dstOffset);
