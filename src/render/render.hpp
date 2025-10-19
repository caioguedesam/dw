#pragma once
#define VK_USE_PLATFORM_WIN32_KHR
#include "../core/memory.hpp"
#include "vulkan/vulkan_core.h"
#include "vma/vk_mem_alloc.h"

#define ASSERTVK(EXPR) ASSERT((EXPR) == VK_SUCCESS)


struct RendererDesc
{
    uint64 mMaxBuffers  = 1024;
    uint64 mMaxTextures = 1024;
    uint64 mMaxSamplers = 64;
};

struct Renderer
{
    // Pools for reusable render data
    Pool* pPoolBuffer   = NULL;    
    Pool* pPoolTexture  = NULL;    
    Pool* pPoolSampler  = NULL;    

    RendererDesc mDesc = {};

    // Vulkan
    VkPhysicalDeviceProperties mVkDeviceProperties = {};
    VkDevice mVkDevice = VK_NULL_HANDLE;
    VmaAllocator mVkAllocator = VK_NULL_HANDLE;
};

void initRenderer(RendererDesc desc, Renderer* pRenderer);
