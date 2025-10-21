#include "buffer.hpp"
#include "render.hpp"
#include "../core/debug.hpp"

void addBuffer(Renderer* pRenderer, BufferDesc desc, Buffer** ppBuffer, void* pSrc)
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

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

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
    (*ppBuffer)->mVkHandle = vkBuffer;
    (*ppBuffer)->mVkAllocation = vkAlloc;

    if(pSrc)
    {
        copyToBuffer(pRenderer, *ppBuffer, 0, pSrc, desc.mSize);
    }
}

void removeBuffer(Renderer* pRenderer, Buffer** ppBuffer)
{
    ASSERT(pRenderer && ppBuffer);
    ASSERT(*ppBuffer);

    vmaDestroyBuffer(
            pRenderer->mVkAllocator,
            (*ppBuffer)->mVkHandle,
            (*ppBuffer)->mVkAllocation);

    **ppBuffer = {};

    poolFree(&pRenderer->poolBuffers, *ppBuffer);
    *ppBuffer = NULL;
}

uint32 getBufferAlignment(Renderer* pRenderer, Buffer* pBuffer)
{
    ASSERT(pRenderer && pBuffer);
    switch(pBuffer->mDesc.mType)
    {
        case BUFFER_TYPE_UNIFORM:
            return pRenderer->mVkDeviceProperties.limits.minUniformBufferOffsetAlignment;
        case BUFFER_TYPE_STORAGE:
            return pRenderer->mVkDeviceProperties.limits.minStorageBufferOffsetAlignment;
        default: return 1;
    }
}

void copyToBuffer(Renderer* pRenderer, Buffer* pDst, uint64 dstOffset, void* srcData, uint64 srcSize)
{
    ASSERT(pRenderer && pDst && srcData);
    
    uint32 align = getBufferAlignment(pRenderer, pDst);
    ASSERT(srcSize % align == 0);

    void* pMapping = NULL;
    VkResult ret = vmaMapMemory(pRenderer->mVkAllocator, pDst->mVkAllocation, &pMapping);
    ASSERTVK(ret);
    void* pStart = (void*)((uint64)pMapping + dstOffset);
    memcpy(pStart, srcData, srcSize);
    vmaUnmapMemory(pRenderer->mVkAllocator, pDst->mVkAllocation);
}
