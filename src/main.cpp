#include "../generated/build_includes.hpp"
#include "src/asset/asset.hpp"

App gApp;
AssetManager gAssetManager;
Shader* pShader = NULL;
Texture* pTexture = NULL;
Renderer gRenderer;

DW_MAIN()
{
    BEGIN_MAIN;

    initApp(800, 600, "DW App", &gApp);
    AssetManagerDesc assetManagerDesc;
    assetManagerDesc.mPermanentArenaSize = GB(1);
    assetManagerDesc.mTempArenaSize = MB(128);
    initAssetManager(assetManagerDesc, &gAssetManager);

    testCore(&gApp);
    testMath();

    // Testing shader compilation
    //loadShader(&gAssetManager, &gRenderer, 
    //        str("../../res/shaders/test.vert"),
    //        &pShader);

    // Testing image loading
    //loadTexture(&gAssetManager, &gRenderer,
    //        str("../../res/textures/white.png"),
    //        false,
    //        &pTexture);

    while(gApp.mRunning)
    {
        poll(&gApp);
        //debugInput(&gApp);
        // update and render
    }

    destroyAssetManager(&gAssetManager);
    destroyApp(&gApp);

    END_MAIN;
}

