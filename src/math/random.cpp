#include "random.hpp"

uint64 randomU64()
{
    // Xorshift*64
    static uint64 x = __rdtsc();
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    return x * 0x2545F4914F6CDD1DULL;
}

float randomUniformF32()
{
    return (float)randomU64()/(float)MAX_UINT64;
}

float randomUniformF32(float start, float end)
{
    return start + randomUniformF32() * (end - start);
}

int32 randomUniformI32(int32 start, int32 end)
{
    // Range: [start, end]
    return start + randomUniformF32() * (end + 1 - start);
}

v3f randomUniformV3F()
{
    return
    {
        randomUniformF32(),
        randomUniformF32(),
        randomUniformF32(),
    };
}

v3f randomUniformV3F(float start, float end)
{
    return
    {
        randomUniformF32(start, end),
        randomUniformF32(start, end),
        randomUniformF32(start, end),
    };
}
