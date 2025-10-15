#include "time.hpp"
#include "app.hpp"
#include "debug.hpp"

void initTime(App* pApp)
{
    ASSERT(pApp);
    // TODO_DW: MULTIPLATFORM
    LARGE_INTEGER frequency;
    BOOL ret = QueryPerformanceFrequency(&frequency);
    ASSERT(ret);

    pApp->mTicksPerSecond = frequency.QuadPart;
}

Timer createTimer(App* pApp)
{
    ASSERT(pApp && pApp->mTicksPerSecond);
    Timer timer = {};
    timer.mFreq = pApp->mTicksPerSecond;
    return timer;
}

void startTimer(Timer *pTimer)
{
    ASSERT(pTimer);
    LARGE_INTEGER counter;
    BOOL ret = QueryPerformanceCounter(&counter);
    ASSERT(ret);
    pTimer->mStartTick = counter.QuadPart;
}

void endTimer(Timer* pTimer)
{
    ASSERT(pTimer && pTimer->mStartTick != TIMER_INVALID);
    LARGE_INTEGER counter;
    BOOL ret = QueryPerformanceCounter(&counter);
    ASSERT(ret);
    pTimer->mEndTick = counter.QuadPart;
}

uint64 getTicks(Timer* pTimer)
{
    ASSERT(pTimer);
    ASSERT(pTimer->mStartTick != TIMER_INVALID);
    ASSERT(pTimer->mEndTick != TIMER_INVALID);
    return pTimer->mEndTick - pTimer->mStartTick;
}

double getS(Timer* pTimer)
{
    uint64 ticks = getTicks(pTimer);
    return (double)(ticks) / (double)(pTimer->mFreq);
}

double getMS(Timer* pTimer)
{
    uint64 ticks = getTicks(pTimer);
    return (double)(ticks) * (double)1e3 / (double)(pTimer->mFreq);
}

double getNS(Timer* pTimer)
{
    uint64 ticks = getTicks(pTimer);
    return (double)(ticks) * (double)1e9 / (double)(pTimer->mFreq);
}

void waitBusyMS(App* pApp, double ms)
{
    Timer timer = createTimer(pApp);
    startTimer(&timer);
    while(true)
    {
        endTimer(&timer);
        double elapsed = getMS(&timer);
        if(elapsed >= ms)
        {
            break;
        }
    }
}

void sleepMS(uint64 ms)
{
    // TODO_DW: MULTIPLATFORM
    Sleep(ms);
}
