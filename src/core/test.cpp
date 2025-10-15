#include "app.hpp"
#include "memory.hpp"
#include "time.hpp"
#include "debug.hpp"

bool testMemory()
{
    Arena arena = {};
    // Testing initialization
    {
        initArena(1024, &arena);
    
        ASSERT(arena.pStart != NULL);
        ASSERT(arena.mCapacity == 1024);
        ASSERT(arena.mOffset == 0);
    
        destroyArena(&arena);
    
        ASSERT(arena.pStart == NULL);
        ASSERT(arena.mCapacity == 0);
        ASSERT(arena.mOffset == 0);
    }

    // Testing basic allocation
    {
        initArena(1024, &arena);

        void* a = arenaPush(&arena, 128);
        void* b = arenaPush(&arena, 128);
        void* c = arenaPush(&arena, 128);

        ASSERT(a);
        ASSERT(b);
        ASSERT(c);
        ASSERT(PTR_DIFF(b, a) >= 128);
        ASSERT(PTR_DIFF(c, b) >= 128);
        ASSERT(arena.mOffset <= arena.mCapacity);

        destroyArena(&arena);
    }

    // Testing aligned allocation
    {
        initArena(1024, &arena);

        void* a = arenaPush(&arena, 4, 16);
        ASSERT(IS_ALIGNED(a, 16));

        void* b = arenaPush(&arena, 8, 32);
        ASSERT(IS_ALIGNED(a, 32));

        destroyArena(&arena);
    }
    
    // Testing zeroed allocation
    {
        initArena(256, &arena);

        byte* mem = (byte*)arenaPushZero(&arena, 128);
        ASSERT(mem);

        for(int32 i = 0; i < 128; i++)
        {
            if(mem[i] != 0)
            {
                ASSERT(0);
            }
        }

        destroyArena(&arena);
    }

    // Testing copy allocation
    {
        initArena(256, &arena);

        const char srcData[] = "ArenaCopyTest";
        uint64 srcSize = (uint64)sizeof(srcData);
        void* mem = arenaPushCopy(&arena, srcSize, (void*)srcData, srcSize);

        ASSERT(mem);
        ASSERT(memcmp(mem, srcData, srcSize) == 0);

        destroyArena(&arena);
    }

    // Testing arena clear and offset
    {
        initArena(512, &arena);

        void* a = arenaPush(&arena, 64);
        void* b = arenaPush(&arena, 64);
        ASSERT(arena.mOffset > 0);

        void* top = arenaGetTop(&arena);
        ASSERT(top == arena.pStart + arena.mOffset);

        uint64 savedOffset = arena.mOffset;
        arenaFallback(&arena, savedOffset - 64);
        ASSERT(arena.mOffset == savedOffset - 64);

        arenaClear(&arena);
        ASSERT(arena.mOffset == 0);

        destroyArena(&arena);
    }

    return true;
}

void testTime(App* pApp)
{
    ASSERT(pApp);

    // Testing timer initialization
    {
        Timer t = createTimer(pApp);

        ASSERT(t.mFreq > 0);
    }

    // Testing start and end
    {
        Timer t = createTimer(pApp);
        startTimer(&t);
        ASSERT(t.mStartTick != TIMER_INVALID);
        ASSERT(t.mEndTick == TIMER_INVALID);

        waitBusyMS(pApp, 10.0);
        endTimer(&t);

        ASSERT(t.mStartTick != TIMER_INVALID);
        ASSERT(t.mEndTick != TIMER_INVALID);
        ASSERT(t.mEndTick > t.mStartTick);
    }
    
    // Testing time measurement
    {
        Timer t = createTimer(pApp);

        startTimer(&t);
        waitBusyMS(pApp, 25.0);
        endTimer(&t);

        double s    = getS(&t);
        double ms   = getMS(&t);
        double ns   = getNS(&t);

        ASSERT(s > 0.0);
        ASSERT(ms > 0.0);
        ASSERT(ns > 0.0);

        // Testing consistency accross units
        ASSERT(ABS(ms - s * 1000.0) < 0.5);
        ASSERT(ABS(ms * 1e6 - ns)   < 1e7); // ~10ms tolerance

        // Testing duration (within a range)
        ASSERT(ms >= 20.0);
        ASSERT(ms <= 30.0);
    }
    
    // Testing multiple measurements
    {
        Timer t = createTimer(pApp);

        startTimer(&t);
        waitBusyMS(pApp, 5.0);
        endTimer(&t);
        double first = getMS(&t);

        startTimer(&t);
        waitBusyMS(pApp, 10.0);
        endTimer(&t);
        double second = getMS(&t);

        ASSERT(second > first);
    }
}

void testCore(App* pApp)
{
    ASSERT(pApp);
    LOG("[TEST] Testing memory...");
    testMemory();

    LOG("[TEST] Testing time...");
    testTime(pApp);

    LOG("[TEST] All core tests passed.");
}
