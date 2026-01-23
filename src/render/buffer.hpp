#pragma once
#include "../core/base.hpp"
#include "vulkan/vulkan_core.h"
#include "vma/vk_mem_alloc.h"

struct Renderer;

// --------------------------------------
// Buffer
enum BufferType : uint32
{
    BUFFER_TYPE_INVALID     = 0,
    BUFFER_TYPE_VERTEX      = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    BUFFER_TYPE_INDEX       = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
    BUFFER_TYPE_UNIFORM     = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    BUFFER_TYPE_STORAGE     = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
    BUFFER_TYPE_STAGING     = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    BUFFER_TYPE_INDIRECT    = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT
                            | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
                            | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
};

struct BufferDesc
{
    BufferType mType = BUFFER_TYPE_INVALID;
    uint64 mSize    = 0;    // Total buffer size in bytes
    uint64 mStride  = 0;    // Size in bytes between elements
    uint64 mCount   = 0;    // Number of elements in buffer

    // TODO_DW: Should be able to alter buffer memory mapping (CPU, GPU, CPU and GPU etc)
};

struct Buffer
{
    BufferDesc mDesc = {};

    VkBuffer        mVkBuffer           = VK_NULL_HANDLE;
    VmaAllocation   mVkAllocation       = VK_NULL_HANDLE;
};

void addBuffer(Renderer* pRenderer, BufferDesc desc, Buffer** ppBuffer, void* pSrc = NULL);
void removeBuffer(Renderer* pRenderer, Buffer** ppBuffer);

uint32  getBufferAlignment(Renderer* pRenderer, Buffer* pBuffer);
void    copyToBuffer(Renderer* pRenderer, Buffer* pDst, uint64 dstOffset, void* srcData, uint64 srcSize);
