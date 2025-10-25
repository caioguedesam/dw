#include "render.hpp"
#include "../core/debug.hpp"
#include "src/core/memory.hpp"
#include "src/render/buffer.hpp"
#include "src/render/texture.hpp"
#include "src/render/shader.hpp"
#include "src/render/resource.hpp"
#include "src/render/command_buffer.hpp"
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_win32.h"

// Helper to match c strings in Vulkan property-style structs,
// e.g: int32 match = FIND_STRING_IN_PROPERTIES(props, propName, "myPropName");
#define FIND_STRING_IN_VK_PROPERTIES(ARR, MEMBER_NAME, MATCH) \
    ({\
     int32 result = -1;\
     int32 arrLen = ARR_LEN((ARR));\
     for(int32 iProp = 0; i < arrLen; iProp++) {\
        if(strcmp(MATCH, ARR[iProp].MEMBER_NAME) == 0) {\
            result = iProp;\
            break;\
        }\
     }\
     result;\
     })\

VKAPI_ATTR VkBool32 VKAPI_CALL validationLayerDebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT type,
        const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
        void* userData)
{
    LOGLF("VULKAN DEBUG", "%s", callbackData->pMessage);
    ASSERT(!(severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT));

    return VK_FALSE;
}

void initSwapChain(Renderer* pRenderer, SwapChain* pSwapChain)
{
    ASSERT(pRenderer && pSwapChain);
    *pSwapChain = {};

    // Required format is BGRA8_SRGB, SRGB_NONLINEAR (Swap chain performs gamma correction, not shaders)
    // Fallback is first format found.
    uint32 formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(pRenderer->mVkPhysicalDevice, pRenderer->mVkSurface, &formatCount, NULL);
    ASSERT(formatCount);
    VkSurfaceFormatKHR formats[formatCount];
    vkGetPhysicalDeviceSurfaceFormatsKHR(pRenderer->mVkPhysicalDevice, pRenderer->mVkSurface, &formatCount, formats);

    VkSurfaceFormatKHR format = formats[0];
    for(uint32 i = 0; i < formatCount; i++)
    {
        if(formats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
            formats[i].colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
        {
            format = formats[i];
            break;
        }
    }

    // Required present mode is MAILBOX, fallback is first present mode found.
    uint32 presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(pRenderer->mVkPhysicalDevice, pRenderer->mVkSurface, &presentModeCount, NULL);
    ASSERT(presentModeCount);
    VkPresentModeKHR presentModes[presentModeCount];
    vkGetPhysicalDeviceSurfacePresentModesKHR(pRenderer->mVkPhysicalDevice, pRenderer->mVkSurface, &presentModeCount, presentModes);

    VkPresentModeKHR presentMode = presentModes[0];
    for(uint32 i = 0; i < formatCount; i++)
    {
        if(presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            presentMode = presentModes[i];
            break;
        }
    }

    // Image details
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pRenderer->mVkPhysicalDevice, pRenderer->mVkSurface, &capabilities);
    ASSERT(capabilities.currentExtent.width != -1);
    VkExtent2D extents = capabilities.currentExtent;
    uint32 imageCount = capabilities.minImageCount + 1; // Double buffering
    if(capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
    {
        imageCount = capabilities.maxImageCount;
    }

    // Swap chain creation
    // Creating swap chain object
    VkSwapchainCreateInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    info.surface = pRenderer->mVkSurface;
    info.minImageCount = imageCount;
    info.imageFormat = format.format;
    info.imageColorSpace = format.colorSpace;
    info.imageArrayLayers = 1;
    info.imageExtent = extents;
    info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.preTransform = capabilities.currentTransform;
    info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    info.presentMode = presentMode;
    info.clipped = VK_TRUE;
    info.oldSwapchain = VK_NULL_HANDLE;

    VkSwapchainKHR vkSwapChain;
    VkResult ret = vkCreateSwapchainKHR(pRenderer->mVkDevice, &info, NULL, &vkSwapChain);
    ASSERTVK(ret);

    pSwapChain->mVkSwapChain = vkSwapChain;
    pSwapChain->mVkFormat = format.format;
    pSwapChain->mVkColorSpace = format.colorSpace;
    pSwapChain->mVkPresentMode = presentMode;
    pSwapChain->mVkExtents = extents;
    pSwapChain->mImageCount = imageCount;

    // Swap chain image view creation
    ret = vkGetSwapchainImagesKHR(pRenderer->mVkDevice, vkSwapChain, &imageCount, NULL);
    ASSERTVK(ret);
    ASSERT(imageCount);
    VkImage vkImages[imageCount];
    ret = vkGetSwapchainImagesKHR(pRenderer->mVkDevice, vkSwapChain, &imageCount, vkImages);
    ASSERTVK(ret);

    VkImageView vkImageViews[imageCount];
    for(uint32 i = 0; i < imageCount; i++)
    {
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = vkImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format.format;
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        ret = vkCreateImageView(pRenderer->mVkDevice, &viewInfo, NULL, &vkImageViews[i]);
        ASSERTVK(ret);
        pSwapChain->mVkImages[i] = vkImages[i];
        pSwapChain->mVkImageViews[i] = vkImageViews[i];
        pSwapChain->mVkImageLayouts[i] = VK_IMAGE_LAYOUT_UNDEFINED;
    }
}

void destroySwapChain(Renderer* pRenderer, SwapChain* pSwapChain)
{
    ASSERT(pRenderer && pSwapChain);

    for(uint32 i = 0; i < pSwapChain->mImageCount; i++)
    {
        vkDestroyImageView(pRenderer->mVkDevice, pSwapChain->mVkImageViews[i], NULL);
    }
    vkDestroySwapchainKHR(pRenderer->mVkDevice, pSwapChain->mVkSwapChain, NULL);

    *pSwapChain = {};
}

void addRenderTarget(Renderer* pRenderer, RenderTargetDesc desc, ClearValue clear, RenderTarget** ppTarget)
{
    ASSERT(pRenderer && ppTarget);
    ASSERT(*ppTarget == NULL);

    *ppTarget = (RenderTarget*)poolAlloc(&pRenderer->poolRenderTargets);

    **ppTarget = {};

    TextureDesc textureDesc = {};
    textureDesc.mFormat = desc.mFormat;
    textureDesc.mBaseLayout = IMAGE_LAYOUT_COLOR_OUTPUT;
    textureDesc.mType = TEXTURE_TYPE_2D;
    textureDesc.mUsage =
        TEXTURE_USAGE_COLOR_TARGET |
        TEXTURE_USAGE_TRANSFER_SRC |
        TEXTURE_USAGE_TRANSFER_DST;
    textureDesc.mWidth = desc.mWidth;
    textureDesc.mHeight = desc.mHeight;
    textureDesc.mMipCount = 1;
    textureDesc.mSamples = desc.mSamples;
    Texture* pTexture = NULL;
    addTexture(pRenderer, textureDesc, &pTexture);
    ASSERT(pTexture);

    (*ppTarget)->pTexture = pTexture;
    (*ppTarget)->mDesc = desc;
}

void addDepthTarget(Renderer* pRenderer, RenderTargetDesc desc, ClearValue clear, RenderTarget** ppTarget)
{
    ASSERT(pRenderer && ppTarget);
    ASSERT(*ppTarget == NULL);

    *ppTarget = (RenderTarget*)poolAlloc(&pRenderer->poolRenderTargets);

    **ppTarget = {};

    TextureDesc textureDesc = {};
    textureDesc.mFormat = desc.mFormat;
    textureDesc.mBaseLayout = IMAGE_LAYOUT_DEPTH_STENCIL_OUTPUT;
    textureDesc.mType = TEXTURE_TYPE_2D;
    textureDesc.mUsage =
        TEXTURE_USAGE_DEPTH_TARGET |
        TEXTURE_USAGE_TRANSFER_SRC |
        TEXTURE_USAGE_TRANSFER_DST;
    textureDesc.mWidth = desc.mWidth;
    textureDesc.mHeight = desc.mHeight;
    textureDesc.mMipCount = 1;
    textureDesc.mSamples = desc.mSamples;
    Texture* pTexture = NULL;
    addTexture(pRenderer, textureDesc, &pTexture);
    ASSERT(pTexture);

    (*ppTarget)->pTexture = pTexture;
    (*ppTarget)->mDesc = desc;
}

void removeRenderTarget(Renderer* pRenderer, RenderTarget** ppTarget)
{
    ASSERT(pRenderer && ppTarget);
    ASSERT(*ppTarget);

    removeTexture(pRenderer, &(*ppTarget)->pTexture); 
    ASSERT((*ppTarget)->pTexture == NULL);

    **ppTarget = {};

    poolFree(&pRenderer->poolRenderTargets, *ppTarget);
    *ppTarget = NULL;
}

void initVertexLayout(VertexLayoutDesc desc, VertexLayout* pLayout)
{
    ASSERT(pLayout);

    pLayout->mDesc = desc;
    pLayout->mVkBinding = {};
    pLayout->mVkBinding.binding = 0;
    pLayout->mVkBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    pLayout->mVkBinding.stride = 0;

    for(uint32 i = 0; i < desc.mCount; i++)
    {
        VertexAttrib attr = desc.mAttribs[i];

        pLayout->mVkAttribs[i] = {};
        pLayout->mVkAttribs[i].binding = 0;
        pLayout->mVkAttribs[i].location = i;
        pLayout->mVkAttribs[i].offset = pLayout->mVkBinding.stride;

        uint32 attrSize = 0;
        switch(attr)
        {
            case ATTRIBUTE_FLOAT:
            {
                pLayout->mVkAttribs[i].format = VK_FORMAT_R32_SFLOAT;
                attrSize = sizeof(float);
            } break;
            case ATTRIBUTE_FLOAT2:
            {
                pLayout->mVkAttribs[i].format = VK_FORMAT_R32G32_SFLOAT;
                attrSize = 2 * sizeof(float);
            } break;
            case ATTRIBUTE_FLOAT3:
            {
                pLayout->mVkAttribs[i].format = VK_FORMAT_R32G32B32_SFLOAT;
                attrSize = 3 * sizeof(float);
            } break;
            case ATTRIBUTE_FLOAT4:
            {
                pLayout->mVkAttribs[i].format = VK_FORMAT_R32G32B32A32_SFLOAT;
                attrSize = 4 * sizeof(float);
            } break;
            default: ASSERTF(0, "Unsupported vertex attribute format");
        }

        pLayout->mVkBinding.stride += attrSize;
    }
}

void addPipeline(Renderer* pRenderer, GraphicsPipelineDesc desc, GraphicsPipeline** ppPipeline)
{
    ASSERT(pRenderer && ppPipeline);
    ASSERT(*ppPipeline == NULL);

    *ppPipeline = (GraphicsPipeline*)poolAlloc(&pRenderer->poolGraphicsPipelines);

    **ppPipeline = {};

    // Shader stages
    VkPipelineShaderStageCreateInfo shaderInfos[2];
    shaderInfos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderInfos[0].pName = "main";
    shaderInfos[0].module = desc.pVS->mVkShader;
    shaderInfos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderInfos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderInfos[1].pName = "main";
    shaderInfos[1].module = desc.pFS->mVkShader;
    shaderInfos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo iaInfo = {};
    iaInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    iaInfo.topology = (VkPrimitiveTopology)desc.mPrimitive;
    iaInfo.primitiveRestartEnable = VK_FALSE;

    // Vertex input
    VkPipelineVertexInputStateCreateInfo viInfo = {};
    viInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    viInfo.vertexBindingDescriptionCount = 1;
    viInfo.pVertexBindingDescriptions = &desc.mVertexLayout.mVkBinding;
    viInfo.vertexAttributeDescriptionCount = desc.mVertexLayout.mDesc.mCount;
    viInfo.pVertexAttributeDescriptions = desc.mVertexLayout.mVkAttribs;

    // Dynamic state (viewport, scissor, depth settings)
    VkDynamicState dynamicStates[] =
    {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE,
        VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE,
        VK_DYNAMIC_STATE_DEPTH_COMPARE_OP,
    };
    VkPipelineDynamicStateCreateInfo dynInfo = {};
    dynInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynInfo.dynamicStateCount = ARR_LEN(dynamicStates);
    dynInfo.pDynamicStates = dynamicStates;

    // Viewport state
    VkPipelineViewportStateCreateInfo vpInfo = {};
    vpInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vpInfo.viewportCount = 1;
    vpInfo.scissorCount = 1;

    // Rasterization state
    VkPipelineRasterizationStateCreateInfo rsInfo = {};
    rsInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rsInfo.polygonMode = (VkPolygonMode)desc.mFillMode;
    rsInfo.cullMode = (VkCullModeFlags)desc.mCullMode;
    rsInfo.frontFace = (VkFrontFace)desc.mFrontFace;
    rsInfo.lineWidth = desc.mLineWidth;
    rsInfo.depthClampEnable = VK_FALSE;
    rsInfo.depthBiasClamp = VK_FALSE;

    // Blending
    VkPipelineColorBlendAttachmentState blendStates[desc.mRenderTargetCount];
    for(uint32 i = 0; i < desc.mRenderTargetCount; i++)
    {
        blendStates[i] = {};
        blendStates[i].blendEnable = desc.mBlendEnable;
        blendStates[i].colorWriteMask = (VkColorComponentFlags)desc.mBlendMask;
        blendStates[i].colorBlendOp = (VkBlendOp)desc.mBlendOp;
        blendStates[i].alphaBlendOp = (VkBlendOp)desc.mBlendOp;
        blendStates[i].srcColorBlendFactor = (VkBlendFactor)desc.mSrcColorFactor;
        blendStates[i].srcAlphaBlendFactor = (VkBlendFactor)desc.mSrcAlphaFactor;
        blendStates[i].dstColorBlendFactor = (VkBlendFactor)desc.mDstColorFactor;
        blendStates[i].dstAlphaBlendFactor = (VkBlendFactor)desc.mDstAlphaFactor;
    }
    VkPipelineColorBlendStateCreateInfo blendInfo = {};
    blendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blendInfo.logicOpEnable = VK_FALSE;
    blendInfo.attachmentCount = desc.mRenderTargetCount;
    blendInfo.pAttachments = blendStates;

    // Depth/stencil state
    VkPipelineDepthStencilStateCreateInfo depthInfo = {};
    depthInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthInfo.depthTestEnable = desc.mDepthTest;
    depthInfo.depthWriteEnable = desc.mDepthWrite;
    depthInfo.depthCompareOp = (VkCompareOp)desc.mDepthOp;
    depthInfo.depthBoundsTestEnable = VK_FALSE;
    depthInfo.stencilTestEnable = VK_FALSE;     // TODO_DW: Stencil

    // Resource set layout
    VkDescriptorSetLayout setLayouts[desc.mResourceSetCount];
    for(uint32 i = 0; i < desc.mResourceSetCount; i++)
    {
        setLayouts[i] = desc.pResourceSets[i]->mVkLayout;
    }

    VkPipelineLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = desc.mResourceSetCount;
    layoutInfo.pSetLayouts = setLayouts;
    layoutInfo.pushConstantRangeCount = 0;      // TODO_DW: Push constants
    layoutInfo.pPushConstantRanges = NULL;
    VkPipelineLayout vkLayout;
    VkResult ret = vkCreatePipelineLayout(pRenderer->mVkDevice,
            &layoutInfo,
            NULL,
            &vkLayout);

    // Pipeline
    VkPipelineRenderingCreateInfoKHR renderInfo = {};
    renderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    renderInfo.colorAttachmentCount = desc.mRenderTargetCount;
    renderInfo.pColorAttachmentFormats = (VkFormat*)(&desc.mRenderTargetFormats);
    renderInfo.depthAttachmentFormat = (VkFormat)desc.mDepthTargetFormat;

    VkGraphicsPipelineCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    info.pNext = &renderInfo;
    info.stageCount = 2;
    info.pStages = shaderInfos;
    info.pVertexInputState = &viInfo;
    info.pInputAssemblyState = &iaInfo;
    info.pDynamicState = &dynInfo;
    info.pViewportState = &vpInfo;
    info.pRasterizationState = &rsInfo;
    info.pColorBlendState = &blendInfo;
    info.pDepthStencilState = &depthInfo;
    info.layout = vkLayout;
    info.renderPass = NULL;
    VkPipeline vkPipeline;
    ret = vkCreateGraphicsPipelines(pRenderer->mVkDevice, 
            VK_NULL_HANDLE, 
            1, 
            &info, 
            NULL, 
            &vkPipeline);
    ASSERTVK(ret);

    (*ppPipeline)->mDesc = desc;
    (*ppPipeline)->mVkPipeline = vkPipeline;
    (*ppPipeline)->mVkLayout = vkLayout;
}

void removePipeline(Renderer* pRenderer, GraphicsPipeline** ppPipeline)
{
    ASSERT(pRenderer && ppPipeline);
    ASSERT(*ppPipeline);

    vkDestroyPipelineLayout(pRenderer->mVkDevice, (*ppPipeline)->mVkLayout, NULL);
    vkDestroyPipeline(pRenderer->mVkDevice, (*ppPipeline)->mVkPipeline, NULL);

    **ppPipeline = {};

    poolFree(&pRenderer->poolGraphicsPipelines, *ppPipeline);
    *ppPipeline = NULL;
}

void addPipeline(Renderer* pRenderer, ComputePipelineDesc desc, ComputePipeline** ppPipeline)
{
    ASSERT(pRenderer && ppPipeline);
    ASSERT(*ppPipeline == NULL);

    *ppPipeline = (ComputePipeline*)poolAlloc(&pRenderer->poolComputePipelines);

    **ppPipeline = {};

    // Shader stages
    VkPipelineShaderStageCreateInfo shaderInfo = {};
    shaderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderInfo.pName = "main";
    shaderInfo.module = desc.pCS->mVkShader;
    shaderInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;

    // Resource set layout
    VkDescriptorSetLayout setLayouts[desc.mResourceSetCount];
    for(uint32 i = 0; i < desc.mResourceSetCount; i++)
    {
        setLayouts[i] = desc.pResourceSets[i]->mVkLayout;
    }

    VkPipelineLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = desc.mResourceSetCount;
    layoutInfo.pSetLayouts = setLayouts;
    layoutInfo.pushConstantRangeCount = 0;      // TODO_DW: Push constants
    layoutInfo.pPushConstantRanges = NULL;
    VkPipelineLayout vkLayout;
    VkResult ret = vkCreatePipelineLayout(pRenderer->mVkDevice,
            &layoutInfo,
            NULL,
            &vkLayout);

    // Pipeline
    VkComputePipelineCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    info.layout = vkLayout;
    info.stage = shaderInfo;
    VkPipeline vkPipeline;
    ret = vkCreateComputePipelines(pRenderer->mVkDevice, 
            VK_NULL_HANDLE, 
            1, 
            &info, 
            NULL, 
            &vkPipeline);
    ASSERTVK(ret);

    (*ppPipeline)->mDesc = desc;
    (*ppPipeline)->mVkPipeline = vkPipeline;
    (*ppPipeline)->mVkLayout = vkLayout;
}

void removePipeline(Renderer* pRenderer, ComputePipeline** ppPipeline)
{
    ASSERT(pRenderer && ppPipeline);
    ASSERT(*ppPipeline);

    vkDestroyPipelineLayout(pRenderer->mVkDevice, (*ppPipeline)->mVkLayout, NULL);
    vkDestroyPipeline(pRenderer->mVkDevice, (*ppPipeline)->mVkPipeline, NULL);

    **ppPipeline = {};

    poolFree(&pRenderer->poolComputePipelines, *ppPipeline);
    *ppPipeline = NULL;
}

void initRenderer(RendererDesc desc, Renderer* pRenderer)
{
    ASSERT(pRenderer);
    *pRenderer = {};

    // Initializing reusable data pools
    initPool(sizeof(Buffer), desc.mMaxBuffers, &pRenderer->poolBuffers);
    initPool(sizeof(Texture), desc.mMaxTextures, &pRenderer->poolTextures);
    initPool(sizeof(Sampler), desc.mMaxSamplers, &pRenderer->poolSamplers);
    initPool(sizeof(Shader), desc.mMaxShaders, &pRenderer->poolShaders);
    initPool(sizeof(ShaderResourceSet), desc.mMaxResourceSets, &pRenderer->poolResourceSets);
    initPool(sizeof(RenderTarget), desc.mMaxRenderTargets, &pRenderer->poolRenderTargets);
    initPool(sizeof(GraphicsPipeline), desc.mMaxGraphicsPipelines, &pRenderer->poolGraphicsPipelines);
    initPool(sizeof(ComputePipeline), desc.mMaxComputePipelines, &pRenderer->poolComputePipelines);

    // Initializing VK instance
    VkInstance vkInstance;
    {
        // Application
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "dw";
        appInfo.applicationVersion = VK_MAKE_VERSION(1,0,0);
        appInfo.pEngineName = "dw";
        appInfo.engineVersion = VK_MAKE_VERSION(1,0,0);
        appInfo.apiVersion = VK_API_VERSION_1_3;

        // Instance
        VkInstanceCreateInfo instanceInfo = {};
        instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceInfo.pApplicationInfo = &appInfo;

        // Extensions
        const char* pExtensions[] =
        {
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#if DW_DEBUG
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
        };
        instanceInfo.enabledExtensionCount = ARR_LEN(pExtensions);
        instanceInfo.ppEnabledExtensionNames = pExtensions;

#if DW_DEBUG
        // Debug validation
        const char* pLayerNames[] =
        {
            "VK_LAYER_KHRONOS_validation",
        };
        uint32 layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, NULL);
        ASSERT(layerCount);
        VkLayerProperties layers[layerCount];
        vkEnumerateInstanceLayerProperties(&layerCount, layers);
        for(uint32 i = 0; i < layerCount; i++)
        {
            int32 match = FIND_STRING_IN_VK_PROPERTIES(layers, layerName, pLayerNames[i]);
            ASSERT(match != -1);
        }
        instanceInfo.enabledLayerCount = ARR_LEN(pLayerNames);
        instanceInfo.ppEnabledLayerNames = pLayerNames;
#endif

        VkResult ret = vkCreateInstance(&instanceInfo, NULL, &vkInstance);
        ASSERTVK(ret);
    }

#if DW_DEBUG
    // Setting up API debug validation
    VkDebugUtilsMessengerEXT vkMessenger;
    {
        VkDebugUtilsMessengerCreateInfoEXT messengerInfo = {};
        messengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        messengerInfo.messageSeverity =
              VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        messengerInfo.messageType =
              VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        messengerInfo.pfnUserCallback = validationLayerDebugCallback;
        messengerInfo.pUserData = NULL;

        PFN_vkCreateDebugUtilsMessengerEXT fn = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
                vkInstance, 
                "vkCreateDebugUtilsMessengerEXT");
        ASSERT(fn);
        VkResult ret = fn(vkInstance, &messengerInfo, NULL, &vkMessenger);
        ASSERTVK(ret);
    }
#endif

    // Initializing surface
    VkSurfaceKHR vkSurface;
    {
        VkWin32SurfaceCreateInfoKHR info = {};
        info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        info.hwnd = desc.pApp->mWindow.mWinHandle;
        info.hinstance = desc.pApp->mWindow.mWinInstance;

        VkResult ret = vkCreateWin32SurfaceKHR(vkInstance, &info, NULL, &vkSurface);
        ASSERTVK(ret);
    }

    // Initialing physical device
    VkPhysicalDevice vkPhysicalDevice; 
    VkPhysicalDeviceProperties vkPhysicalDeviceProps;
    const char* pDeviceExtensions[] =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
    };
    {
        // Selecting the first device to match requirements (might select least powerful GPU)
        uint32 deviceCount = 0;
        vkEnumeratePhysicalDevices(vkInstance, &deviceCount, NULL);
        ASSERT(deviceCount);
        VkPhysicalDevice devices[deviceCount];
        vkEnumeratePhysicalDevices(vkInstance, &deviceCount, devices);

        int32 selectedDevice = -1;
        for(int32 i = 0; i < deviceCount; i++)
        {
            VkPhysicalDevice device = devices[i];

            // Check for extension support
            uint32 extensionCount = 0;
            vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, NULL);
            ASSERT(extensionCount);
            VkExtensionProperties deviceExtensions[extensionCount];
            vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, deviceExtensions);
            
            bool supportsExtensions = true;
            for(int32 j = 0; j < ARR_LEN(pDeviceExtensions); j++)
            {
                const char* ext = pDeviceExtensions[j];
                int32 match = FIND_STRING_IN_VK_PROPERTIES(deviceExtensions, extensionName, ext);
                if(match == -1)
                {
                    supportsExtensions = false;
                    break;
                }
            }
            if(!supportsExtensions) continue;

            // Check for surface properties support
            uint32 surfaceFormatCount = 0;
            uint32 surfacePresentModeCount = 0;
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, vkSurface, &surfaceFormatCount, NULL);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, vkSurface, &surfacePresentModeCount, NULL);
            if(!surfaceFormatCount || !surfacePresentModeCount) continue;

            // Check for desired application features support
            VkPhysicalDeviceProperties properties;
            VkPhysicalDeviceFeatures features;
            vkGetPhysicalDeviceProperties(device, &properties);
            vkGetPhysicalDeviceFeatures(device, &features);
            if(properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) continue;
            if(!features.samplerAnisotropy) continue;
            if(!features.fillModeNonSolid) continue;
            if(!features.wideLines) continue;

            selectedDevice = i;
            break;
        }
        ASSERT(selectedDevice != -1);
        vkPhysicalDevice = devices[selectedDevice];
        vkGetPhysicalDeviceProperties(vkPhysicalDevice, &vkPhysicalDeviceProps);
    }

    // Initializing logical device and command queue
    VkDevice vkDevice;
    VkQueue vkQueue;
    uint32 vkQueueFamily = 0;
    {
        uint32 queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueFamilyCount, NULL);
        ASSERT(queueFamilyCount);
        VkQueueFamilyProperties queueFamilyProperties[queueFamilyCount];
        vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueFamilyCount, queueFamilyProperties);

        // Get first command queue family that supports required command types
        // (graphics, compute, present)
        int32 queueFamily = -1;
        for(int32 i = 0; i < queueFamilyCount; i++)
        {
            VkQueueFamilyProperties properties = queueFamilyProperties[i];
            if(!(properties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                    || !(properties.queueFlags & VK_QUEUE_COMPUTE_BIT)) continue;
            
            PFN_vkGetPhysicalDeviceSurfaceSupportKHR fn = 
                (PFN_vkGetPhysicalDeviceSurfaceSupportKHR)vkGetInstanceProcAddr(
                        vkInstance, 
                        "vkGetPhysicalDeviceSurfaceSupportKHR");
            ASSERT(fn);

            VkBool32 supportsPresent = VK_FALSE;
            fn(vkPhysicalDevice, i, vkSurface, &supportsPresent);
            if(supportsPresent == VK_FALSE) continue;

            queueFamily = i;
            break;
        }
        ASSERT(queueFamily != -1);
        vkQueueFamily = (uint32)queueFamily;

        VkDeviceQueueCreateInfo queueInfo = {};
        queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.queueFamilyIndex = queueFamily;
        queueInfo.queueCount = 1;
        float priority = 1;
        queueInfo.pQueuePriorities = &priority;

        VkPhysicalDeviceFeatures features = {};
        features.samplerAnisotropy = VK_TRUE;
        features.fillModeNonSolid = VK_TRUE;
        features.wideLines = VK_TRUE;
        VkPhysicalDeviceDescriptorIndexingFeatures indexingFeatures = {};
        indexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
        indexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;
        indexingFeatures.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE;
        indexingFeatures.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
        indexingFeatures.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
        indexingFeatures.descriptorBindingStorageImageUpdateAfterBind = VK_TRUE;

        VkDeviceCreateInfo deviceInfo = {};
        deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceInfo.queueCreateInfoCount = 1;
        deviceInfo.pQueueCreateInfos = &queueInfo;
        deviceInfo.pEnabledFeatures = &features;
        deviceInfo.enabledExtensionCount = ARR_LEN(pDeviceExtensions);
        deviceInfo.ppEnabledExtensionNames = pDeviceExtensions;
        deviceInfo.pNext = &indexingFeatures;

        VkResult ret = vkCreateDevice(vkPhysicalDevice, &deviceInfo, NULL, &vkDevice);
        ASSERTVK(ret);

        vkGetDeviceQueue(vkDevice, queueFamily, 0, &vkQueue);
    }

    // Initializing resource memory allocator
    VmaAllocator vkAllocator;
    {
        VmaAllocatorCreateInfo info = {};
        info.instance = vkInstance;
        info.physicalDevice = vkPhysicalDevice;
        info.device = vkDevice;

        VkResult ret = vmaCreateAllocator(&info, &vkAllocator);
        ASSERTVK(ret);
    }

    // Initializing descriptor pools for allocating descriptors
    uint32 maxPoolSize = 100;
    VkDescriptorPool vkDescriptorPool;
    {
        VkDescriptorPoolSize poolSizes[] =
        {
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,            maxPoolSize},
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,    maxPoolSize},
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,            maxPoolSize},
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,    maxPoolSize},
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,             maxPoolSize},
            { VK_DESCRIPTOR_TYPE_SAMPLER,                   maxPoolSize},
        };
        VkDescriptorPoolCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        info.poolSizeCount = ARR_LEN(poolSizes);
        info.pPoolSizes = poolSizes;
        info.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
        VkResult ret = vkCreateDescriptorPool(vkDevice, &info, NULL, &vkDescriptorPool);
        ASSERTVK(ret);
    }

    // Initializing command pool
    VkCommandPool vkCommandPool;
    {
        VkCommandPoolCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        info.queueFamilyIndex = vkQueueFamily;
        info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        VkResult ret = vkCreateCommandPool(vkDevice, &info, NULL, &vkCommandPool);
        ASSERTVK(ret);
    }

    // Initializing sync primitives
    {
        VkResult ret;
        for(uint32 i = 0; i < CONCURRENT_FRAMES; i++)
        {
            VkSemaphoreCreateInfo semaphoreInfo = {};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            ret = vkCreateSemaphore(vkDevice, &semaphoreInfo, NULL, &pRenderer->mVkRenderSemaphores[i]);
            ASSERTVK(ret);
            ret = vkCreateSemaphore(vkDevice, &semaphoreInfo, NULL, &pRenderer->mVkPresentSemaphores[i]);
            ASSERTVK(ret);

            VkFenceCreateInfo fenceInfo = {};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            ret = vkCreateFence(vkDevice, &fenceInfo, NULL, &pRenderer->mVkFences[i]);
            ASSERTVK(ret);
        }

        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        ret = vkCreateFence(vkDevice, &fenceInfo, NULL, &pRenderer->mVkImmediateFence);
        ASSERTVK(ret);
    }

    pRenderer->mVkInstance = vkInstance;
#if DW_DEBUG
    pRenderer->mVkDebugMessenger = vkMessenger;
#endif
    pRenderer->mVkSurface = vkSurface;
    pRenderer->mVkPhysicalDevice = vkPhysicalDevice;
    pRenderer->mVkDeviceProperties = vkPhysicalDeviceProps;
    pRenderer->mVkDevice = vkDevice;
    pRenderer->mVkQueue = vkQueue;
    pRenderer->mVkAllocator = vkAllocator;
    pRenderer->mVkDescriptorPool = vkDescriptorPool;
    pRenderer->mVkCommandPool = vkCommandPool;

    // Initializing swap chain
    initSwapChain(pRenderer, &pRenderer->mSwapChain);

    // Initializing command buffers
    initCommandBuffers(pRenderer);
}

void destroyRenderer(Renderer* pRenderer)
{
    ASSERT(pRenderer);

    waitForCommands(pRenderer);

    destroySwapChain(pRenderer, &pRenderer->mSwapChain);
    
    for(uint32 i = 0; i < CONCURRENT_FRAMES; i++)
    {
        vkDestroySemaphore(pRenderer->mVkDevice, pRenderer->mVkRenderSemaphores[i], NULL);
        vkDestroySemaphore(pRenderer->mVkDevice, pRenderer->mVkPresentSemaphores[i], NULL);
        vkDestroyFence(pRenderer->mVkDevice, pRenderer->mVkFences[i], NULL);
    }
    vkDestroyFence(pRenderer->mVkDevice, pRenderer->mVkImmediateFence, NULL);
    vkDestroyCommandPool(pRenderer->mVkDevice, pRenderer->mVkCommandPool, NULL);
    vkDestroyDescriptorPool(pRenderer->mVkDevice, pRenderer->mVkDescriptorPool, NULL);
    vmaDestroyAllocator(pRenderer->mVkAllocator);
    vkDestroyDevice(pRenderer->mVkDevice, NULL);
    vkDestroySurfaceKHR(pRenderer->mVkInstance, pRenderer->mVkSurface, NULL);
#if DW_DEBUG
    PFN_vkDestroyDebugUtilsMessengerEXT fn =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(pRenderer->mVkInstance, "vkDestroyDebugUtilsMessengerEXT");
    ASSERT(fn);
    fn(pRenderer->mVkInstance, pRenderer->mVkDebugMessenger, NULL);
#endif
    vkDestroyInstance(pRenderer->mVkInstance, NULL);

    destroyPool(&pRenderer->poolBuffers);
    destroyPool(&pRenderer->poolTextures);
    destroyPool(&pRenderer->poolSamplers);
    destroyPool(&pRenderer->poolShaders);
    destroyPool(&pRenderer->poolResourceSets);
    destroyPool(&pRenderer->poolRenderTargets);
    destroyPool(&pRenderer->poolGraphicsPipelines);
    destroyPool(&pRenderer->poolComputePipelines);
}

void waitForCommands(Renderer* pRenderer)
{
    ASSERT(pRenderer);
    vkDeviceWaitIdle(pRenderer->mVkDevice);
}

void acquireNextImage(Renderer* pRenderer, uint32 frame)
{
    ASSERT(pRenderer);

    pRenderer->mActiveFrame = frame % CONCURRENT_FRAMES;
    
    VkSemaphore vkPresentSemaphore = pRenderer->mVkPresentSemaphores[pRenderer->mActiveFrame];
    VkResult ret = vkAcquireNextImageKHR(pRenderer->mVkDevice, 
            pRenderer->mSwapChain.mVkSwapChain, 
            MAX_UINT64, 
            vkPresentSemaphore, 
            VK_NULL_HANDLE,
            &pRenderer->mSwapChain.mActiveImage);
    ASSERTVK(ret);      // TODO_DW: Verify if load requests handles out of date swapchain before this.
}

void present(Renderer* pRenderer, uint32 frame)
{
    ASSERT(pRenderer);
    ASSERT(pRenderer->mSwapChain.mVkImageLayouts[pRenderer->mSwapChain.mActiveImage] == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    VkSemaphore vkRenderSemaphore = pRenderer->mVkRenderSemaphores[pRenderer->mActiveFrame];
    VkPresentInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.swapchainCount = 1;
    info.pSwapchains = &pRenderer->mSwapChain.mVkSwapChain;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &vkRenderSemaphore;
    info.pImageIndices = &pRenderer->mSwapChain.mActiveImage;
    VkResult ret = vkQueuePresentKHR(pRenderer->mVkQueue, 
            &info);
    ASSERTVK(ret);      // TODO_DW: Verify if load requests handles out of date swapchain before this.
}

void cmdBarrier(CommandBuffer* pCmd, Barrier barrier)
{
    ASSERT(pCmd);
    VkMemoryBarrier vkBarrier = {};
    vkBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    vkBarrier.srcAccessMask = (VkAccessFlags)barrier.mSrcAccess;
    vkBarrier.dstAccessMask = (VkAccessFlags)barrier.mDstAccess;
    vkCmdPipelineBarrier(pCmd->mVkCmd, 
            (VkPipelineStageFlags)barrier.mSrcStage, 
            (VkPipelineStageFlags)barrier.mDstStage, 
            0, 
            1, &vkBarrier, 
            0, NULL, 
            0, NULL);
}

void cmdTextureBarrier(CommandBuffer* pCmd, TextureBarrier barrier)
{
    ASSERT(pCmd && barrier.pTexture);

    VkImageMemoryBarrier vkBarrier = {};
    vkBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    vkBarrier.oldLayout = (VkImageLayout)barrier.mOldLayout;
    vkBarrier.newLayout = (VkImageLayout)barrier.mNewLayout;
    vkBarrier.srcAccessMask = (VkAccessFlags)barrier.mSrcAccess;
    vkBarrier.dstAccessMask = (VkAccessFlags)barrier.mDstAccess;
    vkBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    vkBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    vkBarrier.image = barrier.pTexture->mVkImage;
    vkBarrier.subresourceRange.aspectMask = 
        barrier.pTexture->mDesc.mUsage & TEXTURE_USAGE_DEPTH_TARGET
        ? VK_IMAGE_ASPECT_DEPTH_BIT
        : VK_IMAGE_ASPECT_COLOR_BIT;
    // Transition all mips by default.
    if(barrier.mMipCount == 0)
    {
        vkBarrier.subresourceRange.baseMipLevel = 0;
        vkBarrier.subresourceRange.levelCount = barrier.pTexture->mDesc.mMipCount;
    }
    else
    {
        vkBarrier.subresourceRange.baseMipLevel = barrier.mStartMip;
        vkBarrier.subresourceRange.levelCount = barrier.mMipCount;
    }
    vkBarrier.subresourceRange.baseArrayLayer = 0;      // TODO_DW: Texture array
    vkBarrier.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(pCmd->mVkCmd, 
            (VkPipelineStageFlags)barrier.mSrcStage, 
            (VkPipelineStageFlags)barrier.mDstStage, 
            0, 
            0, NULL, 
            0, NULL, 
            1, &vkBarrier);

    if(barrier.mStartMip == 0)
    {
        barrier.pTexture->mDesc.mBaseLayout = barrier.mNewLayout;
    }
}

void cmdSwapChainBarrier(CommandBuffer* pCmd, SwapChain* pSwapChain, ImageLayout newLayout)
{
    ASSERT(pCmd && pSwapChain);

    VkImageMemoryBarrier vkBarrier = {};
    vkBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    vkBarrier.oldLayout = pSwapChain->mVkImageLayouts[pSwapChain->mActiveImage];
    vkBarrier.newLayout = (VkImageLayout)newLayout;
    vkBarrier.srcAccessMask = (VkAccessFlags)MEMORY_ACCESS_ALL_WRITES;      // TODO_DW: I can be less agressive with sync here
    vkBarrier.dstAccessMask = (VkAccessFlags)MEMORY_ACCESS_ALL_READS;
    vkBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    vkBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    vkBarrier.image = pSwapChain->mVkImages[pSwapChain->mActiveImage];
    vkBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    vkBarrier.subresourceRange.baseMipLevel = 0;
    vkBarrier.subresourceRange.levelCount = 1;
    vkBarrier.subresourceRange.baseArrayLayer = 0;
    vkBarrier.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(pCmd->mVkCmd, 
            (VkPipelineStageFlags)PIPELINE_STAGE_ALL, 
            (VkPipelineStageFlags)PIPELINE_STAGE_ALL, 
            0, 
            0, NULL, 
            0, NULL, 
            1, &vkBarrier);

    pSwapChain->mVkImageLayouts[pSwapChain->mActiveImage] = (VkImageLayout)newLayout;
};

void cmdClearRenderTarget(CommandBuffer* pCmd, RenderTarget* pTarget)
{
    ASSERT(pCmd && pTarget);

    VkClearColorValue clear =
    {
        pTarget->mDesc.mClear.mColor[0],
        pTarget->mDesc.mClear.mColor[1],
        pTarget->mDesc.mClear.mColor[2],
        pTarget->mDesc.mClear.mColor[3],
    };
    VkImageSubresourceRange range = {};
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel = 0;     // TODO_DW: Does it make sense to generate mips for RTs?
    range.levelCount = 1;
    range.baseArrayLayer = 0;
    range.layerCount = 1;

    vkCmdClearColorImage(pCmd->mVkCmd, 
            pTarget->pTexture->mVkImage, 
            (VkImageLayout)pTarget->pTexture->mDesc.mBaseLayout, 
            &clear, 
            1, &range);
}

void cmdClearDepthTarget(CommandBuffer* pCmd, RenderTarget* pTarget)
{
    ASSERT(pCmd && pTarget);

    ASSERT(pCmd && pTarget);

    VkClearDepthStencilValue clear = {};
    clear.depth = pTarget->mDesc.mClear.mDepth;

    VkImageSubresourceRange range = {};
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel = 0;     // TODO_DW: Does it make sense to generate mips for RTs?
    range.levelCount = 1;
    range.baseArrayLayer = 0;
    range.layerCount = 1;

    vkCmdClearDepthStencilImage(pCmd->mVkCmd, 
            pTarget->pTexture->mVkImage, 
            (VkImageLayout)pTarget->pTexture->mDesc.mBaseLayout, 
            &clear, 
            1, &range);
}
