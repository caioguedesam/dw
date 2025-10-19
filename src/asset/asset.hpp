#pragma once
#include "../core/base.hpp"
#include "../core/memory.hpp"
#include "../core/string.hpp"

struct Shader;
struct Texture;
struct Renderer;

struct AssetManagerDesc
{
    uint64 mPermanentArenaSize = 0;
    uint64 mTempArenaSize = 0;
    uint64 mSTBIArenaSize = 0;
};

struct AssetManager
{
    AssetManagerDesc mDesc = {};

    Arena mArenaPermanent   = {};
    Arena mArenaTemp        = {};
};

void initAssetManager(AssetManagerDesc desc, AssetManager* pAssetManager);
void destroyAssetManager(AssetManager* pAssetManager);

void loadShader(AssetManager* pAssetManager, Renderer* pRenderer,
        String path, Shader** ppOut);
void loadTexture(AssetManager* pAssetManager, Renderer* pRenderer,
        String path, bool flipVertical, Texture** ppOut);
