#pragma once
#include "../core/base.hpp"
#include "../core/array.hpp"

struct Renderer;
struct TextureDesc;
struct Texture;
struct RenderTargetDesc;
struct RenderTarget;

template<typename T>
struct ResourceManager
{
    Renderer* pRenderer = NULL;
    Array<T*> pResources;
    Array<HND> mHandleQueue;
};

template<typename T>
ResourceManager<T> createResourceManager(Renderer* pRenderer, Arena* pArena, uint64 maxResources)
{
    ASSERT(pRenderer && pArena);
    ResourceManager<T> resMan;
    resMan.pRenderer = pRenderer;
    resMan.pResources = array<T*>(pArena, maxResources, maxResources, NULL);
    resMan.mHandleQueue = array<HND>(pArena, maxResources);

    // Queue all handles for assigning to resources
    HND handle = (HND)(maxResources - 1);
    for(uint64 i = 0; i < maxResources; i++)
    {
        resMan.mHandleQueue.push(handle);
        handle--;
    }

    return resMan;
}

void initTexture(ResourceManager<Texture>* pResMan, TextureDesc desc, Texture** ppTexture);
void destroyTexture(ResourceManager<Texture>* pResMan, Texture** ppTexture);
void initRenderTarget(ResourceManager<Texture>* pResMan, RenderTargetDesc desc, RenderTarget** ppTarget);
void initDepthTarget(ResourceManager<Texture>* pResMan, RenderTargetDesc desc, RenderTarget** ppTarget);
void destroyRenderTarget(ResourceManager<Texture>* pResMan, RenderTarget** ppTarget);

void getSampledTextureResources(ResourceManager<Texture>* pResMan, uint64 count, Texture* pFallback, Texture** pOut);
void getStorageTextureResources(ResourceManager<Texture>* pResMan, uint64 count, Texture* pFallback, Texture** pOut);


