#pragma once
#define VK_USE_PLATFORM_WIN32_KHR
#include "../core/app.hpp"
#include "../core/memory.hpp"
#include "texture.hpp"
#include "shader.hpp"
#include "descriptor.hpp"
#include "command_buffer.hpp"
#include "vulkan/vulkan_core.h"
#include "vma/vk_mem_alloc.h"

#define ASSERTVK(EXPR) ASSERT((EXPR) == VK_SUCCESS)

struct RendererDesc;

// --------------------------------------
// Barriers

enum PipelineStage
{
    PIPELINE_STAGE_TOP                  = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
    PIPELINE_STAGE_VERTEX_INPUT         = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
    PIPELINE_STAGE_VERTEX_SHADER        = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
    PIPELINE_STAGE_FRAGMENT_SHADER      = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
    PIPELINE_STAGE_COMPUTE_SHADER       = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    PIPELINE_STAGE_COLOR_OUTPUT         = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    PIPELINE_STAGE_TRANSFER             = VK_PIPELINE_STAGE_TRANSFER_BIT,
    PIPELINE_STAGE_BOTTOM               = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
    PIPELINE_STAGE_ALL                  = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
};

enum MemoryAccess : uint32
{
    MEMORY_ACCESS_NONE                  = VK_ACCESS_NONE,
    MEMORY_ACCESS_SHADER_READ           = VK_ACCESS_SHADER_READ_BIT,
    MEMORY_ACCESS_SHADER_WRITE          = VK_ACCESS_SHADER_WRITE_BIT,
    MEMORY_ACCESS_TRANSFER_READ         = VK_ACCESS_TRANSFER_READ_BIT,
    MEMORY_ACCESS_TRANSFER_WRITE        = VK_ACCESS_TRANSFER_WRITE_BIT,
    MEMORY_ACCESS_COLOR_OUTPUT_READ     = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
    MEMORY_ACCESS_COLOR_OUTPUT_WRITE    = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    MEMORY_ACCESS_ALL_READS             = VK_ACCESS_MEMORY_READ_BIT,
    MEMORY_ACCESS_ALL_WRITES            = VK_ACCESS_MEMORY_WRITE_BIT,
};

struct Barrier
{
    PipelineStage   mSrcStage   = PIPELINE_STAGE_ALL;
    PipelineStage   mDstStage   = PIPELINE_STAGE_ALL;
    MemoryAccess    mSrcAccess  = MEMORY_ACCESS_ALL_WRITES;
    MemoryAccess    mDstAccess  = MEMORY_ACCESS_ALL_READS;
};

struct TextureBarrier
{
    Texture*        pTexture    = NULL;
    ImageLayout     mOldLayout  = IMAGE_LAYOUT_UNDEFINED;
    ImageLayout     mNewLayout  = IMAGE_LAYOUT_UNDEFINED;
    uint32          mStartMip   = 0;
    uint32          mMipCount   = 0;

    PipelineStage   mSrcStage   = PIPELINE_STAGE_ALL;
    PipelineStage   mDstStage   = PIPELINE_STAGE_ALL;
    MemoryAccess    mSrcAccess  = MEMORY_ACCESS_ALL_WRITES;
    MemoryAccess    mDstAccess  = MEMORY_ACCESS_ALL_READS;
};

// --------------------------------------
// Render/Depth Target
enum LoadOp
{
    LOAD_OP_DONT_CARE   = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    LOAD_OP_LOAD        = VK_ATTACHMENT_LOAD_OP_LOAD,
    LOAD_OP_CLEAR       = VK_ATTACHMENT_LOAD_OP_CLEAR, 
};

enum StoreOp
{
    STORE_OP_DONT_CARE  = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    STORE_OP_STORE      = VK_ATTACHMENT_STORE_OP_STORE,
};

struct ClearValue
{
    float mColor[4] = {0,0,0,0};
    float mDepth = 0;
};

struct RenderTargetDesc
{
    ImageFormat mFormat = FORMAT_UNDEFINED;
    ClearValue mClear = {};

    uint32 mWidth   = 0;
    uint32 mHeight  = 0;
    uint32 mSamples = 1;
};

struct RenderTarget
{
    Texture* pTexture = NULL;
    RenderTargetDesc mDesc = {};
};

void addRenderTarget(Renderer* pRenderer, RendererDesc desc, ClearValue clear, RenderTarget** ppTarget);
void addDepthTarget(Renderer* pRenderer, RendererDesc desc, ClearValue clear, RenderTarget** ppTarget);
void removeRenderTarget(Renderer* pRenderer, RenderTarget** ppTarget);

struct RenderTargetBinding
{
    RenderTarget* pTarget   = NULL;
    LoadOp mLoadOp          = LOAD_OP_LOAD;
    StoreOp mStoreOp        = STORE_OP_STORE;
};

#define MAX_PIPELINE_RENDER_TARGETS 8
struct RenderTargetBindDesc
{
    uint32 mColorCount = 0;
    RenderTargetBinding mColorBindings[MAX_PIPELINE_RENDER_TARGETS];
    RenderTargetBinding mDepthBinding;
};

// --------------------------------------
// Swap chain
#define MAX_SWAPCHAIN_IMAGES 8
struct SwapChain
{
    VkSwapchainKHR mVkSwapChain = VK_NULL_HANDLE;

    // Swap chain properties
    VkFormat mVkFormat = VK_FORMAT_UNDEFINED;
    VkColorSpaceKHR mVkColorSpace;
    VkPresentModeKHR mVkPresentMode;
    VkExtent2D mVkExtents = {};

    // Swap chain images
    VkImage mVkImages[MAX_SWAPCHAIN_IMAGES];
    VkImageView mVkImageViews[MAX_SWAPCHAIN_IMAGES];
    VkImageLayout mVkImageLayouts[MAX_SWAPCHAIN_IMAGES];

    uint32 mImageCount  = 0;
    uint32 mActiveImage = 0;
};

void initSwapChain(Renderer* pRenderer, SwapChain* pSwapChain);
void destroySwapChain(Renderer* pRenderer, SwapChain* pSwapChain);

// --------------------------------------
// Vertex Layout
enum VertexAttrib
{
    ATTRIBUTE_FLOAT,
    ATTRIBUTE_FLOAT2,
    ATTRIBUTE_FLOAT3,
    ATTRIBUTE_FLOAT4,
    // TODO_DW: Compressed attributes (for normals and such)
};

#define MAX_VERTEX_ATTRIBUTES 8
struct VertexLayoutDesc
{
    VertexAttrib mAttribs[MAX_VERTEX_ATTRIBUTES];
    uint32 mCount = 0;
};

struct VertexLayout
{
    VertexLayoutDesc mDesc = {};

    VkVertexInputBindingDescription mVkBinding = {};
    VkVertexInputAttributeDescription mVkAttribs[MAX_VERTEX_ATTRIBUTES];
};

void initVertexLayout(VertexLayoutDesc desc, VertexLayout* pLayout);

// --------------------------------------
// Graphics/Compute Pipeline
enum PrimitiveType
{
    PRIMITIVE_TRIANGLE_LIST = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    // TODO_DW: Other primitives?
};

enum FillMode
{
    FILL_MODE_SOLID     = VK_POLYGON_MODE_FILL,
    FILL_MODE_LINE      = VK_POLYGON_MODE_LINE,
    FILL_MODE_POINT     = VK_POLYGON_MODE_POINT
};

enum CullMode
{
    CULL_MODE_NONE      = VK_CULL_MODE_NONE,
    CULL_MODE_FRONT     = VK_CULL_MODE_FRONT_BIT,
    CULL_MODE_BACK      = VK_CULL_MODE_BACK_BIT,
    CULL_MODE_ALL       = VK_CULL_MODE_FRONT_AND_BACK,
};

enum FrontFace
{
    FRONT_FACE_CW       = VK_FRONT_FACE_CLOCKWISE,
    FRONT_FACE_CCW      = VK_FRONT_FACE_COUNTER_CLOCKWISE,
};

enum CompareOp
{
    COMPARE_NEVER   = VK_COMPARE_OP_NEVER,
    COMPARE_LESS    = VK_COMPARE_OP_LESS,
    COMPARE_LEQUAL  = VK_COMPARE_OP_LESS_OR_EQUAL,
    COMPARE_EQUAL   = VK_COMPARE_OP_EQUAL,
    COMPARE_GEQUAL  = VK_COMPARE_OP_GREATER_OR_EQUAL,
    COMPARE_GREATER = VK_COMPARE_OP_GREATER,
    COMPARE_ALWAYS  = VK_COMPARE_OP_ALWAYS,
};

enum BlendFactor
{
    BLEND_FACTOR_ZERO                   = VK_BLEND_FACTOR_ZERO,
    BLEND_FACTOR_ONE                    = VK_BLEND_FACTOR_ONE,
    BLEND_FACTOR_SRC_COLOR              = VK_BLEND_FACTOR_SRC_COLOR,
    BLEND_FACTOR_ONE_MINUS_SRC_COLOR    = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
    BLEND_FACTOR_SRC_ALPHA              = VK_BLEND_FACTOR_SRC_ALPHA,
    BLEND_FACTOR_ONE_MINUS_SRC_ALPHA    = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
};

enum BlendOp
{
    BLEND_ADD           = VK_BLEND_OP_ADD,
    BLEND_SUBTRACT      = VK_BLEND_OP_SUBTRACT,
    BLEND_MIN           = VK_BLEND_OP_MIN,
    BLEND_MAX           = VK_BLEND_OP_MAX,
};

enum ColorComponent : uint32
{
    COMPONENT_R   = VK_COLOR_COMPONENT_R_BIT,
    COMPONENT_G   = VK_COLOR_COMPONENT_G_BIT,
    COMPONENT_B   = VK_COLOR_COMPONENT_B_BIT,
    COMPONENT_A   = VK_COLOR_COMPONENT_A_BIT,
};

#define MAX_PIPELINE_RESOURCE_SETS 16
struct GraphicsPipelineDesc
{
    // Attachments
    uint32 mRenderTargetCount = 0;
    ImageFormat mRenderTargetFormats[MAX_PIPELINE_RENDER_TARGETS];
    ImageFormat mDepthTargetFormat = FORMAT_UNDEFINED;

    // Shader resources
    DescriptorSet* pDescriptorSets[MAX_PIPELINE_RESOURCE_SETS];
    uint32 mDescriptorSetCount = 0;

    // Programmable stages
    Shader* pVS = NULL;
    Shader* pFS = NULL;

    // Fixed stage
    VertexLayout mVertexLayout  = {};
    PrimitiveType mPrimitive    = PRIMITIVE_TRIANGLE_LIST;
    FillMode mFillMode          = FILL_MODE_SOLID;
    CullMode mCullMode          = CULL_MODE_NONE;
    FrontFace mFrontFace        = FRONT_FACE_CCW;
    float mLineWidth            = 1.f;

    // Depth state
    bool mDepthTest = false;
    bool mDepthWrite = false;
    CompareOp mDepthOp = COMPARE_GREATER;   // Default is reverse depth (near 1, far 0)
    // TODO_DW: Stencil

    // Blend state
    bool mBlendEnable           = false;
    BlendFactor mSrcColorFactor = BLEND_FACTOR_ZERO;
    BlendFactor mDstColorFactor = BLEND_FACTOR_ZERO;
    BlendFactor mSrcAlphaFactor = BLEND_FACTOR_ZERO;
    BlendFactor mDstAlphaFactor = BLEND_FACTOR_ZERO;
    BlendOp mBlendOp            = BLEND_ADD;
    uint32 mBlendMask           = 0;
};

struct GraphicsPipeline
{
    GraphicsPipelineDesc mDesc = {};

    VkPipeline mVkPipeline      = VK_NULL_HANDLE;
    VkPipelineLayout mVkLayout  = VK_NULL_HANDLE;
};

void addPipeline(Renderer* pRenderer, GraphicsPipelineDesc desc, GraphicsPipeline** ppPipeline);
void removePipeline(Renderer* pRenderer, GraphicsPipeline** ppPipeline);

struct ComputePipelineDesc
{
    // Shader resources
    DescriptorSet* pDescriptorSets[MAX_PIPELINE_RESOURCE_SETS];
    uint32 mDescriptorSetCount = 0;

    // Programmable stages
    Shader* pCS = NULL;
};

struct ComputePipeline
{
    ComputePipelineDesc mDesc = {};

    VkPipeline mVkPipeline      = VK_NULL_HANDLE;
    VkPipelineLayout mVkLayout  = VK_NULL_HANDLE;
};

void addPipeline(Renderer* pRenderer, ComputePipelineDesc desc, ComputePipeline** ppPipeline);
void removePipeline(Renderer* pRenderer, ComputePipeline** ppPipeline);

// --------------------------------------
// Renderer
struct RendererDesc
{
    App* pApp = NULL;

    uint64 mMaxBuffers              = 1024;
    uint64 mMaxTextures             = 1024;
    uint64 mMaxSamplers             = 64;
    uint64 mMaxShaders              = 256;
    uint64 mMaxDescriptorSets       = 64;
    uint64 mMaxRenderTargets        = 64;
    uint64 mMaxGraphicsPipelines    = 64;
    uint64 mMaxComputePipelines     = 64;
};

#define CONCURRENT_FRAMES 2
struct Renderer
{
    // Pools for reusable render data
    Pool poolBuffers            = {};    
    Pool poolTextures           = {};    
    Pool poolSamplers           = {};
    Pool poolShaders            = {};
    Pool poolDescriptorSets     = {};
    Pool poolRenderTargets      = {};
    Pool poolGraphicsPipelines  = {};
    Pool poolComputePipelines   = {};

    RendererDesc mDesc = {};

    SwapChain mSwapChain = {};
    CommandBuffer mCommandBuffers[MAX_COMMAND_BUFFERS];
    uint32 mActiveFrame = 0;

    // Vulkan
    VkInstance mVkInstance = VK_NULL_HANDLE;
#if DW_DEBUG
    VkDebugUtilsMessengerEXT mVkDebugMessenger = VK_NULL_HANDLE;
#endif
    VkSurfaceKHR mVkSurface = VK_NULL_HANDLE;
    VkPhysicalDevice mVkPhysicalDevice = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties mVkDeviceProperties = {};
    VkDevice mVkDevice = VK_NULL_HANDLE;
    VkQueue mVkQueue = VK_NULL_HANDLE;
    VmaAllocator mVkAllocator = VK_NULL_HANDLE;
    VkDescriptorPool mVkDescriptorPool = VK_NULL_HANDLE;
    VkCommandPool mVkCommandPool = VK_NULL_HANDLE;
    VkSemaphore mVkRenderSemaphores[CONCURRENT_FRAMES];
    VkSemaphore mVkPresentSemaphores[CONCURRENT_FRAMES];
    VkFence mVkFences[CONCURRENT_FRAMES];
    VkFence mVkImmediateFence = VK_NULL_HANDLE;
};

void initRenderer(RendererDesc desc, Renderer* pRenderer);
void destroyRenderer(Renderer* pRenderer);

void waitForCommands(Renderer* pRenderer);
void acquireNextImage(Renderer* pRenderer, uint32 frame);
void present(Renderer* pRenderer, uint32 frame);

// --------------------------------------
// Render Commands
void cmdBarrier(CommandBuffer* pCmd, Barrier barrier);
void cmdTextureBarrier(CommandBuffer* pCmd, TextureBarrier barrier);
void cmdSwapChainBarrier(CommandBuffer* pCmd, SwapChain* pSwapChain, ImageLayout newLayout);
void cmdClearRenderTarget(CommandBuffer* pCmd, RenderTarget* pTarget);
void cmdClearDepthTarget(CommandBuffer* pCmd, RenderTarget* pTarget);
void cmdBindRenderTargets(CommandBuffer* pCmd, RenderTargetBindDesc desc);
void cmdUnbindRenderTargets(CommandBuffer* pCmd);
void cmdBindGraphicsPipeline(CommandBuffer* pCmd, GraphicsPipeline* pPipeline);
void cmdBindComputePipeline(CommandBuffer* pCmd, ComputePipeline* pPipeline);
void cmdBindDescriptorSet(CommandBuffer* pCmd, GraphicsPipeline* pPipeline,
        DescriptorSet* pDescriptorSet, uint32 setBinding);
void cmdBindDescriptorSet(CommandBuffer* pCmd, ComputePipeline* pPipeline,
        DescriptorSet* pDescriptorSet, uint32 setBinding);
void cmdSetViewport(CommandBuffer* pCmd, float x, float y, float w, float h);
void cmdSetViewport(CommandBuffer* pCmd, RenderTarget* pTarget);
void cmdSetScissor(CommandBuffer* pCmd, int32 x, int32 y, uint32 w, uint32 h);
void cmdSetScissor(CommandBuffer* pCmd, RenderTarget* pTarget);
void cmdBindVertexBuffer(CommandBuffer* pCmd, Buffer* pBuffer);
void cmdBindIndexBuffer(CommandBuffer* pCmd, Buffer* pBuffer);
void cmdDraw(CommandBuffer* pCmd, uint32 vertexCount, uint32 instanceCount);
void cmdDrawIndexed(CommandBuffer* pCmd, uint32 indexCount, uint32 instanceCount);
void cmdDispatch(CommandBuffer* pCmd, uint32 x, uint32 y, uint32 z);
void cmdCopyToSwapChain(CommandBuffer* pCmd, SwapChain* pSwapChain, Texture* pSrc);
// TODO_DW: CONTINUE Implement these
