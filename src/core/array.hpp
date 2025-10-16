#pragma once
#include "base.hpp"
#include "memory.hpp"
#include "debug.hpp"

// Static array (fixed capacity, no dynamic resizing)
template <typename T>
struct Array
{
    uint64 mCapacity = 0;
    uint64 mCount = 0;
    T* mData = NULL;

    T& operator[](uint64 index)
    {
        ASSERT(index < mCount);
        return mData[index];
    }

    const T& operator[](uint64 index) const
    {
        ASSERT(index < mCount);
        return mData[index];
    }

    void push(const T& value)
    {
        ASSERT(mCount + 1 <= mCapacity);
        memcpy(mData + mCount, &value, sizeof(T));
        mCount++;
    }

    void pop()
    {
        ASSERT(mCount - 1 >= 0);
        mCount--;
    }

    void clear()
    {
        mCount = 0;
    }
};

template <typename T>
Array<T> array(Arena* pArena, uint64 capacity)
{
    Array<T> result = {};
    result.mCount = 0;
    result.mData = (T*)arenaPush(pArena, capacity * sizeof(T));
    result.mCapacity = capacity;
    return result;
}

template <typename T>
Array<T> array(Arena* pArena, uint64 capacity, uint64 initialCount, T initialValue)
{
    ASSERT(initialCount <= capacity);
    Array<T> result = array<T>(pArena, capacity);
    for(uint64 i = 0; i < initialCount; i++)
    {
        result.push(initialValue);
    }
    return result;
}

template <typename T>
Array<T> arrayAlign(Arena* pArena, uint64 capacity, uint64 alignment)
{
    Array<T> result = {};
    result.mCount = 0;
    result.mData = (T*)arenaPush(pArena, capacity * sizeof(T), alignment);
    result.mCapacity = capacity;
    return result;
}

template <typename T>
Array<T> arrayAlign(Arena* pArena, uint64 capacity, uint64 alignment, uint64 initialCount, T initialValue)
{
    ASSERT(initialCount <= capacity);
    Array<T> result = arrayAlign<T>(pArena, capacity, alignment);
    for(uint64 i = 0; i < initialCount; i++)
    {
        result.push(initialValue);
    }
    return result;
}
