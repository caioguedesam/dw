#include "asset.hpp"
#include "../render/shader.hpp"
#include "../render/render.hpp"
#include "../core/debug.hpp"
#include "../third_party/slang/slang.h"
#include "../core/memory.hpp"

struct ShaderCompiler
{
    slang::IGlobalSession* pGlobalSession = NULL;
};

ShaderCompiler gShaderCompiler = {};

void initShaderCompiler()
{
    SlangGlobalSessionDesc desc = {};
    desc.minLanguageVersion = SLANG_LANGUAGE_VERSION_2026;
    SlangResult result = slang_createGlobalSession2(&desc, &gShaderCompiler.pGlobalSession);
    ASSERT(!SLANG_FAILED(result));
}

void destroyShaderCompiler()
{
    slang_shutdown();
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

void pushMacro(Array<slang::PreprocessorMacroDesc>* pArr, String macro)
{
    pArr->push({cstr(macro), "1"});
}

#define SHADER_SPIRV_PRINT_OUTPUT 0

void loadShader(AssetManager* pAssetManager, Renderer* pRenderer, 
        String fileName, uint32 shaderType, 
        String* pDefines, uint32 definesCount, 
        Shader** ppOut)
{
    ASSERT(pAssetManager && pRenderer && ppOut);
    ASSERT(*ppOut == NULL);

    ShaderType type = (ShaderType)shaderType;

    slang::SessionDesc sessionDesc = {};
    slang::TargetDesc targetDesc = {};

#if SHADER_SPIRV_PRINT_OUTPUT
    targetDesc.format = SLANG_SPIRV_ASM;
#else
    targetDesc.format = SLANG_SPIRV;
#endif
    targetDesc.profile = gShaderCompiler.pGlobalSession->findProfile("sm_6_6");
    sessionDesc.targets = &targetDesc;
    sessionDesc.targetCount = 1;
    const char* searchPaths[] = { "../../res/shaders/" };
    sessionDesc.searchPaths = searchPaths;
    sessionDesc.searchPathCount = 1;
    sessionDesc.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR;

    // Macros
    Array<slang::PreprocessorMacroDesc> macros = array<slang::PreprocessorMacroDesc>(&pAssetManager->mArenaTemp, 128);
    for(uint32 i = 0; i < definesCount; i++)
    {
        pushMacro(&macros, pDefines[i]);
    }
    sessionDesc.preprocessorMacroCount = macros.mCount;
    sessionDesc.preprocessorMacros = macros.mData;

    // Compile options
    Array<slang::CompilerOptionEntry> options = array<slang::CompilerOptionEntry>(&pAssetManager->mArenaTemp, 128);
    slang::CompilerOptionEntry entry = {};
    entry.name = slang::CompilerOptionName::VulkanUseEntryPointName;
    entry.value.intValue0 = 1;
    options.push(entry);
    entry.name = slang::CompilerOptionName::Capability;
    entry.value.kind = slang::CompilerOptionValueKind::String;
    entry.value.stringValue0 = "spvDescriptorHeapEXT";
    options.push(entry);
    entry.name = slang::CompilerOptionName::SPIRVUnifiedDescriptorHeapStride;
    entry.value.intValue0 = 1;
    options.push(entry);
    entry.name = slang::CompilerOptionName::ForceCLayout;
    entry.value.intValue0 = 1;
    options.push(entry);
#if DW_DEBUG
    entry.name = slang::CompilerOptionName::DebugInformation;
    entry.value.intValue0 = SLANG_DEBUG_INFO_LEVEL_MAXIMAL;
    options.push(entry);
    entry.name = slang::CompilerOptionName::Optimization;
    entry.value.intValue0 = SLANG_OPTIMIZATION_LEVEL_NONE;
    options.push(entry);
#else
    entry.name = slang::CompilerOptionName::DebugInformation;
    entry.value.intValue0 = SLANG_DEBUG_INFO_LEVEL_NONE;
    options.push(entry);
    entry.name = slang::CompilerOptionName::Optimization;
    entry.value.intValue0 = SLANG_OPTIMIZATION_LEVEL_MAXIMAL;
    options.push(entry);
#endif
    sessionDesc.compilerOptionEntryCount = options.mCount;
    sessionDesc.compilerOptionEntries = options.mData;

    slang::ISession* pSession = NULL;
    SlangResult result = gShaderCompiler.pGlobalSession->createSession(sessionDesc, &pSession);
    ASSERT(!SLANG_FAILED(result));

    slang::IBlob* pDiag = NULL;
    slang::IModule* pModule = pSession->loadModule(cstr(fileName), &pDiag);
    if(pDiag)
    {
        LOGLF("SHADER COMPILE", "%s", pDiag->getBufferPointer());
    }
    ASSERT(pModule);
    slang::IEntryPoint* pEntry = NULL;
    switch(type)
    {
        case SHADER_TYPE_VERT: pModule->findEntryPointByName("VSMain", &pEntry); break;
        case SHADER_TYPE_FRAG: pModule->findEntryPointByName("PSMain", &pEntry); break;
        case SHADER_TYPE_COMP: pModule->findEntryPointByName("CSMain", &pEntry); break;
        default: ASSERTF(0, "Unsupported shader type for shader %s", cstr(fileName));
    }
    ASSERT(pEntry);

    // Compose module and entry point into program
    slang::IComponentType* pProgram = NULL;
    slang::IComponentType* components[] = { pModule, pEntry };
    pSession->createCompositeComponentType(components, 2, &pProgram);
    ASSERT(pProgram);

    // Linking program to resolve cross-module references (not really using module functionality for dw yet)
    slang::IComponentType* pLinkedProgram = NULL;
    ISlangBlob* pLinkDiag = NULL;
    pProgram->link(&pLinkedProgram, &pLinkDiag);
    if(pLinkDiag)
    {
        LOGLF("SHADER LINK", "%s", pLinkDiag->getBufferPointer());
    }
    ASSERT(pLinkedProgram);

    // Generating kernel code which is passed to vulkan renderer
    slang::IBlob* pKernelBlob = NULL;
    pLinkedProgram->getEntryPointCode(0, 0, &pKernelBlob, &pDiag);
    if(pDiag)
    {
        LOGLF("SHADER KERNEL GEN", "%s", pDiag->getBufferPointer());
    }
    ASSERT(pKernelBlob);

#if SHADER_SPIRV_PRINT_OUTPUT

    LOGF("%d", pKernelBlob->getBufferSize());
    const char* spirvAsmText = (const char*)(pKernelBlob->getBufferPointer());
    LOGF("%s", spirvAsmText);
    ASSERT(0);

#else

    uint64 bytecodeLen = pKernelBlob->getBufferSize();
    byte* bytecode = (byte*)pKernelBlob->getBufferPointer();
    
    ShaderDesc desc = {};
    desc.mType = type;
    desc.mBytecodeSize = bytecodeLen;
    desc.pBytecode = (uint32*)bytecode;
    addShader(pRenderer, desc, ppOut);
#endif

    arenaClear(&pAssetManager->mArenaTemp);
}
