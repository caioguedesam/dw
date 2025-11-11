#include "input.hpp"
#include "app.hpp"
#include "debug.hpp"
#include <math.h>

// TODO_DW: MULTIPLATFORM

void initInput(App* pApp)
{
    ASSERT(pApp);

    pApp->mCursor = {};
    pApp->mKeys = {};

    memset(pApp->mKeys.mKeys, 0, MAX_KEY_INPUTS);
    memset(pApp->mKeys.mPrevKeys, 0, MAX_KEY_INPUTS);
}

void pollCursor(CursorState* pState)
{
    ASSERT(pState);

    POINT cursorPoint;
    BOOL ret = GetCursorPos(&cursorPoint);
    ASSERT(ret);
    HWND activeWindow = GetActiveWindow();
    if(!activeWindow) return;   // Skip cursor updates entirely when app is minimized

    ret = ScreenToClient(activeWindow, &cursorPoint);
    ASSERT(ret);

    float posX = (float)cursorPoint.x;
    float posY = (float)cursorPoint.y;
    float prevX = pState->mPosX;
    float prevY = pState->mPosY;

    float deltaX = posX - prevX;
    float deltaY = posY - prevY;
    float deltaLen = sqrtf(deltaX * deltaX + deltaY * deltaY);

    if(deltaLen < 1e-5)
    {
        pState->mDeltaX = 0;
        pState->mDeltaY = 0;
    }
    else
    {
        pState->mDeltaX = deltaX / deltaLen;
        pState->mDeltaY = deltaY / deltaLen;
    }

    if(pState->mLocked)
    {
        RECT clientRect;
        ret = GetClientRect(GetActiveWindow(), &clientRect);
        ASSERT(ret);
        POINT lockPoint = {clientRect.right / 2, clientRect.bottom / 2};
        ret = ClientToScreen(GetActiveWindow(), &lockPoint);
        ASSERT(ret);
        ret = SetCursorPos(lockPoint.x, lockPoint.y);
        ASSERT(ret);

        ret = ScreenToClient(GetActiveWindow(), &lockPoint);
        ASSERT(ret);
        posX = (float)lockPoint.x;
        posY = (float)lockPoint.y;
    }

    pState->mPosX = posX;
    pState->mPosY = posY;
}

void getPos(CursorState* pState, int32* pX, int32* pY)
{
    ASSERT(pState);
    if(pX) *pX = pState->mPosX;
    if(pY) *pY = pState->mPosY;
}

void getDelta(CursorState* pState, float* pX, float* pY)
{
    ASSERT(pState);
    if(pX) *pX = pState->mDeltaX;
    if(pY) *pY = pState->mDeltaY;
}

void setHidden(CursorState* pState, bool hide)
{
    ASSERT(pState);
    if(pState->mHidden == hide) return;
    pState->mHidden = hide;
    ShowCursor(!pState->mHidden);
}

void setLocked(CursorState* pState, bool lock)
{
    ASSERT(pState);
    if(pState->mLocked == lock) return;
    pState->mLocked = lock;
}

void toggleHidden(CursorState* pState)
{
    ASSERT(pState);
    setHidden(pState, !pState->mHidden);
}

void toggleLocked(CursorState* pState)
{
    ASSERT(pState);
    setLocked(pState, !pState->mLocked);
}


void pollKeys(KeyState* pState)
{
    ASSERT(pState);
    memcpy(pState->mPrevKeys, pState->mKeys, MAX_KEY_INPUTS * sizeof(uint8));
    BOOL ret = GetKeyboardState(pState->mKeys);
    ASSERT(ret);
}

bool isDown(KeyState* pState, KeyInput key)
{
    ASSERT(pState);
    return pState->mKeys[key] & 0x80;
}

bool isUp(KeyState* pState, KeyInput key)
{
    ASSERT(pState);
    return !(pState->mKeys[key] & 0x80);
}

bool isJustDown(KeyState* pState, KeyInput key)
{
    ASSERT(pState);
    return isDown(pState, key)
        && !(pState->mPrevKeys[key] & 0x80);
}

bool isJustUp(KeyState* pState, KeyInput key)
{
    ASSERT(pState);
    return isUp(pState, key)
        && (pState->mPrevKeys[key] & 0x80);
}
