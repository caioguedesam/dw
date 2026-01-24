#include "asset.hpp"
#include "../render/shader.hpp"
#include "../render/render.hpp"
#include "../core/file.hpp"
#include "../core/debug.hpp"

#include "shaderc/shaderc.h"
#include "../core/memory.hpp"

shaderc_include_result* resolveInclude(void* pUserData, const char* requested, int32 requestType,
        const char* requesting, size_t includeDepth)
{
    ASSERT(requestType == shaderc_include_type_relative);
    Arena* pArena = (Arena*)pUserData;

    String assetDir = getFileDir(str(requesting), true);
    String assetName = join(pArena, assetDir, str(requested));
    String assetStr = readFileStr(pArena, assetName);

    shaderc_include_result result = {};
    result.source_name = cstr(assetName);
    result.source_name_length = assetName.mLen;
    result.content = cstr(assetStr);
    result.content_length = assetStr.mLen;

    shaderc_include_result* include = (shaderc_include_result*)arenaPush(pArena, sizeof(shaderc_include_result));
    memcpy(include, &result, sizeof(shaderc_include_result));

    return include;
}

void releaseInclude(void* pUserData, shaderc_include_result* pResult)
{
}

void loadShader(AssetManager* pAssetManager, Renderer* pRenderer, 
        String path, 
        uint32 shaderType, String* pDefines, uint32 definesCount, 
        Shader** ppOut)
{
    ASSERT(pAssetManager && pRenderer && ppOut);
    ASSERT(*ppOut == NULL);
    ASSERT(pathExists(path));

    // Shader bytecode doesn't need to persist, using temp arena.
    String code = readFileStr(&pAssetManager->mArenaTemp, path);

    ShaderType type = (ShaderType)shaderType;
    shaderc_shader_kind kind;
    if(type == SHADER_TYPE_VERT)
    {
        kind = shaderc_vertex_shader;
    }
    else if(type == SHADER_TYPE_FRAG)
    {
        kind = shaderc_fragment_shader;
    }
    else if(type == SHADER_TYPE_COMP)
    {
        kind = shaderc_compute_shader;
    }
    else
    {
        ASSERTF(0, "Unsupported shader type for shader %s", cstr(path));
    }

    shaderc_compiler_t compiler = shaderc_compiler_initialize();
    shaderc_compile_options_t options = shaderc_compile_options_initialize();
#if DW_DEBUG
    shaderc_compile_options_set_generate_debug_info(options);
    shaderc_compile_options_set_optimization_level(options, shaderc_optimization_level_zero);
#else
    shaderc_compile_options_set_optimization_level(options, shaderc_optimization_level_performance);
#endif
    shaderc_compile_options_set_include_callbacks(
            options, 
            resolveInclude, 
            releaseInclude, 
            &pAssetManager->mArenaTemp);

    // Defining type of shader
    String typeStr = {};
    if(type == SHADER_TYPE_VERT)
    {
        typeStr = str("VERTEX_SHADER");
    }
    if(type == SHADER_TYPE_FRAG)
    {
        typeStr = str("PIXEL_SHADER");
    }
    if(type == SHADER_TYPE_COMP)
    {
        typeStr = str("COMPUTE_SHADER");
    }
    shaderc_compile_options_add_macro_definition(
            options, 
            cstr(typeStr), 
            typeStr.mLen, 
            "1", 1);

    // Add user defined precompilation options
    // TODO(caio): Add support for custom preprocessor macros
    for(uint32 i = 0; i < definesCount; i++)
    {
        shaderc_compile_options_add_macro_definition(
                options, 
                cstr(pDefines[i]), 
                pDefines[i].mLen, 
                "1", 1);
    }

    shaderc_compilation_result_t compiled = shaderc_compile_into_spv(
            compiler,
            cstr(code),
            code.mLen,
            kind,
            cstr(path),
            "main",
            options);
    uint64 errorCount = shaderc_result_get_num_errors(compiled);
    if(errorCount)
    {
        LOGLF("SHADER COMPILE", "%s", shaderc_result_get_error_message(compiled));
        ASSERT(0);
    }

    // TODO_DW: Is there a way to hook arena with shaderc?
    uint64 bytecodeLen = shaderc_result_get_length(compiled);
    byte* bytecode = (byte*)shaderc_result_get_bytes(compiled);

    ShaderDesc desc = {};
    desc.mType = type;
    desc.mBytecodeSize = bytecodeLen;
    desc.pBytecode = (uint32*)bytecode;
    addShader(pRenderer, desc, ppOut);

    shaderc_result_release(compiled);
    shaderc_compile_options_release(options);
    shaderc_compiler_release(compiler);

    arenaClear(&pAssetManager->mArenaTemp);
}
