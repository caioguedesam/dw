#include "asset.hpp"
#include "../core/file.hpp"
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
        | TEXTURE_USAGE_TRANSFER_DST;

    addTexture(pRenderer, desc, ppOut);
}
