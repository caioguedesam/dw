#pragma once

#include "base.hpp"

enum AppLoadRequest : uint32
{
    LOAD_REQUEST_NONE               = 0,
    LOAD_REQUEST_SHADER             = BIT(0),
    LOAD_REQUEST_RENDER_TARGET      = BIT(1),
    LOAD_REQUEST_RESIZE             = BIT(2),
};

struct App
{
    struct Window
    {
        // TODO_DW: MULTIPLATFORM
        HWND mWinHandle = NULL;
        HINSTANCE mWinInstance = NULL;
    
        uint32 mWidth = 0;
        uint32 mHeight = 0;
    };

    Window  mWindow = {};
    uint32  mLoadRequests = LOAD_REQUEST_NONE;
    bool    mRunning = false;
};

void    initApp(uint32 w, uint32 h, const char* title, App* pApp);
void    destroyApp(App* pApp);
void    poll(App* pApp);
void    addLoadRequest(App* pApp, uint32 loadRequest);
void    removeLoadRequest(App* pApp, uint32 loadRequest);

// TODO_DW: MULTIPLATFORM
#define BEGIN_MAIN
#define END_MAIN return 0;
#define DW_MAIN() int main() { return wWinMain(GetModuleHandle(NULL), NULL, GetCommandLineW(), SW_SHOWNORMAL); } \
    int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrev, PWSTR pCmdLine, int nCmdShow)
