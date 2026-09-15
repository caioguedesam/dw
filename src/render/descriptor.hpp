#pragma once

#include "../core/base.hpp"
#include "vulkan/vulkan_core.h"

// TODO(caio): Rename this to resource set.h/cpp or something

struct Renderer;
struct Texture;
struct Sampler;
struct Buffer;

// These sizes for shader constant blocks and global address constant buffer follow
// Vulkan's guaranteed minimum max sizes. Could be larger depending on hardware.
#define MAX_SHADER_CONSTANT_SIZE 128 * sizeof(byte)
#define ADDR_BUFFER_SIZE KB(16)

#define MAX_TEXTURE_DESCRIPTORS 4096
#define MAX_SAMPLER_DESCRIPTORS 256

struct ResourceSet
{
    VkDescriptorSetLayout mVkLayout = VK_NULL_HANDLE;
    VkDescriptorSet mVkSet = VK_NULL_HANDLE;
    // Single pipeline binding layout for both graphics and compute
    VkPipelineLayout mVkPipelineBindingLayout = VK_NULL_HANDLE;
};

void initResourceSet(Renderer* pRenderer, 
        Texture** ppTextures, uint32 textureCount,
        Sampler** ppSamplers, uint32 samplerCount,
        Buffer* pAddrBuffer);
void destroyResourceSet(Renderer* pRenderer);

// Descriptor heap implementation (INCOMPLETE)
// Leaving this here as reference for a future when this is more widely supported by graphics drivers
#if 0

struct Buffer;
struct Texture;
struct Sampler;
struct Renderer;

// TODO(caio): Variable sized descriptor heap. Currently fixing at 4096 descriptors.
#define RESOURCE_HEAP_MAX_DESCRIPTORS 65536
#define RESOURCE_HEAP_MAX_SAMPLERS 1024

struct DescriptorHeap
{
    // TODO(caio): Using a single descriptor size here. This could be optimized for smaller descriptor sizes depending on resource.
    uint32 mResourceDescriptorSize = 0;
    uint32 mResourceDescriptorAlign = 0;
    uint32 mResourceDescriptorStride = 0;
    uint32 mResourceDescriptorCount = 0;
    uint32 mSamplerDescriptorSize = 0;
    uint32 mSamplerDescriptorAlign = 0;
    uint32 mSamplerDescriptorStride = 0;
    uint32 mSamplerDescriptorCount = 0;

    uint64 mResourceHeapReservedSize = 0;
    uint64 mSamplerHeapOffset = 0;
    uint64 mSamplerHeapReservedSize = 0;

    // Backing buffer for ResourceDescriptorHeap and SamplerDescriptorHeap
    Buffer* pHeapBuffer = NULL;

    // Bindings to resource and sampler sections of the backing buffer
    VkBindHeapInfoEXT mVkBindInfoResources = {};
    VkBindHeapInfoEXT mVkBindInfoSamplers = {};

    // Vulkan function pointers
    PFN_vkWriteResourceDescriptorsEXT pfn_vkWriteResourceDescriptorsEXT;
    PFN_vkWriteSamplerDescriptorsEXT pfn_vkWriteSamplerDescriptorsEXT;
};

void initDescriptorHeap(Renderer* pRenderer, DescriptorHeap* pHeap);
void destroyDescriptorHeap(Renderer* pRenderer, DescriptorHeap* pHeap);

HND addToDescriptorHeap(Renderer* pRenderer, Buffer* pBuffer);
HND addToDescriptorHeap(Renderer* pRenderer, Texture* pTexture, bool readWrite);
HND addToDescriptorHeap(Renderer* pRenderer, Sampler* pSampler);
#endif
