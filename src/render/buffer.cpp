#include "buffer.hpp"
#include "render.hpp"
#include "../core/debug.hpp"
#include "src/render/descriptor.hpp"
#include "vulkan/vulkan_core.h"

void addBuffer(Renderer* pRenderer, BufferDesc desc, Buffer** ppBuffer, void* pSrc, uint64 srcSize)
{
    ASSERT(pRenderer && ppBuffer);
    ASSERT(*ppBuffer == NULL);

    *ppBuffer = (Buffer*)poolAlloc(&pRenderer->poolBuffers);

    **ppBuffer = {};

    ASSERT(desc.mSize >= desc.mStride);

    VkBufferCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size = desc.mSize;
    info.usage = (VkBufferUsageFlags)desc.mType;
    info.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;    // For referencing buffer address in shaders

    uint64 alignment = desc.mAlign;
    if(alignment == 0)
    {
        alignment = getBufferAlignment(pRenderer, desc);
    }
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = 0;
    if(desc.mFlags & BUFFER_FLAGS_HOST_MAPPED)
    {
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    }
    //allocInfo.minAlignment = desc.mAlign;
    allocInfo.minAlignment = alignment;

    VkBuffer vkBuffer;
    VmaAllocation vkAlloc;
    VkResult ret = vmaCreateBuffer(
            pRenderer->mVkAllocator,
            &info,
            &allocInfo,
            &vkBuffer,
            &vkAlloc,
            NULL);
    ASSERTVK(ret);

    (*ppBuffer)->mDesc = desc;
    (*ppBuffer)->mVkBuffer = vkBuffer;
    (*ppBuffer)->mVkAllocation = vkAlloc;

    VkBufferDeviceAddressInfo vkAddrInfo = {};
    vkAddrInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    vkAddrInfo.buffer = vkBuffer;
    (*ppBuffer)->mVkDeviceAddr = vkGetBufferDeviceAddress(pRenderer->mVkDevice, &vkAddrInfo);

    if(pSrc)
    {
        uint64 copySize = srcSize == 0 ? desc.mSize : srcSize;
        if(desc.mFlags & BUFFER_FLAGS_HOST_MAPPED)
        {
            copyHostDataToBuffer(pRenderer, *ppBuffer, 0, pSrc, copySize);
        }
        else
        {
            // Copy first to staging buffer, then to device local buffer.
            // Data must fit within the staging buffer limits.
            ASSERT(desc.mSize < pRenderer->pStagingBuffer->mDesc.mSize);
            copyHostDataToBuffer(pRenderer, pRenderer->pStagingBuffer, 0, pSrc, copySize);

            CommandBuffer* pCmd = getCmd(pRenderer, COMMAND_BUFFER_TYPE_IMMEDIATE);
            beginCmd(pCmd);

            cmdCopyBuffer(pCmd, pRenderer->pStagingBuffer, *ppBuffer, 0, copySize, 0);

            endCmd(pCmd);
            submitImmediateCmd(pRenderer, pCmd);
        }
    }
}

void removeBuffer(Renderer* pRenderer, Buffer** ppBuffer)
{
    ASSERT(pRenderer && ppBuffer);
    ASSERT(*ppBuffer);

    vmaDestroyBuffer(
            pRenderer->mVkAllocator,
            (*ppBuffer)->mVkBuffer,
            (*ppBuffer)->mVkAllocation);

    **ppBuffer = {};

    poolFree(&pRenderer->poolBuffers, *ppBuffer);
    *ppBuffer = NULL;
}

HND getHandle(Buffer* pBuffer)
{
    ASSERT(pBuffer && pBuffer->mGPUHandle != HND_INVALID);
    return pBuffer->mGPUHandle;
}

uint64 getBufferAddress(Buffer* pBuffer)
{
    ASSERT(pBuffer && pBuffer->mGPUHandle != HND_INVALID);
    return pBuffer->mVkDeviceAddr;
}

void* mapBufferMemory(Renderer* pRenderer, Buffer* pBuffer)
{
    ASSERT(pBuffer && pBuffer->mDesc.mFlags & BUFFER_FLAGS_HOST_MAPPED);
    void* result;
    VkResult ret = vmaMapMemory(pRenderer->mVkAllocator, pBuffer->mVkAllocation, &result);
    ASSERTVK(ret);
    return result;
}

void unmapBufferMemory(Renderer* pRenderer, Buffer* pBuffer)
{
    ASSERT(pBuffer && pBuffer->mDesc.mFlags & BUFFER_FLAGS_HOST_MAPPED);
    vmaUnmapMemory(pRenderer->mVkAllocator, pBuffer->mVkAllocation);
}

uint32 getBufferAlignment(Renderer* pRenderer, BufferDesc bufferDesc)
{
    ASSERT(pRenderer);
    switch(bufferDesc.mType)
    {
        case BUFFER_TYPE_UNIFORM:
            return pRenderer->mVkDeviceProperties.limits.minUniformBufferOffsetAlignment;
        case BUFFER_TYPE_STORAGE:
            return pRenderer->mVkDeviceProperties.limits.minStorageBufferOffsetAlignment;
        default: return 1;
    }
}

void copyHostDataToBuffer(Renderer* pRenderer, Buffer* pDst, uint64 dstOffset, void* srcData, uint64 srcSize)
{
    ASSERT(pRenderer && pDst && srcData);
    
    void* pMapping = mapBufferMemory(pRenderer, pDst);
    void* pStart = (void*)((uint64)pMapping + dstOffset);
    memcpy(pStart, srcData, srcSize);
    unmapBufferMemory(pRenderer, pDst);
}

void cmdCopyBuffer(CommandBuffer* pCmd, Buffer* pSrc, Buffer* pDst,
        uint64 srcOffset, uint64 srcSize, uint64 dstOffset)
{
    ASSERT(pCmd && pSrc && pDst && srcSize);

    VkBufferCopy vkCopy = {};
    vkCopy.srcOffset = srcOffset;
    vkCopy.dstOffset = dstOffset;
    vkCopy.size = srcSize;

    vkCmdCopyBuffer(pCmd->mVkCmd, pSrc->mVkBuffer, pDst->mVkBuffer, 1, &vkCopy);
}
