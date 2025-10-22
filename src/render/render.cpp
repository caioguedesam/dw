#include "render.hpp"
#include "../core/debug.hpp"
#include "src/render/texture.hpp"
#include "vulkan/vulkan_core.h"

void initSwapChain(Renderer* pRenderer, SwapChain* pSwapChain)
{
    ASSERT(pRenderer && pSwapChain);
    *pSwapChain = {};

    // Required format is BGRA8_SRGB, SRGB_NONLINEAR (Swap chain performs gamma correction, not shaders)
    // Fallback is first format found.
    uint32 formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(pRenderer->mVkPhysicalDevice, pRenderer->mVkSurface, &formatCount, NULL);
    ASSERT(formatCount);
    VkSurfaceFormatKHR formats[formatCount];
    vkGetPhysicalDeviceSurfaceFormatsKHR(pRenderer->mVkPhysicalDevice, pRenderer->mVkSurface, &formatCount, formats);

    VkSurfaceFormatKHR format = formats[0];
    for(uint32 i = 0; i < formatCount; i++)
    {
        if(formats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
            formats[i].colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
        {
            format = formats[i];
            break;
        }
    }

    // Required present mode is MAILBOX, fallback is first present mode found.
    uint32 presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(pRenderer->mVkPhysicalDevice, pRenderer->mVkSurface, &presentModeCount, NULL);
    ASSERT(presentModeCount);
    VkPresentModeKHR presentModes[presentModeCount];
    vkGetPhysicalDeviceSurfacePresentModesKHR(pRenderer->mVkPhysicalDevice, pRenderer->mVkSurface, &presentModeCount, presentModes);

    VkPresentModeKHR presentMode = presentModes[0];
    for(uint32 i = 0; i < formatCount; i++)
    {
        if(presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            presentMode = presentModes[i];
            break;
        }
    }

    // Image details
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pRenderer->mVkPhysicalDevice, pRenderer->mVkSurface, &capabilities);
    ASSERT(capabilities.currentExtent.width != -1);
    VkExtent2D extents = capabilities.currentExtent;
    uint32 imageCount = capabilities.minImageCount + 1; // Double buffering
    if(capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
    {
        imageCount = capabilities.maxImageCount;
    }

    // Swap chain creation
    // Creating swap chain object
    VkSwapchainCreateInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    info.surface = pRenderer->mVkSurface;
    info.minImageCount = imageCount;
    info.imageFormat = format.format;
    info.imageColorSpace = format.colorSpace;
    info.imageArrayLayers = 1;
    info.imageExtent = extents;
    info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.preTransform = capabilities.currentTransform;
    info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    info.presentMode = presentMode;
    info.clipped = VK_TRUE;
    info.oldSwapchain = VK_NULL_HANDLE;

    VkSwapchainKHR vkSwapChain;
    VkResult ret = vkCreateSwapchainKHR(pRenderer->mVkDevice, &info, NULL, &vkSwapChain);
    ASSERTVK(ret);

    pSwapChain->mVkSwapChain = vkSwapChain;
    pSwapChain->mVkFormat = format.format;
    pSwapChain->mVkColorSpace = format.colorSpace;
    pSwapChain->mVkPresentMode = presentMode;
    pSwapChain->mVkExtents = extents;
    pSwapChain->mImageCount = imageCount;

    // Swap chain image view creation
    ret = vkGetSwapchainImagesKHR(pRenderer->mVkDevice, vkSwapChain, &imageCount, NULL);
    ASSERTVK(ret);
    ASSERT(imageCount);
    VkImage vkImages[imageCount];
    ret = vkGetSwapchainImagesKHR(pRenderer->mVkDevice, vkSwapChain, &imageCount, vkImages);
    ASSERTVK(ret);

    VkImageView vkImageViews[imageCount];
    for(uint32 i = 0; i < imageCount; i++)
    {
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = vkImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format.format;
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        ret = vkCreateImageView(pRenderer->mVkDevice, &viewInfo, NULL, &vkImageViews[i]);
        ASSERTVK(ret);
        pSwapChain->mVkImages[i] = vkImages[i];
        pSwapChain->mVkImageViews[i] = vkImageViews[i];
        pSwapChain->mVkImageLayouts[i] = VK_IMAGE_LAYOUT_UNDEFINED;
    }
}

void destroySwapChain(Renderer* pRenderer, SwapChain* pSwapChain)
{
    ASSERT(pRenderer && pSwapChain);

    for(uint32 i = 0; i < pSwapChain->mImageCount; i++)
    {
        vkDestroyImageView(pRenderer->mVkDevice, pSwapChain->mVkImageViews[i], NULL);
    }
    vkDestroySwapchainKHR(pRenderer->mVkDevice, pSwapChain->mVkSwapChain, NULL);

    *pSwapChain = {};
}

void addRenderTarget(Renderer* pRenderer, RenderTargetDesc desc, ClearValue clear, RenderTarget** ppTarget)
{
    ASSERT(pRenderer && ppTarget);
    ASSERT(*ppTarget == NULL);

    *ppTarget = (RenderTarget*)poolAlloc(&pRenderer->poolRenderTargets);

    **ppTarget = {};

    TextureDesc textureDesc = {};
    textureDesc.mFormat = desc.mFormat;
    textureDesc.mLayout = IMAGE_LAYOUT_COLOR_OUTPUT;
    textureDesc.mType = TEXTURE_TYPE_2D;
    textureDesc.mUsage =
        TEXTURE_USAGE_COLOR_TARGET |
        TEXTURE_USAGE_TRANSFER_SRC |
        TEXTURE_USAGE_TRANSFER_DST;
    textureDesc.mWidth = desc.mWidth;
    textureDesc.mHeight = desc.mHeight;
    textureDesc.mMipCount = 1;
    textureDesc.mSamples = desc.mSamples;
    Texture* pTexture = NULL;
    addTexture(pRenderer, textureDesc, &pTexture);
    ASSERT(pTexture);

    (*ppTarget)->pTexture = pTexture;
    (*ppTarget)->mDesc = desc;
}

void addDepthTarget(Renderer* pRenderer, RenderTargetDesc desc, ClearValue clear, RenderTarget** ppTarget)
{
    ASSERT(pRenderer && ppTarget);
    ASSERT(*ppTarget == NULL);

    *ppTarget = (RenderTarget*)poolAlloc(&pRenderer->poolRenderTargets);

    **ppTarget = {};

    TextureDesc textureDesc = {};
    textureDesc.mFormat = desc.mFormat;
    textureDesc.mLayout = IMAGE_LAYOUT_COLOR_OUTPUT;
    textureDesc.mType = TEXTURE_TYPE_2D;
    textureDesc.mUsage =
        TEXTURE_USAGE_DEPTH_TARGET |
        TEXTURE_USAGE_TRANSFER_SRC |
        TEXTURE_USAGE_TRANSFER_DST;
    textureDesc.mWidth = desc.mWidth;
    textureDesc.mHeight = desc.mHeight;
    textureDesc.mMipCount = 1;
    textureDesc.mSamples = desc.mSamples;
    Texture* pTexture = NULL;
    addTexture(pRenderer, textureDesc, &pTexture);
    ASSERT(pTexture);

    (*ppTarget)->pTexture = pTexture;
    (*ppTarget)->mDesc = desc;
}

void removeRenderTarget(Renderer* pRenderer, RenderTarget** ppTarget)
{
    ASSERT(pRenderer && ppTarget);
    ASSERT(*ppTarget);

    removeTexture(pRenderer, &(*ppTarget)->pTexture); 
    ASSERT((*ppTarget)->pTexture == NULL);

    **ppTarget = {};

    poolFree(&pRenderer->poolRenderTargets, *ppTarget);
    *ppTarget = NULL;
}

void initVertexLayout(VertexLayoutDesc desc, VertexLayout* pLayout)
{
    ASSERT(pLayout);

    pLayout->mDesc = desc;
    pLayout->mVkBinding = {};
    pLayout->mVkBinding.binding = 0;
    pLayout->mVkBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    pLayout->mVkBinding.stride = 0;

    for(uint32 i = 0; i < desc.mCount; i++)
    {
        VertexAttrib attr = desc.mAttribs[i];

        pLayout->mVkAttribs[i] = {};
        pLayout->mVkAttribs[i].binding = 0;
        pLayout->mVkAttribs[i].location = i;
        pLayout->mVkAttribs[i].offset = pLayout->mVkBinding.stride;

        uint32 attrSize = 0;
        switch(attr)
        {
            case ATTRIBUTE_FLOAT:
            {
                pLayout->mVkAttribs[i].format = VK_FORMAT_R32_SFLOAT;
                attrSize = sizeof(float);
            } break;
            case ATTRIBUTE_FLOAT2:
            {
                pLayout->mVkAttribs[i].format = VK_FORMAT_R32G32_SFLOAT;
                attrSize = 2 * sizeof(float);
            } break;
            case ATTRIBUTE_FLOAT3:
            {
                pLayout->mVkAttribs[i].format = VK_FORMAT_R32G32B32_SFLOAT;
                attrSize = 3 * sizeof(float);
            } break;
            case ATTRIBUTE_FLOAT4:
            {
                pLayout->mVkAttribs[i].format = VK_FORMAT_R32G32B32A32_SFLOAT;
                attrSize = 4 * sizeof(float);
            } break;
            default: ASSERTF(0, "Unsupported vertex attribute format");
        }

        pLayout->mVkBinding.stride += attrSize;
    }
}

void initRenderer(RendererDesc desc, Renderer* pRenderer)
{
    *pRenderer = {};
    // Initializing reusable data pools
}
