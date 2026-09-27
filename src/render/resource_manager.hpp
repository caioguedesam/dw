#pragma once
#include "../core/base.hpp"
#include "../core/array.hpp"
#include "../render/buffer.hpp"
#include "../render/texture.hpp"

struct Renderer;
struct RenderTargetDesc;
struct RenderTarget;
struct ResourceSet;
struct TextureResource;

struct TextureHandle
{
    HND mHandleReadOnly = HND_INVALID;
    HND mHandleReadWrite = HND_INVALID;
};

enum ResourceType
{
    RESOURCE_TYPE_BUFFER,
    RESOURCE_TYPE_TEXTURE,
    RESOURCE_TYPE_SAMPLER,
};

template<typename T>
struct ResourceArray
{
    ResourceType    mType;
    Array<T>        mResources = {};
    Array<HND>      mFreeList = {};
};
template<typename T>
ResourceArray<T> createResourceArray(Arena* pArena, ResourceType type, uint64 capacity)
{
    ResourceArray<T> arr = {};
    arr.mType = type;
    arr.mResources = array<T>(pArena, capacity);
    arr.mFreeList = array<uint32>(pArena, capacity);
    return arr;
}
template<typename T>
HND addToResourceArray(ResourceArray<T>* pArray, T resource)
{
    if(pArray->mFreeList.mCount)
    {
        HND freeIdx = pArray->mFreeList.top();
        pArray->mFreeList.pop();
        pArray->mResources[freeIdx] = resource;
        return freeIdx;
    }
    pArray->mResources.push(resource);
    return (HND)(pArray->mResources.mCount - 1);
}
template<typename T>
void removeFromResourceArray(ResourceArray<T>* pArray, HND handle)
{
    ASSERT(handle < pArray->mResources.mCount);
    pArray->mResources[handle] = {};
    pArray->mFreeList.push(handle);
}

struct ResourceManager
{
    Renderer* pRenderer = NULL;

    ResourceArray<Buffer*> mBuffers;
    ResourceArray<TextureResource> mTextures;
    ResourceArray<Sampler*> mSamplers;

    Buffer* pGlobalAddrBuffer = NULL;
    bool mResourceSetInitialized = false;
};

#define MAX_TEXTURES 1024
#define MAX_BUFFERS  1024
#define MAX_SAMPLERS 256

void initResourceManager(Renderer* pRenderer, Arena* pArena, ResourceManager* pResMan);

// Resource initialization functions. These create the resource in the renderer and assign them a shader handle
void initBuffer(ResourceManager* pResMan, BufferDesc desc, Buffer** ppBuffer, void* pSrc = NULL);
void destroyBuffer(ResourceManager* pResMan, Buffer** ppBuffer);
void initTexture(ResourceManager* pResMan, TextureDesc desc, Texture** ppTexture);
void destroyTexture(ResourceManager* pResMan, Texture** ppTexture);
void initSampler(ResourceManager* pResMan, SamplerDesc desc, Sampler** ppSampler);
void destroySampler(ResourceManager* pResMan, Sampler** ppSampler);
void initRenderTarget(ResourceManager* pResMan, RenderTargetDesc desc, RenderTarget** ppTarget);
void initDepthTarget(ResourceManager* pResMan, RenderTargetDesc desc, RenderTarget** ppTarget);
void destroyRenderTarget(ResourceManager* pResMan, RenderTarget** ppTarget);

void setResources(ResourceManager* pResMan, Arena* pArena);
void unsetResources(ResourceManager* pResMan);
