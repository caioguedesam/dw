#include "string.hpp"
#include "debug.hpp"
#include "memory.hpp"
#include <stdio.h>

bool operator==(String s1, String s2)
{
    return s1.mLen == s2.mLen && memcmp(s1.mData, s2.mData, s1.mLen) == 0;
}

bool operator!=(String s1, String s2)
{
    return s1.mLen != s2.mLen || memcmp(s1.mData, s2.mData, MIN(s1.mLen, s2.mLen)) != 0;
}

bool operator==(String s1, const char* s2)
{
    return s1 == str(s2);
}

bool operator!=(String s1, const char* s2)
{
    return s1 != str(s2);
}

bool operator==(const char* s1, String s2)
{
    return str(s1) == s2;
}

bool operator!=(const char* s1, String s2)
{
    return str(s1) != s2;
}

char& String::operator[](uint64 index)
{
    ASSERT(index < mLen);
    return (char&)mData[index];
}

char* cstr(String s)
{
    return (char*)s.mData;
}

uint64 hash(String s)
{
    // djb2 string hash
    uint64 result = 5381;
    for(uint64 i = 0; i < s.mLen; i++)
    {
        result = ((result << 5) + result) + (uint8)s.mData[i];
    }
    return result;
}

uint64 hash(const char* s)
{
    return hash(str(s));
}

String str(byte* pData, uint64 len)
{
    String s = {};
    s.mData = pData;
    s.mLen = len;
    return s;
}

String str(const char* literal)
{
    return str((byte*)literal, strlen(literal));
}

String str(Arena* pArena, const char* src)
{
    ASSERT(pArena);
    uint64 len = strlen(src);
    byte* buf = (byte*)arenaPush(pArena, len + 1);
    memcpy(buf, src, len);
    buf[len] = 0;   // Null terminator for c-string compatibility
    
    String s = {};
    s.mData = buf;
    s.mLen = len;
    return s;
}

String str(Arena* pArena, String src)
{
    ASSERT(pArena);

    uint64 len = src.mLen;
    byte* buf = (byte*)arenaPush(pArena, len + 1);
    memcpy(buf, src.mData, len);
    buf[len] = 0;   // Null terminator for c-string compatibility
    
    String s = {};
    s.mData = buf;
    s.mLen = len;
    return s;
}


int64 find(String s, char target)
{
    for(int64 i = 0; i < s.mLen; i++)
    {
        if(s[i] == target)
        {
            return i;
        }
    }
    return -1;
}

int64 find(String s, String target)
{
    ASSERT(target.mLen);
    if(target.mLen > s.mLen) return -1;

    // Naive string search
    for(int64 i = 0; i < s.mLen - target.mLen + 1; i++)
    {
        if(s[i] == target[0])
        {
            bool match = true;
            for(int64 j = 1; j < target.mLen; j++)
            {
                if(s[i + j] != target[j])
                {
                    match = false;
                    break;
                }
            }
            if(match) return i;
        }
    }
    return -1;
}

int64 rfind(String s, char target)
{
    for(int64 i = s.mLen - 1; i >= 0; i--)
    {
        if(s[i] == target)
        {
            return i;
        }
    }
    return -1;
}

int64 rfind(String s, String target)
{
    ASSERT(target.mLen);
    if(target.mLen > s.mLen) return -1;

    // Naive string search
    for(int64 i = s.mLen - 1; i >= target.mLen - 1; i--)
    {
        if(s[i] == target[0])
        {
            bool match = true;
            for(int64 j = 1; j < target.mLen; j++)
            {
                int64 cursor = i + j;
                if(cursor >= s.mLen || s[cursor] != target[j])
                {
                    match = false;
                    break;
                }
            }
            if(match) return i;
        }
    }
    return -1;
}

String substr(String s, uint64 start)
{
    ASSERT(start < s.mLen);
    String result = {};
    result.mData = s.mData + start;
    result.mLen = s.mLen - start;
    return result;
}

String substr(String s, uint64 start, uint64 len)
{
    ASSERT(start + len <= s.mLen);
    String result = {};
    result.mData = s.mData + start;
    result.mLen = len;
    return result;
}

String join(Arena* pArena, String s1, String s2)
{
    String result = {};
    result.mLen = s1.mLen + s2.mLen;
    byte* buf = (byte*)arenaPush(pArena, result.mLen + 1);
    memcpy(buf, s1.mData, s1.mLen);
    memcpy(buf + s1.mLen, s2.mData, s2.mLen);
    buf[result.mLen] = 0;   // Null terminator for c-string compatibility.
    result.mData = buf;
    return result;
}

String strf(Arena* pArena, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    int64 len = vsnprintf(0, 0, fmt, args);
    byte* buf = (byte*)arenaPush(pArena, len + 1);
    vsnprintf((char*)buf, len + 1, fmt, args);
    buf[len] = 0;   // Null terminator for c-string compatibility

    va_end(args);
    return str(buf, len);
}
