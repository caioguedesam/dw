#pragma once
#include "../core/app.hpp"
#include "../core/string.hpp"
#include "../math/math.hpp"
#include "../core/hash_map.hpp"
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

    HashMap<Texture*, VkDescriptorSet> mVkDescriptors;

    // ImGui Vulkan context
    VkDescriptorPool mVkDescriptorPool = VK_NULL_HANDLE;
};

void initUI(UIDesc desc, UIState* pUI);
void destroyUI(UIState* pUI);

void uiStartFrame();
void uiEndFrame(CommandBuffer* pCmd, RenderTargetBindDesc bindDesc);

bool uiStartWindow(String windowTitle, float x, float y, float w, float h);
void uiEndWindow();
bool uiStartTabBar(String tabBarTitle);
void uiEndTabBar();
bool uiStartTab(String tabTitle);
void uiEndTab();

bool uiIsHovered(bool delay = false);

void uiSeparator();
void uiSeparator(String name);
void uiSameLine();
void uiText(String text);
void uiImage(UIState* pUI, Texture* pTexture, Sampler* pSampler, uint32 w, uint32 h);
bool uiButton(String label, uint32 w = 0, uint32 h = 0);
void uiCheckbox(String label, bool* pOut);
void uiInputf(String label, float* pOut);
void uiColor3f(String label, float* pOut);
void uiColor4f(String label, float* pOut);
void uiDragf(String label, float* pOut, float speed = 1.f, float start = 0, float end = 100);
void uiDragi(String label, int32* pOut, float speed = 1.f, int32 start = 0, int32 end = 100);
void uiSliderf(String label, float* pOut, float start, float end, bool logarithmic = false);
void uiSlideri(String label, int32* pOut, int32 start, int32 end, bool logarithmic = false);
void uiSlider2f(String label, float* pOut, float start, float end);
void uiSlider3f(String label, float* pOut, float start, float end);
void uiSliderAngle(String label, float* pOut);
void uiTooltip(String text);
bool uiTreeNode(String name);
void uiTreePop();
void uiDemo();

#define UI_MAX_LINES_PER_PLOT 8
struct UILinePlotDesc
{
    v2f mSize       = {0, 0};   // -1 fills axis
    v2f mMinLimit   = {0, 0};
    v2f mMaxLimit   = {0, 0};
    v4f mColor      = {0, 0, 0, -1};    // Default value for IMPLOT_AUTO_COL
    bool mShaded    = false;

    float* mDataX[UI_MAX_LINES_PER_PLOT];
    float* mDataY[UI_MAX_LINES_PER_PLOT];
    uint32 mLinePointCount = 0;
    uint32 mLineCount = 0;

    // TODO_DW: Label controls for axes
};

void uiLinePlot(String name, UILinePlotDesc desc);
