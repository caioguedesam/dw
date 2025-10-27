#include "../generated/build_includes.hpp"
#include "src/asset/asset.hpp"
#include "src/core/base.hpp"
#include "src/core/input.hpp"
#include "src/core/app.hpp"
#include "src/math/math.hpp"
#include "src/render/buffer.hpp"
#include "src/render/command_buffer.hpp"
#include "src/render/descriptor.hpp"
#include "src/render/render.hpp"
#include "src/render/shader.hpp"
#include "src/render/camera.hpp"

float cubeVertices[] = {
    // Front (+Z)
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // bottom-left
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f, // bottom-right
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f, // top-right
    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, // top-left

    // Back (-Z)
     0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  0.0f, 1.0f,

    // Left (-X)
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,

    // Right (+X)
     0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f, 1.0f,

    // Top (+Y)
    -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,

    // Bottom (-Y)
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 1.0f
};

uint16 cubeIndices[] = {
    // Front
    0, 1, 2,  0, 2, 3,
    // Back
    4, 5, 6,  4, 6, 7,
    // Left
    8, 9,10,  8,10,11,
    // Right
    12,13,14, 12,14,15,
     // Top
    16,17,18, 16,18,19,
     // Bottom
    20,21,22, 20,22,23
};

App gApp;
AssetManager gAssetManager;
Renderer gRenderer;

VertexLayout gCubeVertexLayout = {};
Buffer* pVBCube = NULL;
Buffer* pIBCube = NULL;

RenderTarget*       pTargetUnlit = NULL;
Shader*             pVSUnlit = NULL;
Shader*             pFSUnlit = NULL;
GraphicsPipeline*   pPipelineUnlit = NULL;
DescriptorSet*      pDescriptorSetPerFrame = NULL;

struct PerFrameUniforms
{
    m4f mWorld = {};
    m4f mView = {};
    m4f mProj = {};
};
PerFrameUniforms perFrameUniforms = {};
Buffer* pUBPerFrame = NULL;

Camera gCamera = {};
Timer gTimer = {};
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
        addRenderTarget(&gRenderer, desc, &pTargetUnlit);
    }
}

void removeRenderTargets()
{
    removeRenderTarget(&gRenderer, &pTargetUnlit);
}

void addShaders()
{
    loadShader(&gAssetManager, &gRenderer, 
            str("../../res/shaders/unlit.vert"), 
            &pVSUnlit);
    loadShader(&gAssetManager, &gRenderer, 
            str("../../res/shaders/unlit.frag"), 
            &pFSUnlit);
}

void removeShaders()
{
    removeShader(&gRenderer, &pFSUnlit);
    removeShader(&gRenderer, &pVSUnlit);
}

void addDescriptors()
{
    // Per frame
    {
        DescriptorSetDesc desc = {};
        desc.mCount = 1;
        desc.mResources[0] = { DESCRIPTOR_UNIFORM_BUFFER, pUBPerFrame, 1 };
        addDescriptorSet(&gRenderer, desc, &pDescriptorSetPerFrame);
    }
}

void removeDescriptors()
{
    removeDescriptorSet(&gRenderer, &pDescriptorSetPerFrame);
}

void addPipelines()
{
    // Fullscreen pipeline
    {
        GraphicsPipelineDesc desc = {};
        desc.mRenderTargetCount = 1;
        desc.mRenderTargetFormats[0] = pTargetUnlit->mDesc.mFormat;

        desc.mVertexLayout = gCubeVertexLayout;
        desc.pVS = pVSUnlit;
        desc.pFS = pFSUnlit;

        desc.mCullMode = CULL_MODE_NONE;
        desc.mFrontFace = FRONT_FACE_CW;

        desc.mDescriptorSetCount = 1;
        desc.pDescriptorSets[0] = pDescriptorSetPerFrame;

        addPipeline(&gRenderer, desc, &pPipelineUnlit);
    }
}

void removePipelines()
{
    removePipeline(&gRenderer, &pPipelineUnlit);
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

    // Cube vertex/index buffer
    {
        VertexLayoutDesc layoutDesc = {};
        layoutDesc.mCount = 2;
        layoutDesc.mAttribs[0] = ATTRIBUTE_FLOAT3;
        layoutDesc.mAttribs[1] = ATTRIBUTE_FLOAT2;
        initVertexLayout(layoutDesc, &gCubeVertexLayout);

        BufferDesc vbDesc = {};
        vbDesc.mType = BUFFER_TYPE_VERTEX;
        vbDesc.mSize = ARR_SIZE(cubeVertices);
        vbDesc.mCount = ARR_LEN(cubeVertices);
        vbDesc.mStride = sizeof(float);
        addBuffer(&gRenderer, vbDesc, &pVBCube, cubeVertices);

        BufferDesc ibDesc = {};
        ibDesc.mType = BUFFER_TYPE_INDEX;
        ibDesc.mSize = ARR_SIZE(cubeIndices);
        ibDesc.mCount = ARR_LEN(cubeIndices);
        ibDesc.mStride = sizeof(uint16);
        addBuffer(&gRenderer, ibDesc, &pIBCube, cubeIndices);
    }

    // Per frame uniform buffer
    {
        BufferDesc desc = {};
        desc.mType = BUFFER_TYPE_UNIFORM;
        desc.mSize = sizeof(PerFrameUniforms);
        desc.mCount = 1;
        desc.mStride = sizeof(PerFrameUniforms);
        addBuffer(&gRenderer, desc, &pUBPerFrame);
    }

    addRenderTargets();
    addShaders();
    addDescriptors();
    addPipelines();

    // App controls
    CameraDesc camDesc = {};
    float fovX = TO_RAD(90.f);
    float aspect = getAspectRatio(&gApp);
    camDesc.mFovY = fovHtoV(fovX, aspect);
    camDesc.mAspect = aspect;
    camDesc.mNear = 0.01f;
    camDesc.mFar = 1000.f;
    initCamera(
            {0,0,-5}, 
            {0,0,0}, 
            camDesc, 
            &gCamera);
}

void shutdown()
{
    waitForCommands(&gRenderer);

    removePipelines();
    removeDescriptors();
    removeShaders();
    removeRenderTargets();

    removeBuffer(&gRenderer, &pUBPerFrame);
    removeBuffer(&gRenderer, &pIBCube);
    removeBuffer(&gRenderer, &pVBCube);

    destroyRenderer(&gRenderer);
    destroyAssetManager(&gAssetManager);
    destroyApp(&gApp);
}

void updatePerFrameUniforms()
{
    perFrameUniforms.mWorld = identity();
    perFrameUniforms.mView = getView(&gCamera);
    perFrameUniforms.mProj = getProj(&gCamera);
}

void update()
{
    if(isJustDown(&gApp.mKeys, KEY_R))
    {
        addLoadRequest(&gApp, LOAD_REQUEST_SHADER);
    }

    // Camera
    {
        v3f moveDir = {0, 0, 0};
        if(isDown(&gApp.mKeys, KEY_W)) moveDir.z = -1;
        if(isDown(&gApp.mKeys, KEY_S)) moveDir.z = 1;
        if(isDown(&gApp.mKeys, KEY_A)) moveDir.x = -1;
        if(isDown(&gApp.mKeys, KEY_D)) moveDir.x = 1;
        moveCamera(&gCamera, moveDir, gApp.mDt);
        
        if(isDown(&gApp.mKeys, KEY_RMB))
        {
            v2f rotateDir;
            getDelta(&gApp.mCursor, &rotateDir.x, &rotateDir.y);
            rotateCamera(&gCamera, rotateDir, gApp.mDt);
        }
    }
    updateCamera(&gCamera, gApp.mDt);

    updatePerFrameUniforms();
}

void render()
{
    acquireNextImage(&gRenderer, gFrame);

    CommandBuffer* pCmd = getCmd(&gRenderer);
    beginCmd(pCmd);

    // Upload per frame data
    copyToBuffer(&gRenderer, pUBPerFrame, 0, &perFrameUniforms, sizeof(PerFrameUniforms));

    // Unlit pass
    {
        RenderTargetBarrier barrier = {pTargetUnlit, getImageLayout(pTargetUnlit), IMAGE_LAYOUT_COLOR_OUTPUT };
        cmdRenderTargetBarrier(pCmd, 1, &barrier);

        RenderTargetBindDesc bindDesc = {};
        bindDesc.mColorCount = 1;
        bindDesc.mColorBindings[0] = { pTargetUnlit, LOAD_OP_CLEAR, STORE_OP_STORE };
        cmdBindRenderTargets(pCmd, bindDesc);

        cmdBindDescriptorSet(pCmd, pPipelineUnlit, pDescriptorSetPerFrame, 0);
        cmdBindGraphicsPipeline(pCmd, pPipelineUnlit);

        cmdSetViewport(pCmd, pTargetUnlit);
        cmdSetScissor(pCmd, pTargetUnlit);

        cmdBindVertexBuffer(pCmd, pVBCube);
        cmdBindIndexBuffer(pCmd, pIBCube);

        //cmdDraw(pCmd, 3, 1);
        cmdDrawIndexed(pCmd, ARR_LEN(cubeIndices), 1);

        cmdUnbindRenderTargets(pCmd);
    }

    // Copy output to swap chain
    {
        RenderTargetBarrier barrier = {pTargetUnlit, IMAGE_LAYOUT_COLOR_OUTPUT, IMAGE_LAYOUT_TRANSFER_SRC };
        cmdRenderTargetBarrier(pCmd, 1, &barrier);
        cmdCopyToSwapChain(pCmd, &gRenderer.mSwapChain, pTargetUnlit->pTexture);
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
        removeDescriptors();
        removeRenderTargets();
        destroySwapChain(&gRenderer, &gRenderer.mSwapChain);

        initSwapChain(&gRenderer, &gRenderer.mSwapChain);
        addRenderTargets();
        addDescriptors();
        addPipelines();

        gCamera.mDesc.mAspect = getAspectRatio(&gApp);

        removeLoadRequest(&gApp, LOAD_REQUEST_RESIZE);
    }
    if(pApp->mLoadRequests & LOAD_REQUEST_SHADER)
    {
        LOG("Reloading shaders...");
        removePipelines();
        removeDescriptors();
        removeShaders();

        addShaders();
        addDescriptors();
        addPipelines();
        removeLoadRequest(&gApp, LOAD_REQUEST_SHADER);
    }
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

        update();        
        render();
    }

    shutdown();

    END_MAIN;
}

