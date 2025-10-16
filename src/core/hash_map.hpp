#pragma once
#include "base.hpp"
#include "array.hpp"

#define HASH(x) hash((x))

inline uint64 hash(void* p)
{
    uint64 x = (uint64)p;

    // Mix bits: variant of splitmix64 finalizer
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;

    return x;
}

// Hash map
template <typename Tk, typename Tv>
struct HashMap
{
    struct Bucket
    {
        bool valid = false;
        Tk key;
        Tv value;
    };

    Array<Bucket> mBuckets;

    Tv& operator[](Tk key)
    {
        uint64 keyHash = HASH(key);
        for(uint64 i = 0; i < mBuckets.mCount; i++)
        {
            uint64 pos = (keyHash + i) % mBuckets.mCount;
            Bucket& bucket = mBuckets[pos];
            if(bucket.valid && bucket.key == key) return bucket.value;
        }
        ASSERT(0);      // Linear probing failed, hash map too small.
        return mBuckets[0].value;
    };

    const Tv& operator[](Tk key) const
    {

        uint64 keyHash = HASH(key);
        for(uint64 i = 0; i < mBuckets.mCount; i++)
        {
            uint64 pos = (keyHash + i) % mBuckets.mCount;
            Bucket& bucket = mBuckets[pos];
            if(bucket.valid && bucket.key == key) return bucket.value;
        }
        ASSERT(0);      // Linear probing failed, hash map too small.
        return mBuckets[0].value;
    };

    bool contains(Tk key)
    {
        uint64 keyHash = HASH(key);
        for(uint64 i = 0; i < mBuckets.mCount; i++)
        {
            uint64 pos = (keyHash + i) % mBuckets.mCount;
            Bucket& bucket = mBuckets[pos];
            if(bucket.valid && bucket.key == key) return true;
        }
        return false;
    }

    bool insert(const Tk& key, const Tv& value)
    {
        uint64 keyHash = HASH(key);
        for(uint64 i = 0; i < mBuckets.mCount; i++)
        {
            uint64 pos = (keyHash + i) % mBuckets.mCount;
            if(mBuckets[i].valid)
            {
                if(key == mBuckets[i].key)
                {
                    return false;
                }
            }

            if(!mBuckets[i].valid)
            {
                mBuckets[i] = { true, key, value };
                return true;
            }
        }
        ASSERT(0);      // Linear probing failed, hash map too small.
        return false;
    }

    void remove(const Tk& key)
    {
        uint64 keyHash = HASH(key);
        for(uint64 i = 0; i < mBuckets.mCount; i++)
        {
            uint64 pos = (keyHash + i) % mBuckets.mCount;
            if(mBuckets[i].valid && mBuckets[i].key == key)
            {
                mBuckets[i].valid = false;
                return;
            }
        }
        ASSERT(0);      // Key not present in the hash map, invalid op.
    }
};

template <typename Tk, typename Tv>
HashMap<Tk, Tv> hashmap(Arena* pArena, uint64 capacity)
{
    HashMap<Tk, Tv> result = {};
    result.mBuckets = array<typename HashMap<Tk, Tv>::Bucket>(pArena, capacity);
    typename HashMap<Tk, Tv>::Bucket empty = {};
    for(uint64 i = 0; i < capacity; i++)
    {
        result.mBuckets.push(empty);
    }
    return result;
}
