#pragma once
#include "../core/base.hpp"
#include "../core/array.hpp"
#include "../render/buffer.hpp"
#include "../render/texture.hpp"

struct Renderer;
struct RenderTargetDesc;
struct RenderTarget;
struct ResourceSet;

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

struct ResourceArray
{
    ResourceType    mType;
    Array<void*>    mResources = {};
    Array<HND>      mFreeList = {};
};
ResourceArray createResourceArray(Arena* pArena, ResourceType type, uint64 capacity);
HND addToResourceArray(ResourceArray* pArray, void* pResource);
void removeFromResourceArray(ResourceArray* pArray, HND handle);

struct ResourceManager
{
    Renderer* pRenderer = NULL;

    ResourceArray mBuffers;
    ResourceArray mTextures;
    ResourceArray mSamplers;

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
