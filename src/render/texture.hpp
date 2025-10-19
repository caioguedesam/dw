#pragma once
#include "../core/base.hpp"
#include "vulkan/vulkan_core.h"
#include "vma/vk_mem_alloc.h"

struct Renderer;
struct CommandBuffer;
struct Buffer;

enum ImageFormat
{
    FORMAT_INVALID              = VK_FORMAT_UNDEFINED,
    FORMAT_R8_SRGB              = VK_FORMAT_R8_SRGB,
    FORMAT_RG8_SRGB             = VK_FORMAT_R8G8_SRGB,
    FORMAT_RGB8_SRGB            = VK_FORMAT_R8G8B8_SRGB,
    FORMAT_RGBA8_SRGB           = VK_FORMAT_R8G8B8A8_SRGB,
    FORMAT_RGBA8_UNORM          = VK_FORMAT_R8G8B8A8_UNORM,
    FORMAT_RGBA16_UNORM         = VK_FORMAT_R16G16B16A16_UNORM,
    FORMAT_BGRA8_SRGB           = VK_FORMAT_B8G8R8A8_SRGB,
    FORMAT_RG32_FLOAT           = VK_FORMAT_R32G32_SFLOAT,
    FORMAT_RGB32_FLOAT          = VK_FORMAT_R32G32B32_SFLOAT,
    FORMAT_RGBA32_FLOAT         = VK_FORMAT_R32G32B32A32_SFLOAT,
    FORMAT_RGBA16_FLOAT         = VK_FORMAT_R16G16B16A16_SFLOAT,
    FORMAT_D32_FLOAT            = VK_FORMAT_D32_SFLOAT,
    FORMAT_D16_UNORM            = VK_FORMAT_D16_UNORM,
};

enum ImageLayout
{
    IMAGE_LAYOUT_UNDEFINED                  = VK_IMAGE_LAYOUT_UNDEFINED,
    IMAGE_LAYOUT_GENERAL                    = VK_IMAGE_LAYOUT_GENERAL,
    IMAGE_LAYOUT_COLOR_OUTPUT               = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    IMAGE_LAYOUT_PRESENT_SRC                = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    IMAGE_LAYOUT_TRANSFER_SRC               = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    IMAGE_LAYOUT_TRANSFER_DST               = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    IMAGE_LAYOUT_SHADER_READ_ONLY           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    IMAGE_LAYOUT_DEPTH_STENCIL_OUTPUT       = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
};

enum TextureType
{
    TEXTURE_TYPE_1D,
    TEXTURE_TYPE_2D,
    TEXTURE_TYPE_3D,
    TEXTURE_TYPE_CUBEMAP,
    //TEXTURE_TYPE_2D_ARRAY,    // TODO_DW: TEXTURE_ARRAY
};

enum TextureUsage : uint32
{
    TEXTURE_USAGE_COLOR_TARGET = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    TEXTURE_USAGE_DEPTH_TARGET = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
    TEXTURE_USAGE_TRANSFER_SRC = VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
    TEXTURE_USAGE_TRANSFER_DST = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
    TEXTURE_USAGE_SAMPLED      = VK_IMAGE_USAGE_SAMPLED_BIT,
    TEXTURE_USAGE_STORAGE      = VK_IMAGE_USAGE_STORAGE_BIT,
    TEXTURE_USAGE_ANY          = VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM,
};

struct TextureDesc
{
    ImageFormat mFormat = FORMAT_INVALID;
    ImageLayout mLayout = IMAGE_LAYOUT_UNDEFINED;
    TextureType mType   = TEXTURE_TYPE_2D;
    uint32 mUsage       = TEXTURE_USAGE_ANY;

    uint32 mWidth       = 0;
    uint32 mHeight      = 0;
    uint32 mDepth       = 0;
    uint32 mSamples     = 1;
    uint32 mMipCount    = 1;
};

struct Texture
{
    TextureDesc mDesc = {};

    VkImage         mVkImage        = VK_NULL_HANDLE;
    VkImageView     mVkImageView    = VK_NULL_HANDLE;
    VmaAllocation   mVkAllocation   = VK_NULL_HANDLE;
};

void addTexture(Renderer* pRenderer, TextureDesc desc, Texture** ppTexture);
void removeTexture(Renderer* pRenderer, Texture** ppTexture);

uint32 getMaxMipCount(uint32 w, uint32 h);

// TODO_DW: After command buffers
void cmdBarrier(CommandBuffer* pCmd, Texture* pTexture, ImageLayout oldLayout, ImageLayout newLayout);
void cmdGenerateMipmap(CommandBuffer* pCmd, Texture* pTexture);
void cmdCopyToTexture(CommandBuffer* pCmd, Texture* pDst, Buffer* pSrc);
void cmdClearTexture(CommandBuffer* pCmd, Texture* pTexture, float r, float g, float b, float a);
