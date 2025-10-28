#pragma once
#include "../core/app.hpp"
#include "render.hpp"

struct UIDesc
{
    App* pApp = NULL;
    Renderer* pRenderer = NULL;

    ImageFormat mTargetFormat = FORMAT_UNDEFINED;
};

struct UIState
{
    UIDesc mDesc = {};

    // ImGui Vulkan context
    VkDescriptorPool mVkDescriptorPool = VK_NULL_HANDLE;
};

void initUI(UIDesc desc, UIState* pUI);
void destroyUI(UIState* pUI);

void uiStartFrame();
void uiEndFrame(CommandBuffer* pCmd, RenderTargetBindDesc bindDesc);

void uiDemo();
