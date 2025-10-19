#include "asset.hpp"
#include "../core/debug.hpp"

// stb_image (needs to be defined here due to memory alloc redefinitions)
Arena* pArenaImage;

#define STB_IMAGE_IMPLEMENTATION
#define STBI_MALLOC(sz) arenaPush(pArenaImage, sz);
#define STBI_REALLOC(p, newsz) arenaPushCopy(pArenaImage, newsz, p, newsz);
#define STBI_FREE(p)
#define STBI_ASSERT(x) ASSERT(x)
#include "../third_party/stb_image.h"

void initAssetManager(AssetManagerDesc desc, AssetManager* pAssetManager)
{
    ASSERT(pAssetManager);

    pAssetManager->mDesc = desc;
    initArena(desc.mPermanentArenaSize, &pAssetManager->mArenaPermanent);
    initArena(desc.mTempArenaSize, &pAssetManager->mArenaTemp);

    pArenaImage = &pAssetManager->mArenaPermanent;
}

void destroyAssetManager(AssetManager* pAssetManager)
{
    ASSERT(pAssetManager);

    destroyArena(&pAssetManager->mArenaPermanent);
    destroyArena(&pAssetManager->mArenaTemp);

    *pAssetManager = {};

    pArenaImage = NULL;
}
