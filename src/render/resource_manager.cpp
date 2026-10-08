#include "../render/resource_manager.hpp"
#include "../render/descriptor.hpp"
#include "../render/render.hpp"
#include "../render/buffer.hpp"
#include "src/core/memory.hpp"

void initResourceManager(Renderer* pRenderer, Arena* pArena, ResourceManager* pResMan)
{
    ASSERT(pRenderer && pArena && pResMan);
    *pResMan = {};
    pResMan->pRenderer  = pRenderer;
    pResMan->mBuffers   = createResourceArray<Buffer*>(pArena, RESOURCE_TYPE_BUFFER,  MAX_BUFFERS);
    pResMan->mTextures  = createResourceArray<TextureResource>(pArena, RESOURCE_TYPE_TEXTURE, MAX_TEXTURES);
    pResMan->mSamplers  = createResourceArray<Sampler*>(pArena, RESOURCE_TYPE_SAMPLER, MAX_SAMPLERS);
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

void registerTexture(ResourceManager* pResMan, TextureDesc desc, Texture* pTexture)
{
    TextureResource res = {};
    res.pTexture = pTexture;
    // All texture mips sampled then all mips storage
    uint32 mipHandleCount = desc.mFlags & TEXTURE_FLAGS_SEPARATE_LEVELS ? desc.mMipCount : 1;
    if(desc.mUsage & TEXTURE_USAGE_SAMPLED)
    {
        for(uint32 i = 0; i < mipHandleCount; i++)
        {
            res.mMipLevel = i;
            HND handle = addToResourceArray(&pResMan->mTextures, res);
            pTexture->mGPUHandles[i] = handle;
        }
    }
    if(desc.mUsage & TEXTURE_USAGE_STORAGE)
    {
        for(uint32 i = 0; i < mipHandleCount; i++)
        {
            res.mMipLevel = i;
            HND handle = addToResourceArray(&pResMan->mTextures, res);
            pTexture->mGPURWHandles[i] = handle;
        }
    }
}

void unregisterTexture(ResourceManager* pResMan, Texture* pTexture)
{
    uint32 mipHandleCount = pTexture->mDesc.mFlags & TEXTURE_FLAGS_SEPARATE_LEVELS ? pTexture->mDesc.mMipCount : 1;
    if(pTexture->mDesc.mUsage & TEXTURE_USAGE_SAMPLED)
    {
        for(int32 i = 0; i < mipHandleCount; i++)
        {
            removeFromResourceArray(&pResMan->mTextures, pTexture->mGPUHandles[i]);
        }
    }
    if(pTexture->mDesc.mUsage & TEXTURE_USAGE_STORAGE)
    {
        for(int32 i = 0; i < mipHandleCount; i++)
        {
            removeFromResourceArray(&pResMan->mTextures, pTexture->mGPURWHandles[i]);
        } 
    }
}

void initTexture(ResourceManager* pResMan, TextureDesc desc, Texture** ppTexture)
{
    ASSERT(pResMan && ppTexture);
    addTexture(pResMan->pRenderer, desc, ppTexture);

    Texture* pTexture = *ppTexture;
    ASSERT(pTexture);

    registerTexture(pResMan, desc, pTexture);
}

void destroyTexture(ResourceManager* pResMan, Texture** ppTexture)
{
    ASSERT(pResMan && ppTexture);

    Texture* pTexture = *ppTexture;
    ASSERT(pTexture);

    unregisterTexture(pResMan, pTexture);

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

    registerTexture(pResMan, pTexture->mDesc, pTexture);
}

void initDepthTarget(ResourceManager* pResMan, RenderTargetDesc desc, RenderTarget** ppTarget)
{
    ASSERT(pResMan && ppTarget);
    Renderer* pRenderer = pResMan->pRenderer;
    addDepthTarget(pRenderer, desc, ppTarget);

    Texture* pTexture = (*ppTarget)->pTexture;
    ASSERT(pTexture);

    registerTexture(pResMan, pTexture->mDesc, pTexture);
}

void destroyRenderTarget(ResourceManager* pResMan, RenderTarget** ppTarget)
{
    ASSERT(pResMan && ppTarget);

    Texture* pTexture = (*ppTarget)->pTexture;
    ASSERT(pTexture);

    unregisterTexture(pResMan, pTexture);

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
    addrBufferDesc.mType = BUFFER_TYPE_UNIFORM | BUFFER_TYPE_TRANSFER_DST;
    addrBufferDesc.mSize = ADDR_BUFFER_SIZE;
    addrBufferDesc.mStride = sizeof(uint32*);
    addBuffer(pResMan->pRenderer, addrBufferDesc, &pResMan->pGlobalAddrBuffer, bufferAddresses.mData, bufferCount * sizeof(uint32*));
    //-copyHostDataToBuffer(pResMan->pRenderer, pResMan->pGlobalAddrBuffer, 0, 
    //-        bufferAddresses.mData, bufferCount * sizeof(uint32*));

    initResourceSet(pResMan->pRenderer, 
            (TextureResource*)pResMan->mTextures.mResources.mData, pResMan->mTextures.mResources.mCount, 
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
