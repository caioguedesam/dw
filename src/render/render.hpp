#pragma once
#define VK_USE_PLATFORM_WIN32_KHR
#include "../core/memory.hpp"
#include "texture.hpp"
#include "shader.hpp"
#include "resource.hpp"
#include "command_buffer.hpp"
#include "vulkan/vulkan_core.h"
#include "vma/vk_mem_alloc.h"

#define ASSERTVK(EXPR) ASSERT((EXPR) == VK_SUCCESS)

struct RendererDesc;

// --------------------------------------
// Render/Depth Target
struct ClearValue
{
    float mColor[4] = {0,0,0,0};
    float mDepth = 1;
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

#define MAX_PIPELINE_RENDER_TARGETS 8
#define MAX_PIPELINE_RESOURCE_SETS 16
struct GraphicsPipelineDesc
{
    // Attachments
    uint32 mRenderTargetCount = 0;
    ImageFormat mRenderTargetFormats[MAX_PIPELINE_RENDER_TARGETS];
    ImageFormat mDepthTargetFormat = FORMAT_UNDEFINED;

    // Shader resources
    ShaderResourceSet* pResourceSets[MAX_PIPELINE_RESOURCE_SETS];
    uint32 mResourceSetCount = 0;

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
    ShaderResourceSet* pResourceSets[MAX_PIPELINE_RESOURCE_SETS];
    uint32 mResourceSetCount = 0;

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
    uint64 mMaxBuffers              = 1024;
    uint64 mMaxTextures             = 1024;
    uint64 mMaxSamplers             = 64;
    uint64 mMaxShaders              = 256;
    uint64 mMaxResourceSets         = 64;
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
    Pool poolResourceSets       = {};
    Pool poolRenderTargets      = {};
    Pool poolGraphicsPipelines  = {};
    Pool poolComputePipelines   = {};

    RendererDesc mDesc = {};

    CommandBuffer mCommandBuffers[MAX_COMMAND_BUFFERS];
    uint32 mCommandBufferCount = 0;

    uint32 mActiveFrame = 0;

    // Vulkan
    VkInstance mVkInstance = VK_NULL_HANDLE;
    VkSurfaceKHR mVkSurface = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties mVkDeviceProperties = {};
    VkDevice mVkDevice = VK_NULL_HANDLE;
    VkPhysicalDevice mVkPhysicalDevice = VK_NULL_HANDLE;
    VmaAllocator mVkAllocator = VK_NULL_HANDLE;
    VkQueue mVkQueue = VK_NULL_HANDLE;
    VkCommandPool mVkCommandPool = VK_NULL_HANDLE;
    VkDescriptorPool mVkDescriptorPool = VK_NULL_HANDLE;
    VkFence mVkFences[CONCURRENT_FRAMES];
    VkFence mVkImmediateFence = VK_NULL_HANDLE;
};

void initRenderer(RendererDesc desc, Renderer* pRenderer);
void destroyRenderer(Renderer* pRenderer);

void waitForCommands(Renderer* pRenderer);

void beginFrame(Renderer* pRenderer);
void endFrame(Renderer* pRenderer);
