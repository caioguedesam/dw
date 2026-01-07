#pragma once
#include "base.hpp"
#include "memory.hpp"

// Immutable string type, represents either a view to a literal or to a buffer.
struct String
{
    uint64 mLen = 0;
    byte* mData = NULL;

    char& operator[](uint64 index);
};

bool operator==(String s1, String s2);
bool operator!=(String s1, String s2);
bool operator==(String s1, const char* s2);
bool operator!=(String s1, const char* s2);
bool operator==(const char* s1, String s2);
bool operator!=(const char* s1, String s2);

char* cstr(String s);
#define STRF_ARG(S) (S).mLen, cstr((S))

uint64 hash(String s);
uint64 hash(const char* s);

String str(byte* pData, uint64 len);
String str(const char* literal);
String str(Arena* pArena, const char* src);
String str(Arena* pArena, String src);

int64 find(String s, char target);
int64 find(String s, String target);
int64 rfind(String s, char target);
int64 rfind(String s, String target);
String substr(String s, uint64 start);
String substr(String s, uint64 start, uint64 len);

String join(Arena* pArena, String s1, String s2);
String strf(Arena* pArena, const char* fmt, ...);
String strf(char* buf, const char* fmt, ...);
