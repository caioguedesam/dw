#include "../render/resource_manager.hpp"
#include "../render/render.hpp"

void initTexture(ResourceManager<Texture>* pResMan, TextureDesc desc, Texture** ppTexture)
{
    ASSERT(pResMan && ppTexture);
    addTexture(pResMan->pRenderer, desc, ppTexture);

    HND handle = pResMan->mHandleQueue.top();
    pResMan->mHandleQueue.pop();

    pResMan->pResources[handle] = *ppTexture;
    (*ppTexture)->mHandle = handle;
}

void destroyTexture(ResourceManager<Texture>* pResMan, Texture** ppTexture)
{
    ASSERT(pResMan && ppTexture);

    HND handle = (*ppTexture)->mHandle;
    pResMan->pResources[handle] = NULL;
    pResMan->mHandleQueue.push(handle);

    removeTexture(pResMan->pRenderer, ppTexture);
}

void initRenderTarget(ResourceManager<Texture>* pResMan, RenderTargetDesc desc, RenderTarget** ppTarget)
{
    ASSERT(pResMan && ppTarget);
    addRenderTarget(pResMan->pRenderer, desc, ppTarget);

    HND handle = pResMan->mHandleQueue.top();
    pResMan->mHandleQueue.pop();

    pResMan->pResources[handle] = (*ppTarget)->pTexture;
    (*ppTarget)->pTexture->mHandle = handle;
}

void initDepthTarget(ResourceManager<Texture>* pResMan, RenderTargetDesc desc, RenderTarget** ppTarget)
{
    ASSERT(pResMan && ppTarget);
    addDepthTarget(pResMan->pRenderer, desc, ppTarget);

    HND handle = pResMan->mHandleQueue.top();
    pResMan->mHandleQueue.pop();

    pResMan->pResources[handle] = (*ppTarget)->pTexture;
    (*ppTarget)->pTexture->mHandle = handle;
}

void destroyRenderTarget(ResourceManager<Texture>* pResMan, RenderTarget** ppTarget)
{
    ASSERT(pResMan && ppTarget);

    HND handle = (*ppTarget)->pTexture->mHandle;
    pResMan->pResources[handle] = NULL;
    pResMan->mHandleQueue.push(handle);

    removeRenderTarget(pResMan->pRenderer, ppTarget);
}

void getSampledTextureResources(ResourceManager<Texture>* pResMan, uint64 count, Texture* pFallback, Texture** pOut)
{
    ASSERT(pResMan && pOut && pFallback);
    ASSERT(pFallback->mDesc.mUsage & TEXTURE_USAGE_SAMPLED);

    for(uint64 i = 0; i < count; i++)
    {
        pOut[i] = pFallback;
    }

    for(uint64 i = 0; i < pResMan->pResources.mCount; i++)
    {
        Texture* pTexture = pResMan->pResources[i];
        if(pTexture && pTexture->mDesc.mUsage & TEXTURE_USAGE_SAMPLED)
        {
            pOut[i] = pTexture;
        }
    }
}

void getStorageTextureResources(ResourceManager<Texture>* pResMan, uint64 count, Texture* pFallback, Texture** pOut)
{
    ASSERT(pResMan && pOut && pFallback);
    ASSERT(pFallback->mDesc.mUsage & TEXTURE_USAGE_STORAGE);

    for(uint64 i = 0; i < count; i++)
    {
        pOut[i] = pFallback;
    }

    for(uint64 i = 0; i < pResMan->pResources.mCount; i++)
    {
        Texture* pTexture = pResMan->pResources[i];
        if(pTexture && pTexture->mDesc.mUsage & TEXTURE_USAGE_STORAGE)
        {
            pOut[i] = pTexture;
        }
    }
}
