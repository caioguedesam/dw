#include "render.hpp"
#include "../core/debug.hpp"
#include "src/render/texture.hpp"
#include "vulkan/vulkan_core.h"

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
    textureDesc.mLayout = IMAGE_LAYOUT_COLOR_OUTPUT;
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
    textureDesc.mLayout = IMAGE_LAYOUT_COLOR_OUTPUT;
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
    *pRenderer = {};
    // Initializing reusable data pools
}
