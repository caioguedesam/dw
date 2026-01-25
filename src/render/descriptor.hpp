#pragma once
#include "../core/base.hpp"
#include "vulkan/vulkan_core.h"

struct Renderer;

// --------------------------------------
// Descriptor (pointer to shader resource)
enum DescriptorType
{
    DESCRIPTOR_UNIFORM_BUFFER         = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    DESCRIPTOR_DYNAMIC_UNIFORM_BUFFER = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
    DESCRIPTOR_STORAGE_BUFFER         = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
    DESCRIPTOR_DYNAMIC_STORAGE_BUFFER = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
    DESCRIPTOR_TEXTURE                = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
    DESCRIPTOR_SAMPLER                = VK_DESCRIPTOR_TYPE_SAMPLER,
};

struct Descriptor
{
    DescriptorType mType;
    void* pData = NULL;
    uint32 mCount = 1;
    uint32 mMaxCount = 1;
};

#define MAX_SHADER_RESOURCES_PER_SET 32
struct DescriptorSetDesc
{
    Descriptor mResources[MAX_SHADER_RESOURCES_PER_SET];
    uint32 mCount = 0;
};

struct DescriptorSet
{
    DescriptorSetDesc mDesc = {};

    VkDescriptorSetLayout mVkLayout = VK_NULL_HANDLE;
    VkDescriptorSet mVkSet = VK_NULL_HANDLE;
};

void addDescriptorSet(Renderer* pRenderer, DescriptorSetDesc desc, DescriptorSet** ppSet);
void removeDescriptorSet(Renderer* pRenderer, DescriptorSet** ppSet);
