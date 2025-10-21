#include "resource.hpp"
#include "render.hpp"
#include "buffer.hpp"
#include "texture.hpp"
#include "../core/debug.hpp"
#include "src/core/memory.hpp"
#include "vulkan/vulkan_core.h"

void addResourceSet(Renderer* pRenderer, ShaderResourceSetDesc desc, ShaderResourceSet** ppSet)
{
    ASSERT(pRenderer && ppSet);
    ASSERT(*ppSet == NULL);
    ASSERT(desc.mCount);

    *ppSet = (ShaderResourceSet*)poolAlloc(&pRenderer->poolResourceSets);

    **ppSet = {};

    uint32 count = desc.mCount;
    VkDescriptorSetLayoutBinding    vkBindings[count];
    VkDescriptorBindingFlags        vkBindingFlags[count];
    for(uint32 i = 0; i < count; i++)
    {
        vkBindings[i] = {};
        vkBindings[i].binding = i;
        vkBindings[i].stageFlags = VK_SHADER_STAGE_ALL;
        vkBindings[i].descriptorType = (VkDescriptorType)desc.mResources[i].mType;
        vkBindings[i].descriptorCount = (VkDescriptorType)desc.mResources[i].mCount;

        vkBindingFlags[i] =
            VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
            VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
    }

    VkDescriptorSetLayoutBindingFlagsCreateInfo flagsInfo = {};
    flagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    flagsInfo.bindingCount = count;
    flagsInfo.pBindingFlags = vkBindingFlags;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = count;
    layoutInfo.pBindings = vkBindings;
    layoutInfo.pNext = &flagsInfo;
    layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;

    VkDescriptorSetLayout vkLayout;
    VkResult ret = vkCreateDescriptorSetLayout(
            pRenderer->mVkDevice, 
            &layoutInfo, 
            NULL, 
            &vkLayout);
    ASSERTVK(ret);

    VkDescriptorSetAllocateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    info.descriptorPool = pRenderer->mVkDescriptorPool;
    info.descriptorSetCount = 1;
    info.pSetLayouts = &vkLayout;
    VkDescriptorSet vkSet;
    ret = vkAllocateDescriptorSets(
            pRenderer->mVkDevice, 
            &info, 
            &vkSet);
    ASSERTVK(ret);

    (*ppSet)->mDesc = desc;
    (*ppSet)->mVkLayout = vkLayout;
    (*ppSet)->mVkSet = vkSet;

    // Populate descriptor set with descriptors
    uint32 descriptorCount = 0;
    for(uint32 i = 0; i < count; i++)
    {
        descriptorCount += desc.mResources[i].mCount;
    }

    VkWriteDescriptorSet    vkWrites[count];
    VkDescriptorBufferInfo  vkBufferInfos[descriptorCount];
    VkDescriptorImageInfo   vkImageInfos[descriptorCount];
    uint32 cursor = 0;
    for(uint32 i = 0; i < count; i++)
    {
        ShaderResource res = desc.mResources[i];
        vkWrites[i] = {};
        vkWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        vkWrites[i].dstSet = vkSet;
        vkWrites[i].dstBinding = i;
        vkWrites[i].descriptorType = (VkDescriptorType)res.mType;
        vkWrites[i].descriptorCount = res.mCount;

        switch(res.mType)
        {
            case RESOURCE_UNIFORM_BUFFER:
            case RESOURCE_DYNAMIC_UNIFORM_BUFFER:
            case RESOURCE_STORAGE_BUFFER:
            case RESOURCE_DYNAMIC_STORAGE_BUFFER:
            {
                Buffer* pBuffer = (Buffer*)res.pData;
                vkBufferInfos[cursor] = {};
                vkBufferInfos[cursor].buffer = pBuffer->mVkHandle;
                vkBufferInfos[cursor].offset = 0;
                vkBufferInfos[cursor].range = pBuffer->mDesc.mSize;
                vkWrites[i].pBufferInfo = &vkBufferInfos[cursor];
                cursor++;
            } break;
            case RESOURCE_TEXTURE:
            {
                Texture* pStart = (Texture*)res.pData;
                uint32 textureWriteStart = cursor;
                for(uint32 j = 0; j < res.mCount; j++)
                {
                    Texture* pTexture = &pStart[j];
                    vkImageInfos[cursor] = {};
                    vkImageInfos[cursor].imageView = pTexture->mVkImageView;
                    vkImageInfos[cursor].imageLayout = (VkImageLayout)pTexture->mDesc.mLayout;
                    vkImageInfos[cursor].sampler = VK_NULL_HANDLE;
                    cursor++;
                }
                vkWrites[i].pImageInfo = &vkImageInfos[textureWriteStart];
            } break;
            case RESOURCE_SAMPLER:
            {
                Sampler* pSampler = (Sampler*)res.pData;
                vkImageInfos[cursor] = {};
                vkImageInfos[cursor].imageView = VK_NULL_HANDLE;
                vkImageInfos[cursor].imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                vkImageInfos[cursor].sampler = pSampler->vkSampler;
                vkWrites[i].pImageInfo = &vkImageInfos[cursor];
                cursor++;
            } break;
            default: ASSERTF(0, "Unsupported descriptor type");
        }
    }

    vkUpdateDescriptorSets(
            pRenderer->mVkDevice, 
            count, 
            &vkWrites[0], 
            0, NULL);
}

void removeResourceSet(Renderer* pRenderer, ShaderResourceSet** ppSet)
{
    ASSERT(pRenderer && ppSet);
    ASSERT(*ppSet);

    **ppSet = {};

    vkDestroyDescriptorSetLayout(
            pRenderer->mVkDevice,
            (*ppSet)->mVkLayout,
            NULL);

    // TODO_DW: Descriptor sets are freed with the pool. I might need to
    // free and recreate the descriptor pool on shader reload.

    poolFree(&pRenderer->poolResourceSets, *ppSet);
    *ppSet = NULL;
}
