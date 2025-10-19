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
    void* result = arenaPush(pArena, size);
    if(pSrc)
    {
        memcpy(result, pSrc, srcSize);
    }
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

void initPool(uint64 blockSize, uint64 blockCount, Pool* pPool)
{
    ASSERT(pPool);

    uint64 fullBlockSize = blockSize + sizeof(Pool::Header);
    uint64 poolSize = fullBlockSize * blockCount;
    void* poolMemory = VirtualAlloc(0, poolSize,
            MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    pPool->pStart = (byte*)poolMemory;
    pPool->mBlockSize = blockSize;
    pPool->mBlockCount = blockCount;

    // Initializing block headers and free list
    Pool::Header* header = (Pool::Header*)pPool->pStart;
    for(uint64 i = 0; i < blockCount; i++)
    {
        header->mUsed = false;
        Pool::Header* next = (Pool::Header*)((uint64)header + fullBlockSize);
        header->pNextFree = (i == blockCount - 1)
            ? NULL
            : next;
        header = next;
    }

    pPool->pFreeBlocks = (Pool::Header*)pPool->pStart;
}

void destroyPool(Pool* pPool)
{
    ASSERT(pPool);
    VirtualFree(pPool->pStart, 0, MEM_RELEASE);
    *pPool = {};
}

void* poolAlloc(Pool* pPool)
{
    ASSERT(pPool);
    ASSERT(pPool->pFreeBlocks);

    Pool::Header* header = pPool->pFreeBlocks;
    pPool->pFreeBlocks = header->pNextFree;
    header->mUsed = true;
    return (void*)((uint64)header + sizeof(Pool::Header));
}

void poolFree(Pool* pPool, void* pBlock)
{
    ASSERT(pPool && pBlock);
    ASSERT(pBlock >= pPool->pStart);
    uint64 fullBlockSize = pPool->mBlockSize + sizeof(Pool::Header);
    uint64 poolSize = fullBlockSize * pPool->mBlockCount;
    ASSERT(pBlock < pPool->pStart + poolSize);

    Pool::Header* header = (Pool::Header*)((uint64)pBlock - sizeof(Pool::Header));
    header->mUsed = false;
    header->pNextFree = pPool->pFreeBlocks;
    pPool->pFreeBlocks = header;
}
