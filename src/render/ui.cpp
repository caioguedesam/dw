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

    // Descriptor map (to associate Texture* with corresponding ImGui descriptor set)
    pUI->mVkDescriptors = hashmap<Texture*, VkDescriptorSet>(&desc.pApp->mAppArena, 128);

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

bool uiStartWindow(String windowTitle, float x, float y, float w, float h)
{
    ImVec2 displaySize = ImGui::GetIO().DisplaySize;

    if(x < 0) x = displaySize.x + x;
    if(y < 0) y = displaySize.y + y;

    if(w == 0) w = displaySize.x;
    if(h == 0) h = displaySize.y;

    ImGui::SetNextWindowPos(ImVec2(x, y));
    ImGui::SetNextWindowSize(ImVec2(w, h));
    return ImGui::Begin(cstr(windowTitle));
}

void uiEndWindow()
{
    ImGui::End();
}

bool uiStartTabBar(String tabBarTitle)
{
    return ImGui::BeginTabBar(cstr(tabBarTitle));
}

void uiEndTabBar()
{
    ImGui::EndTabBar();
}

bool uiStartTab(String tabTitle)
{
    return ImGui::BeginTabItem(cstr(tabTitle));
}

void uiEndTab()
{
    ImGui::EndTabItem();
}

bool uiIsHovered(bool delay)
{
    ImGuiHoveredFlags flags = delay
        ? ImGuiHoveredFlags_DelayNormal
        : ImGuiHoveredFlags_None;
    return ImGui::IsItemHovered(flags);
}

void uiSeparator()
{
    ImGui::Separator();
}

void uiSeparator(String name)
{
    ImGui::SeparatorText(cstr(name));
}

void uiSameLine()
{
    ImGui::SameLine();
}

void uiText(String text)
{
    ImGui::Text("%s", cstr(text));
}

void uiImage(UIState* pUI, Texture* pTexture, Sampler* pSampler, uint32 w, uint32 h)
{
    ASSERT(pUI);
    VkDescriptorSet vkDescriptorSet;
    if(!pUI->mVkDescriptors.contains(pTexture))
    {
        // TODO_DW: This technically should be using a single hash
        // for both textures and samplers.
        vkDescriptorSet = ImGui_ImplVulkan_AddTexture(pSampler->vkSampler, 
                pTexture->mVkImageView, 
                (VkImageLayout)IMAGE_LAYOUT_SHADER_READ_ONLY);
        pUI->mVkDescriptors.insert(pTexture, vkDescriptorSet);
    }
    else
    {
        vkDescriptorSet = pUI->mVkDescriptors[pTexture];
    }
    ImGui::Image((ImTextureID)vkDescriptorSet, ImVec2(w, h));
}

bool uiButton(String label, uint32 w, uint32 h)
{
    return ImGui::Button(cstr(label), ImVec2(w, h));
}

void uiCheckbox(String label, bool* pOut)
{
    ImGui::Checkbox(cstr(label), pOut);
}

void uiColor3f(String label, float* pOut)
{
    ImGui::ColorEdit3(cstr(label), pOut, ImGuiColorEditFlags_NoAlpha);
}

void uiColor4f(String label, float* pOut)
{
    ImGui::ColorEdit4(cstr(label), pOut);
}

void uiDragf(String label, float* pOut, float speed, float start, float end)
{
    ImGui::DragFloat(cstr(label), pOut, speed, start, end);
}

void uiDragi(String label, int32* pOut, float speed, int32 start, int32 end)
{
    ImGui::DragInt(cstr(label), pOut, speed, start, end);
}

void uiSliderf(String label, float* pOut, float start, float end, bool logarithmic)
{
    ImGuiSliderFlags flags = logarithmic
        ? ImGuiSliderFlags_Logarithmic
        : ImGuiSliderFlags_None;
    ImGui::SliderFloat(cstr(label), pOut, start, end, "%.3f", flags);
}

void uiSlideri(String label, int32* pOut, int32 start, int32 end, bool logarithmic)
{
    ImGuiSliderFlags flags = logarithmic
        ? ImGuiSliderFlags_Logarithmic
        : ImGuiSliderFlags_None;
    ImGui::SliderInt(cstr(label), pOut, start, end, "%.3f", flags);
}

void uiSlider2f(String label, float* pOut, float start, float end)
{
    ImGui::SliderFloat2(cstr(label), pOut, start, end);
}

void uiSlider3f(String label, float* pOut, float start, float end)
{
    ImGui::SliderFloat3(cstr(label), pOut, start, end);
}

void uiSliderAngle(String label, float* pOut)
{
    ImGui::SliderAngle(cstr(label), pOut);
}

void uiTooltip(String text)
{
    ImGui::SetTooltip("%s", cstr(text));
}

bool uiTreeNode(String name)
{
    return ImGui::TreeNode(cstr(name));
}

void uiTreePop()
{
    ImGui::TreePop();
}

void uiDemo()
{
    ImGui::ShowDemoWindow();
}

