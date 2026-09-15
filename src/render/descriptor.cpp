#include "descriptor.hpp"
#include "render.hpp"
#include "buffer.hpp"
#include "texture.hpp"
#include "../core/base.hpp"
#include "../core/debug.hpp"
#include "vma/vk_mem_alloc.h"
#include "vulkan/vulkan_core.h"

void initResourceSet(Renderer* pRenderer, 
        Texture** ppTextures, uint32 textureCount,
        Sampler** ppSamplers, uint32 samplerCount,
        Buffer* pAddrBuffer)
{
    pRenderer->mResourceSet = {};

    // Texture types
    struct TextureDescType
    {
        TextureUsage type;
        VkDescriptorType vkType;
    };
    TextureDescType vkTextureDescTypes[] =
    {
        {TEXTURE_USAGE_SAMPLED, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE},   // Texture2D<float4>
        {TEXTURE_USAGE_STORAGE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE},   // RWTexture2D<float4>
        {TEXTURE_USAGE_STORAGE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE},   // RWTexture2D<float>
    };
    uint32 textureTypeCount = ARR_LEN(vkTextureDescTypes);

    // Textures + Samplers + Resource address buffer
    uint32 bindingCount = textureTypeCount + 2;
    VkDescriptorSetLayoutBinding    vkBindings[bindingCount];
    VkDescriptorBindingFlags        vkBindingFlags[bindingCount];

    uint32 cursor = 0;

    // Resource address buffer
    vkBindings[cursor] = {};
    vkBindings[cursor].binding = cursor;
    vkBindings[cursor].stageFlags = VK_SHADER_STAGE_ALL;
    vkBindings[cursor].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    vkBindings[cursor].descriptorCount = 1;
    vkBindingFlags[cursor] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
    cursor++;

    // Samplers
    vkBindings[cursor] = {};
    vkBindings[cursor].binding = cursor;
    vkBindings[cursor].stageFlags = VK_SHADER_STAGE_ALL;
    vkBindings[cursor].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    vkBindings[cursor].descriptorCount = MAX_SAMPLER_DESCRIPTORS;
    vkBindingFlags[cursor] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
    cursor++;

    // Textures
    for(uint32 i = 0; i < textureTypeCount; i++)
    {
        vkBindings[cursor] = {};
        vkBindings[cursor].binding = cursor;
        vkBindings[cursor].stageFlags = VK_SHADER_STAGE_ALL;
        vkBindings[cursor].descriptorCount = MAX_TEXTURE_DESCRIPTORS;
        vkBindings[cursor].descriptorType = vkTextureDescTypes[i].vkType;
        vkBindingFlags[cursor] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
        cursor++;
    }

    VkDescriptorSetLayoutBindingFlagsCreateInfo flagsInfo = {};
    flagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    flagsInfo.bindingCount = bindingCount;
    flagsInfo.pBindingFlags = vkBindingFlags;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = bindingCount;
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

    pRenderer->mResourceSet.mVkLayout = vkLayout;
    pRenderer->mResourceSet.mVkSet = vkSet;

    cursor = 0;
    // Writing resource address buffer to descriptor set
    VkDescriptorBufferInfo  vkAddrBufferInfo = {};
    vkAddrBufferInfo.buffer = pAddrBuffer->mVkBuffer;
    vkAddrBufferInfo.offset = 0;
    vkAddrBufferInfo.range = pAddrBuffer->mDesc.mSize;
    VkWriteDescriptorSet vkBufferDescWrite = {};
    vkBufferDescWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    vkBufferDescWrite.dstSet = vkSet;
    vkBufferDescWrite.dstBinding = cursor;
    vkBufferDescWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    vkBufferDescWrite.descriptorCount = 1;
    vkBufferDescWrite.pBufferInfo = &vkAddrBufferInfo;
    vkUpdateDescriptorSets(pRenderer->mVkDevice, 1, &vkBufferDescWrite, 0, NULL);
    cursor++;

    // Writing all samplers to descriptor set
    VkWriteDescriptorSet vkSamplerDescWrite = {};
    vkSamplerDescWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    vkSamplerDescWrite.dstSet = vkSet;
    vkSamplerDescWrite.dstBinding = cursor;
    vkSamplerDescWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    vkSamplerDescWrite.descriptorCount = samplerCount;

    VkDescriptorImageInfo vkSamplerInfos[samplerCount];
    for(uint32 i = 0; i < samplerCount; i++)
    {
        //Sampler* pSampler = &pSamplers[i];
        Sampler* pSampler = ppSamplers[i];
        ASSERT(pSampler);

        vkSamplerInfos[i] = {};
        vkSamplerInfos[i].imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        vkSamplerInfos[i].imageView = VK_NULL_HANDLE;
        vkSamplerInfos[i].sampler = pSampler->vkSampler;
    }
    vkSamplerDescWrite.pImageInfo = vkSamplerInfos;
    vkUpdateDescriptorSets(pRenderer->mVkDevice, 1, &vkSamplerDescWrite, 0, NULL);
    cursor++;

    // Writing all textures to descriptor set (each texture is written once for each type)
    //VkWriteDescriptorSet vkImageDescWrites[textureTypeCount];
    for(uint32 type = 0; type < textureTypeCount; type++)
    {
        VkWriteDescriptorSet vkImageDescWrite = {};
        vkImageDescWrite = {};
        vkImageDescWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        vkImageDescWrite.dstSet = vkSet;
        vkImageDescWrite.dstBinding = cursor;
        vkImageDescWrite.descriptorType = vkTextureDescTypes[type].vkType;
        vkImageDescWrite.descriptorCount = textureCount;

        TextureUsage typeUsage = vkTextureDescTypes[type].type;

        VkDescriptorImageInfo vkImageInfos[textureCount];
        for(uint32 i = 0; i < textureCount; i++)
        {
            //Texture* pTexture = &pTextures[i];
            Texture* pTexture = ppTextures[i];

            if(!pTexture || !(pTexture->mDesc.mUsage & typeUsage))
            {
                // Resort to fallback 0 if null or the texture is not of the correct type for the binding.
                // Fallback 0 must be both sampled and storage.
                pTexture = ppTextures[0];
                ASSERT(pTexture);
            }

            vkImageInfos[i] = {};
            vkImageInfos[i].imageView = pTexture->mVkImageView;
            vkImageInfos[i].imageLayout = (VkImageLayout)pTexture->mDesc.mBaseLayout;
            vkImageInfos[i].sampler = VK_NULL_HANDLE;
        }

        vkImageDescWrite.pImageInfo = vkImageInfos;
        cursor++;
        vkUpdateDescriptorSets(pRenderer->mVkDevice, 1, &vkImageDescWrite, 0, NULL);
    }

    VkPipelineLayoutCreateInfo vkPipeLayoutInfo = {};
    vkPipeLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    vkPipeLayoutInfo.setLayoutCount = 1;
    vkPipeLayoutInfo.pSetLayouts = &vkLayout;

    // Only use a single push constant range with the minimum max size available by vulkan specs (128 bytes).
    // Constants are set before binding in a generic manner.
    vkPipeLayoutInfo.pushConstantRangeCount = 1;
    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_ALL;
    pushConstantRange.offset = 0;
    pushConstantRange.size = MAX_SHADER_CONSTANT_SIZE;
    vkPipeLayoutInfo.pPushConstantRanges = &pushConstantRange;
    VkPipelineLayout vkPipeLayout;
    ret = vkCreatePipelineLayout(pRenderer->mVkDevice,
            &vkPipeLayoutInfo,
            NULL,
            &vkPipeLayout);

    pRenderer->mResourceSet.mVkPipelineBindingLayout = vkPipeLayout;
}

void destroyResourceSet(Renderer* pRenderer)
{
    vkDestroyPipelineLayout(
            pRenderer->mVkDevice, 
            pRenderer->mResourceSet.mVkPipelineBindingLayout, 
            NULL);

    vkDestroyDescriptorSetLayout(
            pRenderer->mVkDevice,
            pRenderer->mResourceSet.mVkLayout,
            NULL);

    // At this point, all descriptors are invalid. Pool is reset to allocate more.
    vkResetDescriptorPool(
            pRenderer->mVkDevice, 
            pRenderer->mVkDescriptorPool,
            0);

    pRenderer->mResourceSet = {};
}

#if 0
void initDescriptorHeap(Renderer* pRenderer, DescriptorHeap* pHeap)
{
    ASSERT(pRenderer && pHeap);
    ASSERT(pHeap != NULL);

    *pHeap = {};

    VkPhysicalDeviceDescriptorHeapPropertiesEXT heapProps = {};
    heapProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_HEAP_PROPERTIES_EXT;

    VkPhysicalDeviceProperties2 props2 = {};
    props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    props2.pNext = &heapProps;

    vkGetPhysicalDeviceProperties2(pRenderer->mVkPhysicalDevice, &props2);

    uint64 resourceDescriptorSize = MAX(heapProps.bufferDescriptorSize, heapProps.imageDescriptorSize);
    uint64 resourceDescriptorAlignment = MAX(heapProps.bufferDescriptorAlignment, heapProps.imageDescriptorAlignment);
    uint64 resourceDescriptorStride = ALIGN_TO(resourceDescriptorSize, resourceDescriptorAlignment);

    uint64 samplerDescriptorSize = heapProps.samplerDescriptorSize;
    uint64 samplerDescriptorAlignment = heapProps.samplerDescriptorAlignment;
    uint64 samplerDescriptorStride = ALIGN_TO(samplerDescriptorSize, samplerDescriptorAlignment);

    uint64 resourceReservedSize = ALIGN_TO(heapProps.minResourceHeapReservedRange, resourceDescriptorAlignment);
    uint64 resourceDescriptorHeapSize = RESOURCE_HEAP_MAX_DESCRIPTORS * resourceDescriptorStride;
    uint64 resourceDescriptorHeapEnd = resourceReservedSize + resourceDescriptorHeapSize;

    uint64 samplerHeapOffset = ALIGN_TO(resourceDescriptorHeapEnd, heapProps.samplerHeapAlignment);
    uint64 samplerReservedSize = ALIGN_TO(heapProps.minSamplerHeapReservedRange, samplerDescriptorAlignment);
    uint64 samplerDescriptorHeapSize = RESOURCE_HEAP_MAX_SAMPLERS * samplerDescriptorStride;
    uint64 totalHeapSize = samplerHeapOffset + samplerReservedSize + samplerDescriptorHeapSize;
    totalHeapSize = ALIGN_TO(totalHeapSize, heapProps.samplerHeapAlignment);

#if 0
    LOGF("Buffer Descriptor size           = %llu", heapProps.bufferDescriptorSize);
    LOGF("Buffer Descriptor alignment      = %llu", heapProps.bufferDescriptorAlignment);
    LOGF("Image Descriptor size            = %llu", heapProps.imageDescriptorSize);
    LOGF("Image Descriptor alignment       = %llu", heapProps.imageDescriptorAlignment);
    LOGF("Resource Descriptor size         = %llu", resourceDescriptorSize);
    LOGF("Resource Descriptor alignment    = %llu", resourceDescriptorAlignment);
    LOGF("Resource Descriptor stride       = %llu", resourceDescriptorStride);
#endif
    
    BufferDesc desc = {};
    desc.mType = BUFFER_TYPE_DESCRIPTOR_HEAP;
    desc.mSize = totalHeapSize;
    desc.mAlign = MAX(heapProps.resourceHeapAlignment, heapProps.samplerHeapAlignment);
    addBuffer(pRenderer, desc, &pHeap->pHeapBuffer);

    VkBufferDeviceAddressInfo addrInfo = {};
    addrInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    addrInfo.buffer = pHeap->pHeapBuffer->mVkBuffer;
    VkDeviceAddress vkAddress = vkGetBufferDeviceAddress(pRenderer->mVkDevice, &addrInfo);

    pHeap->mVkBindInfoResources = {};
    pHeap->mVkBindInfoResources.sType = VK_STRUCTURE_TYPE_BIND_HEAP_INFO_EXT;
    pHeap->mVkBindInfoResources.heapRange.address = vkAddress;
    pHeap->mVkBindInfoResources.heapRange.size = resourceDescriptorHeapEnd;
    pHeap->mVkBindInfoResources.reservedRangeOffset = 0;
    pHeap->mVkBindInfoResources.reservedRangeSize = resourceReservedSize;

    pHeap->mVkBindInfoSamplers = {};
    pHeap->mVkBindInfoSamplers.sType = VK_STRUCTURE_TYPE_BIND_HEAP_INFO_EXT;
    pHeap->mVkBindInfoSamplers.heapRange.address = vkAddress + samplerHeapOffset;
    pHeap->mVkBindInfoSamplers.heapRange.size = totalHeapSize - samplerHeapOffset;
    pHeap->mVkBindInfoSamplers.reservedRangeOffset = 0;
    pHeap->mVkBindInfoSamplers.reservedRangeSize = samplerReservedSize;

    pHeap->mResourceDescriptorSize = resourceDescriptorSize;
    pHeap->mResourceDescriptorAlign = resourceDescriptorAlignment;
    pHeap->mResourceDescriptorStride = resourceDescriptorStride;
    pHeap->mSamplerDescriptorSize = samplerDescriptorSize;
    pHeap->mSamplerDescriptorAlign = samplerDescriptorAlignment;
    pHeap->mSamplerDescriptorStride = samplerDescriptorStride;
    pHeap->mSamplerHeapOffset = samplerHeapOffset;
    pHeap->mResourceHeapReservedSize = resourceReservedSize;
    pHeap->mSamplerHeapReservedSize = samplerReservedSize;

    // Loading function pointers
    pHeap->pfn_vkWriteResourceDescriptorsEXT = 
        (PFN_vkWriteResourceDescriptorsEXT)vkGetDeviceProcAddr(pRenderer->mVkDevice, "vkWriteResourceDescriptorsEXT");
    pHeap->pfn_vkWriteSamplerDescriptorsEXT = 
        (PFN_vkWriteSamplerDescriptorsEXT)vkGetDeviceProcAddr(pRenderer->mVkDevice, "vkWriteSamplerDescriptorsEXT");
}

void destroyDescriptorHeap(Renderer* pRenderer, DescriptorHeap* pHeap)
{
    removeBuffer(pRenderer, &pHeap->pHeapBuffer);
    *pHeap = {};
}

HND addToDescriptorHeap(Renderer* pRenderer, Buffer* pBuffer)
{
    // TODO(caio): Having troubles with uniform/constant buffer support.
    // Remove this after first implementation is done
    ASSERT(pBuffer->mDesc.mType != BUFFER_TYPE_UNIFORM);

    DescriptorHeap* pHeap = &pRenderer->mDescriptorHeap;
    // TODO(caio): Stop mapping here after moving heap memory to device local
    void* mappedHeap = mapBufferMemory(pRenderer, pRenderer->mDescriptorHeap.pHeapBuffer);
    uint8* descriptorAddr = (uint8*)mappedHeap 
                            + pHeap->mResourceHeapReservedSize
                            + pHeap->mResourceDescriptorStride * pHeap->mResourceDescriptorCount;
    ASSERT(IS_ALIGNED(descriptorAddr, pHeap->mResourceDescriptorAlign));

    VkHostAddressRangeEXT dest = {};
    dest.address = descriptorAddr;
    dest.size = pHeap->mResourceDescriptorSize;

    VkBufferDeviceAddressInfo addrInfo = {};
    addrInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    addrInfo.buffer = pBuffer->mVkBuffer;
    VkDeviceAddress vkAddress = vkGetBufferDeviceAddress(pRenderer->mVkDevice, &addrInfo);

    VkDeviceAddressRangeKHR vkRange = {};
    vkRange.address = vkAddress;
    vkRange.size = pBuffer->mDesc.mSize;

    VkResourceDescriptorInfoEXT descriptorInfo = {};
    descriptorInfo.sType = VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT;
    // TODO(caio): Having troubles with uniform/constant buffer support.
    // Change this after first implementation is done
    descriptorInfo.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorInfo.data.pAddressRange = &vkRange;

    pHeap->pfn_vkWriteResourceDescriptorsEXT(pRenderer->mVkDevice, 1, &descriptorInfo, &dest);

    HND result = pHeap->mResourceDescriptorCount;
    pHeap->mResourceDescriptorCount++;

    unmapBufferMemory(pRenderer, pRenderer->mDescriptorHeap.pHeapBuffer);

    return result;
}

HND addToDescriptorHeap(Renderer* pRenderer, Texture* pTexture, bool readWrite)
{
    DescriptorHeap* pHeap = &pRenderer->mDescriptorHeap;
    // TODO(caio): Stop mapping here after moving heap memory to device local
    void* mappedHeap = mapBufferMemory(pRenderer, pRenderer->mDescriptorHeap.pHeapBuffer);
    uint8* descriptorAddr = (uint8*)mappedHeap 
                            + pHeap->mResourceHeapReservedSize
                            + pHeap->mResourceDescriptorStride * pHeap->mResourceDescriptorCount;
    ASSERT(IS_ALIGNED(descriptorAddr, pHeap->mResourceDescriptorAlign));

    VkHostAddressRangeEXT dest = {};
    dest.address = descriptorAddr;
    dest.size = pHeap->mResourceDescriptorSize;

    VkImageDescriptorInfoEXT imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_DESCRIPTOR_INFO_EXT;
    imageInfo.pView = &pTexture->mVkCreateInfo;
    imageInfo.layout = readWrite
        ? VK_IMAGE_LAYOUT_GENERAL
        : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkResourceDescriptorInfoEXT descriptorInfo = {};
    descriptorInfo.sType = VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT;
    descriptorInfo.type = readWrite
        ? VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
        : VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    descriptorInfo.data.pImage = &imageInfo;

    pHeap->pfn_vkWriteResourceDescriptorsEXT(pRenderer->mVkDevice, 1, &descriptorInfo, &dest);

    HND result = pHeap->mResourceDescriptorCount;
    pHeap->mResourceDescriptorCount++;

    unmapBufferMemory(pRenderer, pRenderer->mDescriptorHeap.pHeapBuffer);

    return result;
}

HND addToDescriptorHeap(Renderer* pRenderer, Sampler* pSampler)
{
    DescriptorHeap* pHeap = &pRenderer->mDescriptorHeap;
    // TODO(caio): Stop mapping here after moving heap memory to device local
    void* mappedHeap = mapBufferMemory(pRenderer, pRenderer->mDescriptorHeap.pHeapBuffer);
    uint8* descriptorAddr = (uint8*)mappedHeap 
                            + pHeap->mSamplerHeapOffset
                            + pHeap->mSamplerHeapReservedSize
                            + pHeap->mSamplerDescriptorStride * pHeap->mSamplerDescriptorCount;
    ASSERT(IS_ALIGNED(descriptorAddr, pHeap->mSamplerDescriptorAlign));

    VkHostAddressRangeEXT dest = {};
    dest.address = descriptorAddr;
    dest.size = pHeap->mSamplerDescriptorSize;

    pHeap->pfn_vkWriteSamplerDescriptorsEXT(pRenderer->mVkDevice, 1, &pSampler->vkCreateInfo, &dest);
    
    HND result = pHeap->mSamplerDescriptorCount;
    pHeap->mSamplerDescriptorCount++;

    unmapBufferMemory(pRenderer, pRenderer->mDescriptorHeap.pHeapBuffer);

    return result;
}
#endif
