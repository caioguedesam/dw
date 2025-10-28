#include "ui.hpp"
#include "../core/debug.hpp"

#include "../third_party/imgui/imgui.h"
#include "../third_party/imgui/backends/imgui_impl_vulkan.h"
#include "../third_party/imgui/backends/imgui_impl_win32.h"
#include "src/render/render.hpp"
#include "vulkan/vulkan_core.h"

void initUI(UIDesc desc, UIState* pUI)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // ImGui Win32
    ImGuiIO& io = ImGui::GetIO();
    RECT clientRect;
    GetWindowRect(desc.pApp->mWindow.mWinHandle, &clientRect);
    io.DisplaySize =
        ImVec2((float)(clientRect.right - clientRect.left),
               (float)(clientRect.bottom - clientRect.top));

    ImGui_ImplWin32_Init(desc.pApp->mWindow.mWinHandle);

    // ImGui Vulkan
    VkDescriptorPoolSize poolSizes[] =
    {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };
    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.poolSizeCount = ARR_LEN(poolSizes);
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = 1000;
    VkDescriptorPool vkDescriptorPool;
    VkResult ret = vkCreateDescriptorPool(desc.pRenderer->mVkDevice, &poolInfo, NULL, &vkDescriptorPool);
    ASSERTVK(ret);

    // TODO_DW: How to deal with this?
    VkFormat imageFormat = (VkFormat)desc.mTargetFormat;
    ImGui_ImplVulkan_InitInfo initInfo = {};
    initInfo.Instance = desc.pRenderer->mVkInstance;
    initInfo.PhysicalDevice = desc.pRenderer->mVkPhysicalDevice;
    initInfo.Device = desc.pRenderer->mVkDevice;
    initInfo.QueueFamily = desc.pRenderer->mVkQueueFamily;
    initInfo.Queue = desc.pRenderer->mVkQueue;
    initInfo.DescriptorPool = vkDescriptorPool;
    initInfo.MinImageCount = desc.pRenderer->mSwapChain.mImageCount;
    initInfo.ImageCount = desc.pRenderer->mSwapChain.mImageCount;
    initInfo.UseDynamicRendering = true;
    initInfo.PipelineInfoMain.PipelineRenderingCreateInfo = {};
    initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats =
        &imageFormat;
    initInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    ImGui_ImplVulkan_Init(&initInfo);

    // Style
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowBorderSize = 1;
    style.TabBorderSize = 1;
    style.TabRounding = 0;
    style.FrameBorderSize = 1;
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
    ImGui::StyleColorsClassic();

    pUI->mDesc = desc;
    pUI->mVkDescriptorPool = vkDescriptorPool;
}

void destroyUI(UIState* pUI)
{
    ASSERT(pUI);
    waitForCommands(pUI->mDesc.pRenderer);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    vkDestroyDescriptorPool(pUI->mDesc.pRenderer->mVkDevice, pUI->mVkDescriptorPool, NULL);

    *pUI = {};
}

void uiStartFrame()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void uiEndFrame(CommandBuffer* pCmd, RenderTargetBindDesc bindDesc)
{
    cmdBindRenderTargets(pCmd, bindDesc);
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), pCmd->mVkCmd);
    cmdUnbindRenderTargets(pCmd);
}

void uiDemo()
{
    ImGui::ShowDemoWindow();
}
