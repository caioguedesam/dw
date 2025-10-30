// Vulkan Memory Allocator
#define VMA_IMPLEMENTATION
#include "vma/vk_mem_alloc.h"

// ImGui
#include "third_party/imgui/imgui.cpp"
#include "third_party/imgui/imgui_draw.cpp"
#include "third_party/imgui/imgui_tables.cpp"
#include "third_party/imgui/imgui_widgets.cpp"
#include "third_party/imgui/imgui_demo.cpp"
#include "third_party/imgui/backends/imgui_impl_win32.cpp"
#include "third_party/imgui/backends/imgui_impl_vulkan.cpp"

// Tracy profiler
#if DW_PROFILE
#include "third_party/tracy/public/TracyClient.cpp"
#endif

// ImPlot
#include "third_party/implot/implot.cpp"
#include "third_party/implot/implot_demo.cpp"
#include "third_party/implot/implot_items.cpp"

