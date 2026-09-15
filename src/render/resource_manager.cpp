#include "../render/resource_manager.hpp"
#include "../render/descriptor.hpp"
#include "../render/render.hpp"
#include "../render/buffer.hpp"
#include "src/core/memory.hpp"

ResourceArray createResourceArray(Arena* pArena, ResourceType type, uint64 capacity)
{
    ResourceArray arr = {};
    arr.mType = type;
    arr.mResources = array<void*>(pArena, capacity);
    arr.mFreeList = array<uint32>(pArena, capacity);
    return arr;
}

HND addToResourceArray(ResourceArray* pArray, void* pResource)
{
    if(pArray->mFreeList.mCount)
    {
        HND freeIdx = pArray->mFreeList.top();
        pArray->mFreeList.pop();
        pArray->mResources[freeIdx] = pResource;
        return freeIdx;
    }
    pArray->mResources.push(pResource);
    return (HND)(pArray->mResources.mCount - 1);
}

void removeFromResourceArray(ResourceArray* pArray, HND handle)
{
    // Textures have two separate entries
    ASSERT(handle < pArray->mResources.mCount);
    pArray->mResources[handle] = NULL;
    pArray->mFreeList.push(handle);
}

void initResourceManager(Renderer* pRenderer, Arena* pArena, ResourceManager* pResMan)
{
    ASSERT(pRenderer && pArena && pResMan);
    *pResMan = {};
    pResMan->pRenderer  = pRenderer;
    pResMan->mBuffers   = createResourceArray(pArena, RESOURCE_TYPE_BUFFER,  MAX_BUFFERS);
    pResMan->mTextures  = createResourceArray(pArena, RESOURCE_TYPE_TEXTURE, MAX_TEXTURES);
    pResMan->mSamplers  = createResourceArray(pArena, RESOURCE_TYPE_SAMPLER, MAX_SAMPLERS);
}

void initBuffer(ResourceManager* pResMan, BufferDesc desc, Buffer** ppBuffer, void* pSrc)
{
    ASSERT(pResMan && ppBuffer);
    addBuffer(pResMan->pRenderer, desc, ppBuffer, pSrc);
    
    HND handle = addToResourceArray(&pResMan->mBuffers, *ppBuffer);
    (*ppBuffer)->mGPUHandle = handle;
}

void destroyBuffer(ResourceManager* pResMan, Buffer** ppBuffer)
{
    ASSERT(pResMan && ppBuffer && (*ppBuffer)->mGPUHandle != HND_INVALID);

    removeFromResourceArray(&pResMan->mBuffers, (*ppBuffer)->mGPUHandle);
    removeBuffer(pResMan->pRenderer, ppBuffer);
}

void initTexture(ResourceManager* pResMan, TextureDesc desc, Texture** ppTexture)
{
    ASSERT(pResMan && ppTexture);
    addTexture(pResMan->pRenderer, desc, ppTexture);

    Texture* pTexture = *ppTexture;
    ASSERT(pTexture);

    if(pTexture->mDesc.mUsage & TEXTURE_USAGE_SAMPLED)
    {
        HND handle = addToResourceArray(&pResMan->mTextures, pTexture);
        pTexture->mGPUHandle = handle;
    }
    if(pTexture->mDesc.mUsage & TEXTURE_USAGE_STORAGE)
    {
        HND rwhandle = addToResourceArray(&pResMan->mTextures, pTexture);
        pTexture->mGPURWHandle = rwhandle;
    }
}

void destroyTexture(ResourceManager* pResMan, Texture** ppTexture)
{
    ASSERT(pResMan && ppTexture);

    Texture* pTexture = *ppTexture;
    ASSERT(pTexture);

    if(pTexture->mDesc.mUsage & TEXTURE_USAGE_SAMPLED)
    {
        removeFromResourceArray(&pResMan->mTextures, pTexture->mGPUHandle);
    }
    if(pTexture->mDesc.mUsage & TEXTURE_USAGE_STORAGE)
    {
        removeFromResourceArray(&pResMan->mTextures, pTexture->mGPURWHandle);
    }

    removeTexture(pResMan->pRenderer, ppTexture);
}

void initSampler(ResourceManager* pResMan, SamplerDesc desc, Sampler** ppSampler)
{
    ASSERT(pResMan && ppSampler);
    addSampler(pResMan->pRenderer, desc, ppSampler);

    HND handle = addToResourceArray(&pResMan->mSamplers, *ppSampler);
    (*ppSampler)->mGPUHandle = handle;
}

void destroySampler(ResourceManager* pResMan, Sampler** ppSampler)
{
    ASSERT(pResMan && ppSampler);
    removeFromResourceArray(&pResMan->mSamplers, (*ppSampler)->mGPUHandle);
    removeSampler(pResMan->pRenderer, ppSampler);
}

void initRenderTarget(ResourceManager* pResMan, RenderTargetDesc desc, RenderTarget** ppTarget)
{
    ASSERT(pResMan && ppTarget);
    Renderer* pRenderer = pResMan->pRenderer;
    addRenderTarget(pRenderer, desc, ppTarget);

    Texture* pTexture = (*ppTarget)->pTexture;
    ASSERT(pTexture);

    if(pTexture->mDesc.mUsage & TEXTURE_USAGE_SAMPLED)
    {
        HND handle = addToResourceArray(&pResMan->mTextures, pTexture);
        pTexture->mGPUHandle = handle;
    }
    if(pTexture->mDesc.mUsage & TEXTURE_USAGE_STORAGE)
    {
        HND rwhandle = addToResourceArray(&pResMan->mTextures, pTexture);
        pTexture->mGPURWHandle = rwhandle;
    }
}

void initDepthTarget(ResourceManager* pResMan, RenderTargetDesc desc, RenderTarget** ppTarget)
{
    ASSERT(pResMan && ppTarget);
    Renderer* pRenderer = pResMan->pRenderer;
    addDepthTarget(pRenderer, desc, ppTarget);

    Texture* pTexture = (*ppTarget)->pTexture;
    ASSERT(pTexture);

    if(pTexture->mDesc.mUsage & TEXTURE_USAGE_SAMPLED)
    {
        HND handle = addToResourceArray(&pResMan->mTextures, pTexture);
        pTexture->mGPUHandle = handle;
    }
    if(pTexture->mDesc.mUsage & TEXTURE_USAGE_STORAGE)
    {
        HND rwhandle = addToResourceArray(&pResMan->mTextures, pTexture);
        pTexture->mGPURWHandle = rwhandle;
    } 
}

void destroyRenderTarget(ResourceManager* pResMan, RenderTarget** ppTarget)
{
    ASSERT(pResMan && ppTarget);

    Texture* pTexture = (*ppTarget)->pTexture;
    ASSERT(pTexture);

    if(pTexture->mDesc.mUsage & TEXTURE_USAGE_SAMPLED)
    {
        removeFromResourceArray(&pResMan->mTextures, pTexture->mGPUHandle);
    }
    if(pTexture->mDesc.mUsage & TEXTURE_USAGE_STORAGE)
    {
        removeFromResourceArray(&pResMan->mTextures, pTexture->mGPURWHandle);
    }

    removeRenderTarget(pResMan->pRenderer, ppTarget);
}

STATIC_ASSERT(sizeof(uint32*) == sizeof(uint64));

void setResources(ResourceManager* pResMan, Arena* pArena)
{
    ASSERT(pResMan && !pResMan->mResourceSetInitialized);

    ARENA_CHECKPOINT_SET(pArena, setResources);

    uint32 bufferCount = pResMan->mBuffers.mResources.mCount;
    Array<uint64> bufferAddresses = array<uint64>(pArena, bufferCount);
    for(uint32 i = 0; i < bufferCount; i++)
    {
        Buffer* pBuffer = (Buffer*)pResMan->mBuffers.mResources[i];
        uint64 gpuAddr = getBufferAddress(pBuffer);
        bufferAddresses.push(gpuAddr);
    }

    BufferDesc addrBufferDesc = {};
    addrBufferDesc.mType = BUFFER_TYPE_UNIFORM;
    addrBufferDesc.mSize = ADDR_BUFFER_SIZE;
    addrBufferDesc.mStride = sizeof(uint32*);
    addBuffer(pResMan->pRenderer, addrBufferDesc, &pResMan->pGlobalAddrBuffer);
    copyToBuffer(pResMan->pRenderer, pResMan->pGlobalAddrBuffer, 0, 
            bufferAddresses.mData, bufferCount * sizeof(uint32*));

    initResourceSet(pResMan->pRenderer, 
            (Texture**)pResMan->mTextures.mResources.mData, pResMan->mTextures.mResources.mCount, 
            (Sampler**)pResMan->mSamplers.mResources.mData, pResMan->mSamplers.mResources.mCount, 
            pResMan->pGlobalAddrBuffer);

    pResMan->mResourceSetInitialized = true;

    ARENA_CHECKPOINT_RESET(pArena, setResources);
}

void unsetResources(ResourceManager* pResMan)
{
    ASSERT(pResMan && pResMan->mResourceSetInitialized);

    destroyResourceSet(pResMan->pRenderer);
    removeBuffer(pResMan->pRenderer, &pResMan->pGlobalAddrBuffer);

    pResMan->mResourceSetInitialized = false;
}
