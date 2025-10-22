#pragma once
#include "../core/base.hpp"
#include "vulkan/vulkan_core.h"

struct Renderer;

// --------------------------------------
// Shader Resource
enum ShaderResourceType
{
    RESOURCE_UNIFORM_BUFFER         = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    RESOURCE_DYNAMIC_UNIFORM_BUFFER = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
    RESOURCE_STORAGE_BUFFER         = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
    RESOURCE_DYNAMIC_STORAGE_BUFFER = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
    RESOURCE_TEXTURE                = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
    RESOURCE_SAMPLER                = VK_DESCRIPTOR_TYPE_SAMPLER,
};

#define MAX_COUNT_PER_RESOURCE 1024
struct ShaderResource
{
    ShaderResourceType mType;
    void* pData = NULL;
    uint32 mCount = 1;
};

#define MAX_SHADER_RESOURCES_PER_SET 16
struct ShaderResourceSetDesc
{
    ShaderResource mResources[MAX_SHADER_RESOURCES_PER_SET];
    uint32 mCount = 0;
};

struct ShaderResourceSet
{
    ShaderResourceSetDesc mDesc = {};

    VkDescriptorSetLayout mVkLayout = VK_NULL_HANDLE;
    VkDescriptorSet mVkSet = VK_NULL_HANDLE;
};

void addResourceSet(Renderer* pRenderer, ShaderResourceSetDesc desc, ShaderResourceSet** ppSet);
void removeResourceSet(Renderer* pRenderer, ShaderResourceSet** ppSet);
