#pragma once
#include "../core/base.hpp"
#include "vulkan/vulkan_core.h"

struct Renderer;

// --------------------------------------
// Shader
enum ShaderType : uint32
{
    SHADER_TYPE_VERT = VK_SHADER_STAGE_VERTEX_BIT,
    SHADER_TYPE_FRAG = VK_SHADER_STAGE_FRAGMENT_BIT,
    SHADER_TYPE_COMP = VK_SHADER_STAGE_COMPUTE_BIT,
};

struct ShaderDesc
{
    ShaderType  mType;
    uint64      mBytecodeSize   = 0;
    uint32*     pBytecode       = NULL;
};

struct Shader
{
    ShaderDesc mDesc;
    
    VkShaderModule mVkShader = VK_NULL_HANDLE;
};

void addShader(Renderer* pRenderer, ShaderDesc desc, Shader** ppShader);
void removeShader(Renderer* pRenderer, Shader** ppShader);
