#include "texture.hpp"
#include "buffer.hpp"
#include "render.hpp"
#include "command_buffer.hpp"
#include "../core/debug.hpp"
#include "../math/math.hpp"
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

    *ppTexture = (Texture*)poolAlloc(&pRenderer->poolTextures);

    **ppTexture = {};

    VkImageCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.usage = desc.mUsage;
    info.format = (VkFormat)desc.mFormat;
    info.initialLayout = (VkImageLayout)desc.mBaseLayout;
    info.imageType = getVkImageType(desc.mType);
    info.extent.width = desc.mWidth;
    info.extent.height = desc.mHeight;
    info.extent.depth = desc.mDepth;
    info.samples = (VkSampleCountFlagBits)desc.mSamples;
    info.mipLevels = desc.mMipCount;
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
        desc.mUsage & TEXTURE_USAGE_DEPTH_TARGET
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

    // TODO_DW: Generate texture mipmap based on params
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

    poolFree(&pRenderer->poolTextures, *ppTexture);
    *ppTexture = NULL;
}

void addSampler(Renderer* pRenderer, SamplerDesc desc, Sampler** ppSampler)
{
    ASSERT(pRenderer && ppSampler);
    ASSERT(*ppSampler == NULL);

    *ppSampler = (Sampler*)poolAlloc(&pRenderer->poolSamplers);

    **ppSampler = {};

    VkSamplerCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.minFilter = (VkFilter)desc.mMinFilter;
    info.magFilter = (VkFilter)desc.mMagFilter;
    info.mipmapMode = (VkSamplerMipmapMode)desc.mMipFilter;
    info.addressModeU = (VkSamplerAddressMode)desc.mAddressU;
    info.addressModeV = (VkSamplerAddressMode)desc.mAddressV;
    info.addressModeW = (VkSamplerAddressMode)desc.mAddressW;
    info.borderColor = (VkBorderColor)desc.mBorderColor;
    info.anisotropyEnable = desc.mAniso;
    if(desc.mAniso)
    {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(pRenderer->mVkPhysicalDevice, &props);
        info.maxAnisotropy = props.limits.maxSamplerAnisotropy;
    }

    info.unnormalizedCoordinates = VK_FALSE;
    info.compareEnable = VK_FALSE;
    info.compareOp = VK_COMPARE_OP_ALWAYS;
    info.mipLodBias = 0.f;
    info.minLod = 0.f;
    info.maxLod = 1000.f;

    VkSampler vkSampler;
    VkResult ret = vkCreateSampler(
            pRenderer->mVkDevice,
            &info,
            NULL,
            &vkSampler);
    ASSERTVK(ret);

    (*ppSampler)->mDesc = desc;
    (*ppSampler)->vkSampler = vkSampler;
}

void removeSampler(Renderer* pRenderer, Sampler** ppSampler)
{
    ASSERT(pRenderer && ppSampler);
    ASSERT(*ppSampler);

    vkDestroySampler(pRenderer->mVkDevice, (*ppSampler)->vkSampler, NULL);

    **ppSampler = {};

    poolFree(&pRenderer->poolSamplers, *ppSampler);
    *ppSampler = NULL;
}

uint32 getMaxMipCount(uint32 w, uint32 h)
{
    return (uint32)(floorf(log2f(MAX(w, h))));
}

void cmdGenerateMipmap(CommandBuffer* pCmd, Texture* pTexture, SamplerFilter mipFilter)
{
    // Texture format must support linear blit (can be queried with VkPhysicalDeviceProperties)
    
    // Change all mips to TRANSFER_DST
    TextureBarrier barrier = {};
    barrier.pTexture = pTexture;
    barrier.mOldLayout = pTexture->mDesc.mBaseLayout;
    barrier.mNewLayout = IMAGE_LAYOUT_TRANSFER_DST;
    barrier.mStartMip = 0;
    barrier.mMipCount = pTexture->mDesc.mMipCount;
    cmdTextureBarrier(pCmd, 1, &barrier);

    // For each mip, blit from past level
    int32 mipWidth = pTexture->mDesc.mWidth;
    int32 mipHeight = pTexture->mDesc.mHeight;
    for(uint32 i = 1; i < pTexture->mDesc.mMipCount; i++)
    {
        // Transition past mip to TRANSFER_SRC
        barrier.mOldLayout = IMAGE_LAYOUT_TRANSFER_DST;
        barrier.mNewLayout = IMAGE_LAYOUT_TRANSFER_SRC;
        barrier.mStartMip = i - 1;
        barrier.mMipCount = 1;
        cmdTextureBarrier(pCmd, 1, &barrier);

        // Blit
        VkImageBlit blitRegion = {};
        blitRegion.srcOffsets[0] = {0, 0, 0};
        blitRegion.srcOffsets[1] = {mipWidth, mipHeight, 1};
        blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blitRegion.srcSubresource.mipLevel = i - 1;
        blitRegion.srcSubresource.baseArrayLayer = 0;
        blitRegion.srcSubresource.layerCount = 1;
        blitRegion.dstOffsets[0] = {0, 0, 0};
        blitRegion.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
        blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blitRegion.dstSubresource.mipLevel = i;
        blitRegion.dstSubresource.baseArrayLayer = 0;
        blitRegion.dstSubresource.layerCount = 1;
        vkCmdBlitImage(pCmd->mVkCmd,
                pTexture->mVkImage,
                (VkImageLayout)IMAGE_LAYOUT_TRANSFER_SRC,
                pTexture->mVkImage,
                (VkImageLayout)IMAGE_LAYOUT_TRANSFER_DST,
                1, &blitRegion,
                (VkFilter)mipFilter);

        if(mipWidth > 1) mipWidth /= 2;
        if(mipHeight > 1) mipHeight /= 2;
    }

    // All mips should finish in TRANSFER_SRC layout.
    barrier.mOldLayout = IMAGE_LAYOUT_TRANSFER_DST;
    barrier.mNewLayout = IMAGE_LAYOUT_TRANSFER_SRC;
    barrier.mStartMip = pTexture->mDesc.mMipCount - 1;
    barrier.mMipCount = 1;
    cmdTextureBarrier(pCmd, 1, &barrier);
}

void cmdCopyToTexture(CommandBuffer* pCmd, Texture* pDst, Buffer* pSrc)
{
    ASSERT(pCmd && pDst && pSrc);

    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;   // TODO_DW: Is there a case where I want to copy buffer contents to mips?
    region.imageSubresource.baseArrayLayer = 0;     // TODO_DW: Texture arrays
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent =
    {
        pDst->mDesc.mWidth,
        pDst->mDesc.mHeight,
        pDst->mDesc.mDepth
    };

    vkCmdCopyBufferToImage(pCmd->mVkCmd, 
            pSrc->mVkBuffer,
            pDst->mVkImage, 
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
            1, &region);
}
