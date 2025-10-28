#include "asset.hpp"
#include "../core/file.hpp"
#include "src/render/buffer.hpp"
#undef STB_IMAGE_IMPLEMENTATION
#include "../third_party/stb_image.h"
#include "../core/debug.hpp"
#include "../render/texture.hpp"
#include "../render/render.hpp"

void loadTexture(AssetManager* pAssetManager, Renderer* pRenderer, String path, 
        bool flipVertical, Texture** ppOut)
{
    ASSERT(pAssetManager && pRenderer && ppOut);
    ASSERT(*ppOut == NULL);
    uint64 fileSize = 0;
    byte* fileData = readFile(&pAssetManager->mArenaTemp, path, &fileSize);

    stbi_set_flip_vertically_on_load(flipVertical);
    int32 width, height, channels;
    byte* imageData = stbi_load_from_memory(
            fileData,
            fileSize,
            &width, &height, &channels, STBI_rgb_alpha);
    uint64 imageSize = width * height * STBI_rgb_alpha;

    TextureDesc desc = {};
    desc.mWidth = width;
    desc.mHeight = height;
    desc.mDepth = 1;
    desc.mSamples = 1;
    desc.mMipCount = getMaxMipCount(width, height);
    desc.mType = TEXTURE_TYPE_2D;
    desc.mFormat = FORMAT_RGBA8_SRGB;
    desc.mBaseLayout = IMAGE_LAYOUT_UNDEFINED;
    desc.mUsage = TEXTURE_USAGE_SAMPLED
        | TEXTURE_USAGE_TRANSFER_SRC
        | TEXTURE_USAGE_TRANSFER_DST;

    addTexture(pRenderer, desc, ppOut);

    CommandBuffer* pCmd = getCmd(pRenderer, true);
    beginCmd(pCmd);
    // Transition texture to TRANSFER_DST
    TextureBarrier barrier = { *ppOut, IMAGE_LAYOUT_UNDEFINED, IMAGE_LAYOUT_TRANSFER_DST };
    cmdTextureBarrier(pCmd, 1, &barrier);

    // Copy image data to texture
    copyToBuffer(pRenderer, pRenderer->pStagingBuffer, 0, imageData, imageSize);
    cmdCopyToTexture(pCmd, *ppOut, pRenderer->pStagingBuffer);
    cmdGenerateMipmap(pCmd, *ppOut, SAMPLER_FILTER_LINEAR);

    // Transition texture to SHADER_READ_ONLY
    barrier = { *ppOut, IMAGE_LAYOUT_TRANSFER_SRC, IMAGE_LAYOUT_SHADER_READ_ONLY };
    cmdTextureBarrier(pCmd, 1, &barrier);
    endCmd(pCmd);
    submitImmediateCmd(pRenderer, pCmd);

    arenaClear(&pAssetManager->mArenaTemp);
}
