#include "shader.hpp"
#include "render.hpp"
#include "../core/debug.hpp"

void addShader(Renderer* pRenderer, ShaderDesc desc, Shader** ppShader)
{
    ASSERT(pRenderer && ppShader);
    ASSERT(*ppShader == NULL);

    *ppShader = (Shader*)poolAlloc(&pRenderer->poolShaders);

    **ppShader = {};

    ASSERT(desc.mBytecodeSize && desc.pBytecode);
    VkShaderModuleCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = desc.mBytecodeSize;
    info.pCode = desc.pBytecode;

    VkShaderModule vkShader;
    VkResult ret = vkCreateShaderModule(
            pRenderer->mVkDevice,
            &info,
            NULL,
            &vkShader);
    ASSERTVK(ret);

    (*ppShader)->mDesc = desc;
    (*ppShader)->mVkShader = vkShader; 
}

void removeShader(Renderer* pRenderer, Shader** ppShader)
{
    ASSERT(pRenderer && ppShader);
    ASSERT(*ppShader);

    vkDestroyShaderModule(
            pRenderer->mVkDevice,
            (*ppShader)->mVkShader,
            NULL);

    **ppShader = {};

    poolFree(&pRenderer->poolShaders, ppShader);
    *ppShader = NULL;
}
