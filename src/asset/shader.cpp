#include "asset.hpp"
#include "../render/shader.hpp"
#include "../render/render.hpp"
#include "../core/file.hpp"
#include "../core/debug.hpp"

#include "shaderc/shaderc.h"
#include <unknwn.h>
#include "../third_party/dxc/dxcapi.h"
#include "../core/memory.hpp"

struct ShaderCompiler
{
    IDxcUtils* pDxcUtils = NULL;
    IDxcCompiler3* pDxcCompiler = NULL;
    IDxcIncludeHandler* pDxcIncludeHandler = NULL;
};

ShaderCompiler gShaderCompiler = {};

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

void initShaderCompiler()
{
    DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&gShaderCompiler.pDxcUtils));
    DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&gShaderCompiler.pDxcCompiler));
    gShaderCompiler.pDxcUtils->CreateDefaultIncludeHandler(&gShaderCompiler.pDxcIncludeHandler);
}

void destroyShaderCompiler()
{
    gShaderCompiler.pDxcUtils->Release();
    gShaderCompiler.pDxcCompiler->Release();
    gShaderCompiler.pDxcIncludeHandler->Release();

    gShaderCompiler = {};
}

void towcstr(Arena* pArena, String in, wchar_t** out)
{
    wchar_t* wName = (wchar_t*)arenaPush(pArena, (in.mLen + 1) * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, cstr(in), -1, wName, in.mLen);
    wName[in.mLen] = 0;
    *out = wName;
}

void addToArgBuffer(LPCWSTR* argBuffer, LPCWSTR arg, size_t* argCount)
{
    argBuffer[*argCount] = arg;
    (*argCount)++;
}

#define SHADER_USE_DXC 1
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

#if SHADER_USE_DXC

    DxcBuffer source = {};
    source.Ptr = code.mData;
    source.Size = code.mLen;
    source.Encoding = DXC_CP_UTF8;

    LPCWSTR target;
    LPCWSTR entry;
    ShaderType type = (ShaderType)shaderType;
    switch (type)
    {
        case SHADER_TYPE_VERT: target = L"vs_6_6"; entry = L"VSMain"; break;
        case SHADER_TYPE_FRAG: target = L"ps_6_6"; entry = L"PSMain"; break;
        case SHADER_TYPE_COMP: target = L"cs_6_6"; entry = L"CSMain"; break;
        default: ASSERTF(0, "Unsupported shader type for shader %s", cstr(path));
    }

    String assetDir = getFileDir(path, false);
    wchar_t* wAssetDir;
    towcstr(&pAssetManager->mArenaTemp, assetDir, &wAssetDir);

    // Construct argument buffer with parameters and defines
    size_t argCount = 0;
    LPCWSTR args[256];
    addToArgBuffer(args, L"-spirv", &argCount);
    addToArgBuffer(args, L"-fspv-target-env=vulkan1.3", &argCount);
    addToArgBuffer(args, L"-E", &argCount);
    addToArgBuffer(args, entry, &argCount);
    addToArgBuffer(args, L"-T", &argCount);
    addToArgBuffer(args, target, &argCount);
    addToArgBuffer(args, L"-I", &argCount);
    addToArgBuffer(args, wAssetDir, &argCount);
    addToArgBuffer(args, L"-Zpc", &argCount);
    addToArgBuffer(args, L"-fvk-use-scalar-layout", &argCount);
#if DW_DEBUG
    addToArgBuffer(args, L"-Zi", &argCount);
    addToArgBuffer(args, L"-Od", &argCount);
#else
    addToArgBuffer(args, L"-O3", &argCount);
#endif

    for(uint32 i = 0; i < definesCount; i++)
    {
        addToArgBuffer(args, L"-D", &argCount);
        wchar_t* wName;
        towcstr(&pAssetManager->mArenaTemp, pDefines[i], &wName);
        addToArgBuffer(args, wName, &argCount);
    }

    IDxcResult* pResult = NULL;
    HRESULT hr = gShaderCompiler.pDxcCompiler->Compile(&source, args, (uint32)argCount, gShaderCompiler.pDxcIncludeHandler, IID_PPV_ARGS(&pResult));
    if(FAILED(hr))
    {
        ASSERT("Failed to compile shader!");
    }

    IDxcBlobUtf8* pErrors = NULL;
    pResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), NULL);
    if(pErrors && pErrors->GetStringLength())
    {
        uint64 errorStrSize = pErrors->GetStringLength();
        const char* errorStr = pErrors->GetStringPointer();
        LOGLF("SHADER COMPILE", "%s", errorStr);
        ASSERT(0);
    }

    IDxcBlob* pSpirv = NULL;
    pResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pSpirv), NULL);

    uint64 bytecodeLen = pSpirv->GetBufferSize();
    byte* bytecode = (byte*)pSpirv->GetBufferPointer();

    ShaderDesc desc = {};
    desc.mType = type;
    desc.mBytecodeSize = bytecodeLen;
    desc.pBytecode = (uint32*)bytecode;
    addShader(pRenderer, desc, ppOut);

#else
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
    shaderc_compile_options_set_source_language(options, shaderc_source_language_hlsl);
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
#endif

    arenaClear(&pAssetManager->mArenaTemp);
}
