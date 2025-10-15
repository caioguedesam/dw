#pragma once
#include "base.hpp"

struct App;

void initTime(App* pApp);

#define TIMER_INVALID MAX_UINT64

struct Timer
{
    uint64 mStartTick   = TIMER_INVALID;
    uint64 mEndTick     = TIMER_INVALID;
    uint64 mFreq        = 0;
};

Timer createTimer(App* pApp);
void startTimer(Timer* pTimer);
void endTimer(Timer* pTimer);

uint64 getTicks(Timer* pTimer);
double getS(Timer* pTimer);
double getMS(Timer* pTimer);
double getNS(Timer* pTimer);

void waitBusyMS(App* pApp, double ms);
void sleepMS(uint64 ms);
