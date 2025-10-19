#include "texture.hpp"
#include "buffer.hpp"
#include "render.hpp"
#include "command_buffer.hpp"
#include "../core/debug.hpp"
#include "vma/vk_mem_alloc.h"
#include "vulkan/vulkan_core.h"

VkImageType getVkImageType(TextureType type)
{
    switch(type)
    {
        case TEXTURE_TYPE_1D: 
            return VK_IMAGE_TYPE_1D;
        case TEXTURE_TYPE_2D: 
        case TEXTURE_TYPE_CUBEMAP:
            return VK_IMAGE_TYPE_2D;
        case TEXTURE_TYPE_3D: 
            return VK_IMAGE_TYPE_3D;
        default: return (VkImageType)0;
    }
}

VkImageViewType getVkImageViewType(TextureType type)
{
    switch(type)
    {
        case TEXTURE_TYPE_1D: 
            return VK_IMAGE_VIEW_TYPE_1D;
        case TEXTURE_TYPE_2D: 
            return VK_IMAGE_VIEW_TYPE_2D;
        case TEXTURE_TYPE_CUBEMAP:
            return VK_IMAGE_VIEW_TYPE_CUBE;
        case TEXTURE_TYPE_3D: 
            return VK_IMAGE_VIEW_TYPE_3D;
        default: return (VkImageViewType)0;
    }
}

void addTexture(Renderer* pRenderer, TextureDesc desc, Texture** ppTexture)
{
    ASSERT(pRenderer && ppTexture);
    ASSERT(*ppTexture == NULL);

    *ppTexture = (Texture*)poolAlloc(pRenderer->pPoolTexture);

    **ppTexture = {};

    VkImageCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.usage = desc.mUsage;
    info.format = (VkFormat)desc.mFormat;
    info.initialLayout = (VkImageLayout)desc.mLayout;
    info.imageType = getVkImageType(desc.mType);
    info.extent.width = desc.mWidth;
    info.extent.height = desc.mHeight;
    info.extent.depth = desc.mDepth;
    info.samples = (VkSampleCountFlagBits)desc.mSamples;
    info.arrayLayers = 1;   // TODO_DW: TEXTURE_ARRAY
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.flags = 0;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    VkImage vkImage;
    VmaAllocation vkAlloc;
    VkResult ret = vmaCreateImage(
            pRenderer->mVkAllocator,
            &info,
            &allocInfo,
            &vkImage,
            &vkAlloc,
            NULL);
    ASSERTVK(ret);

    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = vkImage;
    viewInfo.viewType = getVkImageViewType(desc.mType);
    viewInfo.format = (VkFormat)desc.mFormat;
    viewInfo.subresourceRange.aspectMask =
        desc.mUsage && TEXTURE_USAGE_DEPTH_TARGET
        ? VK_IMAGE_ASPECT_DEPTH_BIT
        : VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = desc.mMipCount;
    viewInfo.subresourceRange.baseArrayLayer = 0;   // TODO_DW: TEXTURE_ARRAY
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView vkImageView;
    ret = vkCreateImageView(
            pRenderer->mVkDevice,
            &viewInfo,
            NULL,
            &vkImageView);
    ASSERTVK(ret);

    (*ppTexture)->mDesc = desc;
    (*ppTexture)->mVkImage = vkImage;
    (*ppTexture)->mVkImageView = vkImageView;
    (*ppTexture)->mVkAllocation = vkAlloc;
}

void removeTexture(Renderer* pRenderer, Texture** ppTexture)
{
    ASSERT(pRenderer && ppTexture);
    ASSERT(*ppTexture);

    vkDestroyImageView(
            pRenderer->mVkDevice, 
            (*ppTexture)->mVkImageView, 
            NULL);
    vmaDestroyImage(
            pRenderer->mVkAllocator, 
            (*ppTexture)->mVkImage, 
            (*ppTexture)->mVkAllocation);
    
    **ppTexture = {};

    poolFree(pRenderer->pPoolTexture, *ppTexture);
    *ppTexture = NULL;
}
