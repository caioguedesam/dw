#include "../generated/build_includes.hpp"
#include "src/asset/asset.hpp"
#include "src/render/command_buffer.hpp"
#include "src/render/render.hpp"
#include "src/render/shader.hpp"

App gApp;
AssetManager gAssetManager;
Renderer gRenderer;

RenderTarget*       pFullscreenRT = NULL;
Shader*             pFullscreenVert = NULL;
Shader*             pFullscreenFrag = NULL;
GraphicsPipeline*   pFullscreenPipeline = NULL;

uint32 gFrame = 0;

#define APP_WIDTH 800
#define APP_HEIGHT 600

void addRenderTargets()
{
    // Fullscreen RT
    {
        RenderTargetDesc desc = {};
        desc.mFormat = FORMAT_RGBA8_UNORM;
        desc.mClear = {{0,0,0,0}};
        desc.mWidth = gApp.mWindow.mWidth;
        desc.mHeight = gApp.mWindow.mHeight;
        addRenderTarget(&gRenderer, desc, &pFullscreenRT);
    }
}

void removeRenderTargets()
{
    removeRenderTarget(&gRenderer, &pFullscreenRT);
}

void addShaders()
{
    loadShader(&gAssetManager, &gRenderer, 
            str("../../res/shaders/test.vert"), 
            &pFullscreenVert);
    loadShader(&gAssetManager, &gRenderer, 
            str("../../res/shaders/test.frag"), 
            &pFullscreenFrag);
}

void removeShaders()
{
    removeShader(&gRenderer, &pFullscreenFrag);
    removeShader(&gRenderer, &pFullscreenVert);
}

void addPipelines()
{
    // Fullscreen pipeline
    {
        GraphicsPipelineDesc desc = {};
        desc.mRenderTargetCount = 1;
        desc.mRenderTargetFormats[0] = pFullscreenRT->mDesc.mFormat;

        desc.pVS = pFullscreenVert;
        desc.pFS = pFullscreenFrag;

        desc.mCullMode = CULL_MODE_NONE;
        desc.mFrontFace = FRONT_FACE_CW;
        addPipeline(&gRenderer, desc, &pFullscreenPipeline);
    }
}

void removePipelines()
{
    removePipeline(&gRenderer, &pFullscreenPipeline);
}

void init()
{
    initApp(APP_WIDTH, APP_HEIGHT, "DW App", &gApp);
    AssetManagerDesc assetManagerDesc;
    assetManagerDesc.mPermanentArenaSize = GB(1);
    assetManagerDesc.mTempArenaSize = MB(128);
    initAssetManager(assetManagerDesc, &gAssetManager);

    RendererDesc rendererDesc = {};
    rendererDesc.pApp = &gApp;
    initRenderer(rendererDesc, &gRenderer);

    addRenderTargets();
    addShaders();
    addPipelines();
}

void shutdown()
{
    waitForCommands(&gRenderer);

    removePipelines();
    removeShaders();
    removeRenderTargets();

    destroyRenderer(&gRenderer);
    destroyAssetManager(&gAssetManager);
    destroyApp(&gApp);
}

void render()
{
    acquireNextImage(&gRenderer, gFrame);

    CommandBuffer* pCmd = getCmd(&gRenderer);
    beginCmd(pCmd);

    // Fulscreen pass
    {
        RenderTargetBarrier barrier = {pFullscreenRT, getImageLayout(pFullscreenRT), IMAGE_LAYOUT_COLOR_OUTPUT };
        cmdRenderTargetBarrier(pCmd, 1, &barrier);

        RenderTargetBindDesc bindDesc = {};
        bindDesc.mColorCount = 1;
        bindDesc.mColorBindings[0] = { pFullscreenRT, LOAD_OP_LOAD, STORE_OP_STORE };
        cmdBindRenderTargets(pCmd, bindDesc);

        cmdBindGraphicsPipeline(pCmd, pFullscreenPipeline);

        cmdSetViewport(pCmd, pFullscreenRT);
        cmdSetScissor(pCmd, pFullscreenRT);

        cmdDraw(pCmd, 3, 1);

        cmdUnbindRenderTargets(pCmd);
    }

    // Copy output to swap chain
    {
        RenderTargetBarrier barrier = {pFullscreenRT, IMAGE_LAYOUT_COLOR_OUTPUT, IMAGE_LAYOUT_TRANSFER_SRC };
        cmdRenderTargetBarrier(pCmd, 1, &barrier);
        cmdCopyToSwapChain(pCmd, &gRenderer.mSwapChain, pFullscreenRT->pTexture);
    }

    endCmd(pCmd);
    submitFrameCmd(&gRenderer, pCmd);
    present(&gRenderer);
    gFrame++;
}

void processLoadRequests(App* pApp)
{
    if(!pApp->mLoadRequests)
    {
        return;
    }

    waitForCommands(&gRenderer);
    if(pApp->mLoadRequests & LOAD_REQUEST_RESIZE)
    {
        removePipelines();
        removeRenderTargets();
        destroySwapChain(&gRenderer, &gRenderer.mSwapChain);

        initSwapChain(&gRenderer, &gRenderer.mSwapChain);
        addRenderTargets();
        addPipelines();
    }
    // TODO_DW: CONTINUE
    // Render targets and pipelines need to be recreated on resize
    // So RTs match app window (instead of being fixed size)

    pApp->mLoadRequests = 0;
}

DW_MAIN()
{
    BEGIN_MAIN;

    init();

    while(true)
    {
        poll(&gApp);
        if(!gApp.mRunning)
        {
            break;
        }
        processLoadRequests(&gApp);

        // process load requests
        
        render();
    }

    shutdown();

    END_MAIN;
}

