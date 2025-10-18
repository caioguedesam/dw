#pragma once
#include "base.hpp"

// Arena allocator
struct Arena
{
    byte*   pStart       = NULL;
    uint64  mOffset      = 0;
    uint64  mCapacity    = 0;
};

void initArena(uint64 size, Arena* pArena);
void destroyArena(Arena* pArena);

void*   arenaPush(Arena* pArena, uint64 size);
void*   arenaPush(Arena* pArena, uint64 size, uint64 alignment);
void*   arenaPushZero(Arena* pArena, uint64 size);
void*   arenaPushZero(Arena* pArena, uint64 size, uint64 alignment);
void*   arenaPushCopy(Arena* pArena, uint64 size, void* pSrc, uint64 srcSize);
void    arenaClear(Arena* pArena);
void*   arenaGetTop(Arena* pArena);
void    arenaFallback(Arena* pArena, uint64 offset);

#define ARENA_CHECKPOINT_SET(ARENA, NAME) uint64 CONCATENATE(NAME, __fallback) = (ARENA)->mOffset
#define ARENA_CHECKPOINT_RESET(ARENA, NAME) arenaFallback((ARENA), CONCATENATE(NAME, __fallback))

// Pool allocator
struct Pool
{
    struct Header
    {
        Header*     pNextFree = NULL;
        bool        mUsed = false;
    };

    byte*   pStart = NULL;
    Header* pFreeBlocks = NULL;
    uint64  mBlockSize  = 0;
    uint64  mBlockCount = 0;
};

void initPool(uint64 blockSize, uint64 blockCount, Pool* pPool);
void destroyPool(Pool* pPool);

void*   poolAlloc(Pool* pPool);
void    poolFree(Pool* pPool, void* pBlock);
