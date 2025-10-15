#include "memory.hpp"
#include "debug.hpp"

void initArena(uint64 size, Arena* pArena)
{
    ASSERT(pArena);
    // TODO_DW: MULTIPLATFORM
    void* arenaMemory = VirtualAlloc(0, size,
            MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    pArena->pStart = (byte*)arenaMemory;
    pArena->mOffset = 0;
    pArena->mCapacity = size;
}

void destroyArena(Arena* pArena)
{
    ASSERT(pArena);
    VirtualFree(pArena->pStart, 0, MEM_RELEASE);
    *pArena = {};
}

void* arenaPush(Arena* pArena, uint64 size)
{
    ASSERT(pArena);
    ASSERT(size > 0);
    ASSERT(pArena->mOffset + size <= pArena->mCapacity);
    byte* result = pArena->pStart + pArena->mOffset;
    pArena->mOffset += size;
    return result;
}

void* arenaPush(Arena* pArena, uint64 size, uint64 alignment)
{
    ASSERT(pArena);
    ASSERT(size > 0);
    byte* arenaTop = pArena->pStart + pArena->mOffset;
    byte* arenaTopAligned = (byte*)ALIGN_TO((uint64)arenaTop, alignment);
    ASSERT(IS_ALIGNED(arenaTopAligned, alignment));

    uint64 newOffset = pArena->mOffset + (arenaTopAligned - arenaTop) + size;
    ASSERT(newOffset <= pArena->mCapacity);
    pArena->mOffset = newOffset;

    return arenaTopAligned;
}

void* arenaPushZero(Arena* pArena, uint64 size)
{
    ASSERT(pArena);
    void* result = arenaPush(pArena, size);
    memset(result, 0, size);
    return result;
}

void* arenaPushZero(Arena* pArena, uint64 size, uint64 alignment)
{
    ASSERT(pArena);
    void* result = arenaPush(pArena, size, alignment);
    memset(result, 0, size);
    return result;
}

void* arenaPushCopy(Arena* pArena, uint64 size, void* pSrc, uint64 srcSize)
{
    ASSERT(pArena);
    ASSERT(pSrc);
    void* result = arenaPush(pArena, size);
    memcpy(result, pSrc, srcSize);
    return result;
}

void arenaClear(Arena* pArena)
{
    ASSERT(pArena);
    pArena->mOffset = 0;
}

void* arenaGetTop(Arena* pArena)
{
    ASSERT(pArena);
    return pArena->pStart + pArena->mOffset;
}

void arenaFallback(Arena* pArena, uint64 offset)
{
    ASSERT(pArena);
    ASSERT(offset <= pArena->mOffset);
    pArena->mOffset = offset;
}
