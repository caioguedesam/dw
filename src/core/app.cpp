#include "app.hpp"
#include "debug.hpp"
#include "memory.hpp"
#include "profile.hpp"
#include "time.hpp"

#include "../third_party/imgui/backends/imgui_impl_win32.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

// TODO_DW: MULTIPLATFORM

LRESULT CALLBACK Win32WndProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
    if(ImGui_ImplWin32_WndProcHandler(hwnd, umsg, wparam, lparam)) return TRUE;

    if(umsg == WM_NCCREATE)
    {
        CREATESTRUCT* cs = (CREATESTRUCT*)lparam;
        App* pApp = (App*)(cs->lpCreateParams);
        // Win32 is stupid: SetWindowLongPtr can return 0 and still be a success...
        SetLastError(0);
        int32 ret = SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pApp);
        int32 err = GetLastError();
        if(!ret) ASSERT(!err);
        return DefWindowProc(hwnd, umsg, wparam, lparam);
    }

    App* pApp = (App*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(!pApp)
    {
        return DefWindowProc(hwnd, umsg, wparam, lparam);
    }

    switch(umsg)
    {
        case WM_CLOSE:
        {
            pApp->mRunning = false;
        } break;
        case WM_SIZE:
        {
            pApp->mWindow.mWidth    = LOWORD(lparam);
            pApp->mWindow.mHeight   = HIWORD(lparam);
            addLoadRequest(pApp, LOAD_REQUEST_RESIZE);
        } break;
        default: break;
    }

    return DefWindowProc(hwnd, umsg, wparam, lparam);
}

void initApp(uint32 w, uint32 h, const char* title, App* pApp)
{
    ASSERT(pApp);
    *pApp = {};

    // Initializing app window
    {
        WNDCLASSA windowClass = {};
        windowClass.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
        windowClass.lpszClassName = "wndclassname";
        windowClass.lpfnWndProc = Win32WndProc;
        windowClass.hInstance = GetModuleHandle(NULL);  // Active executable
        RegisterClassA(&windowClass);

        HWND winHandle = CreateWindowExA(
                0, windowClass.lpszClassName,
                title,
                WS_OVERLAPPEDWINDOW,
                CW_USEDEFAULT, CW_USEDEFAULT,
                w, h,
                NULL, NULL,
                windowClass.hInstance,
                pApp);
        ASSERT(winHandle);

        App::Window window = {};
        window.mWidth = w;
        window.mHeight = h;
        window.mWinHandle = winHandle;
        window.mWinInstance = windowClass.hInstance;
        pApp->mWindow = window;
    }

    // App arena
    initArena(MB(64), &pApp->mAppArena);

    // Initializing time
    initTime(pApp);
    pApp->mTimer = createTimer(pApp);
    startTimer(&pApp->mTimer);
    endTimer(&pApp->mTimer);

    // Initializing input
    initInput(pApp);

    pApp->mRunning = true;

    ShowWindow(pApp->mWindow.mWinHandle, SW_SHOWNORMAL);
}

void destroyApp(App* pApp)
{
    ASSERT(pApp);
    // Destroying window
    {
        DestroyWindow(pApp->mWindow.mWinHandle);
    }

    destroyArena(&pApp->mAppArena);

    *pApp = {};
}

void poll(App* pApp)
{
    PROFILE_SCOPE;
    ASSERT(pApp);

    // Poll time
    float lastTime = getS(&pApp->mTimer);
    endTimer(&pApp->mTimer);
    pApp->mTime = getS(&pApp->mTimer);
    pApp->mDt = pApp->mTime - lastTime;

    // Poll input
    pollCursor(&pApp->mCursor);
    pollKeys(&pApp->mKeys);

    // Poll window messages
    MSG msg = {};
    while(true)
    {
        int32 ret = PeekMessage(&msg, pApp->mWindow.mWinHandle, 0, 0, PM_REMOVE);
        ASSERT(ret >= 0);
        if(!ret) break;
        DispatchMessage(&msg);
    }
}

void addLoadRequest(App* pApp, uint32 loadRequest)
{
    ASSERT(pApp);
    pApp->mLoadRequests |= loadRequest;
}

void removeLoadRequest(App* pApp, uint32 loadRequest)
{
    ASSERT(pApp);
    pApp->mLoadRequests &= ~loadRequest;
}

float getAspectRatio(App* pApp)
{
    ASSERT(pApp);
    return (float)pApp->mWindow.mWidth / (float)pApp->mWindow.mHeight;
}
