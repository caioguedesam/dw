#pragma once
#include "base.hpp"

struct Arena
{
    byte* pStart        = NULL;
    uint64 mOffset      = 0;
    uint64 mCapacity    = 0;
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
