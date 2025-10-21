#pragma once
#define VK_USE_PLATFORM_WIN32_KHR
#include "../core/memory.hpp"
#include "texture.hpp"
#include "command_buffer.hpp"
#include "vulkan/vulkan_core.h"
#include "vma/vk_mem_alloc.h"

#define ASSERTVK(EXPR) ASSERT((EXPR) == VK_SUCCESS)

struct RendererDesc
{
    uint64 mMaxBuffers          = 1024;
    uint64 mMaxTextures         = 1024;
    uint64 mMaxSamplers         = 64;
    uint64 mMaxShaders          = 256;
    uint64 mMaxResourceSets     = 64;
    uint64 mMaxRenderTargets    = 64;
};

struct ClearValue
{
    float mColor[4] = {0,0,0,0};
    float mDepth = 1;
};

struct RenderTargetDesc
{
    ImageFormat mFormat = FORMAT_INVALID;
    ClearValue mClear = {};

    uint32 mWidth   = 0;
    uint32 mHeight  = 0;
    uint32 mSamples = 1;
};

struct RenderTarget
{
    Texture* pTexture = NULL;
    RenderTargetDesc mDesc = {};
};

#define MAX_SWAPCHAIN_IMAGES 8
struct SwapChain
{
    VkSwapchainKHR mVkSwapChain = VK_NULL_HANDLE;

    // Swap chain properties
    VkFormat mVkFormat = VK_FORMAT_UNDEFINED;
    VkColorSpaceKHR mVkColorSpace;
    VkPresentModeKHR mVkPresentMode;
    VkExtent2D mVkExtents = {};

    // Swap chain images
    VkImage mVkImages[MAX_SWAPCHAIN_IMAGES];
    VkImageView mVkImageViews[MAX_SWAPCHAIN_IMAGES];
    VkImageLayout mVkImageLayouts[MAX_SWAPCHAIN_IMAGES];

    uint32 mImageCount  = 0;
    uint32 mActiveImage = 0;
};

#define CONCURRENT_FRAMES 2
struct Renderer
{
    // Pools for reusable render data
    Pool poolBuffers        = {};    
    Pool poolTextures       = {};    
    Pool poolSamplers       = {};
    Pool poolShaders        = {};
    Pool poolResourceSets   = {};
    Pool poolRenderTargets  = {};

    RendererDesc mDesc = {};

    CommandBuffer mCommandBuffers[MAX_COMMAND_BUFFERS];
    uint32 mCommandBufferCount = 0;

    uint32 mActiveFrame = 0;

    // Vulkan
    VkInstance mVkInstance = VK_NULL_HANDLE;
    VkSurfaceKHR mVkSurface = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties mVkDeviceProperties = {};
    VkDevice mVkDevice = VK_NULL_HANDLE;
    VkPhysicalDevice mVkPhysicalDevice = VK_NULL_HANDLE;
    VmaAllocator mVkAllocator = VK_NULL_HANDLE;
    VkQueue mVkQueue = VK_NULL_HANDLE;
    VkCommandPool mVkCommandPool = VK_NULL_HANDLE;
    VkDescriptorPool mVkDescriptorPool = VK_NULL_HANDLE;
    VkFence mVkFences[CONCURRENT_FRAMES];
    VkFence mVkImmediateFence = VK_NULL_HANDLE;
};

void initSwapChain(Renderer* pRenderer, SwapChain* pSwapChain);
void destroySwapChain(Renderer* pRenderer, SwapChain* pSwapChain);

void addRenderTarget(Renderer* pRenderer, RendererDesc desc, ClearValue clear, RenderTarget** ppTarget);
void addDepthTarget(Renderer* pRenderer, RendererDesc desc, ClearValue clear, RenderTarget** ppTarget);
void removeRenderTarget(Renderer* pRenderer, RenderTarget** ppTarget);

void initRenderer(RendererDesc desc, Renderer* pRenderer);
void destroyRenderer(Renderer* pRenderer);

void waitForCommands(Renderer* pRenderer);

void beginFrame(Renderer* pRenderer);
void endFrame(Renderer* pRenderer);
