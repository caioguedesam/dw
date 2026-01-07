#include "file.hpp"
#include "debug.hpp"
#include "string.hpp"

// TODO_DW: MULTIPLATFORM

bool pathExists(String path)
{
    DWORD fileAttributes = GetFileAttributes(cstr(path));
    return fileAttributes != INVALID_FILE_ATTRIBUTES;
}

bool pathIsDir(String path)
{
    DWORD fileAttributes = GetFileAttributes(cstr(path));
    return fileAttributes & FILE_ATTRIBUTE_DIRECTORY;
}

String getExt(String path)
{
    ASSERT(pathExists(path));
    ASSERT(!pathIsDir(path));
    uint64 extStart = rfind(path, '.');
    ASSERT(extStart != -1);
    return substr(path, extStart + 1);  // Extension without dot
}

String getNoExt(String path)
{
    ASSERT(pathExists(path));
    ASSERT(!pathIsDir(path));
    uint64 extStart = rfind(path, '.');
    ASSERT(extStart != -1);
    return substr(path, 0, extStart);
}

String getFileName(String path, bool ext)
{
    ASSERT(pathExists(path));
    ASSERT(!pathIsDir(path));

    uint64 lastSlash = rfind(path, '\\');
    if(lastSlash == -1) lastSlash = rfind(path, '/');

    String result;
    if(lastSlash == -1)
        result = substr(path, 0);
    else
        result = substr(path, lastSlash + 1);

    if(!ext)
        result = substr(result, 0, result.mLen - getExt(path).mLen - 1);

    return result;
}

String getFileDir(String path, bool trailingSlash)
{
    ASSERT(pathExists(path));
    ASSERT(!pathIsDir(path));

    uint64 lastSlash = rfind(path, '\\');
    if(lastSlash == -1) lastSlash = rfind(path, '/');
    ASSERT(lastSlash != -1);

    return !trailingSlash 
        ? substr(path, 0, lastSlash)
        : substr(path, 0, lastSlash + 1);
}

uint64 getFileSize(String path)
{
    ASSERT(pathExists(path));
    ASSERT(!pathIsDir(path));

    HANDLE hFile = CreateFile(
            cstr(path),
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
    ASSERT(hFile != INVALID_HANDLE_VALUE);
    DWORD fSize = ::GetFileSize(hFile, NULL);
    ASSERT(fSize != INVALID_FILE_SIZE);
    CloseHandle(hFile);
    return (uint64)fSize;
}

bool createFile(String path)
{
    HANDLE hFile = CreateFile(
            cstr(path),
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
    ASSERT(hFile != INVALID_HANDLE_VALUE);
    CloseHandle(hFile);
    return true;
}

bool createDir(String path)
{
    BOOL ret = CreateDirectory(
            cstr(path),
            NULL);
    ASSERT(ret);
    return true;
}

bool deleteFile(String path)
{
    BOOL ret = DeleteFile(cstr(path));
    ASSERT(ret);
    return true;
}

bool deleteDir(String path)
{
    BOOL ret = RemoveDirectory(cstr(path));
    ASSERT(ret);
    return true;
}

uint64 readFile(String path, byte* pOut)
{
    HANDLE hFile = CreateFile(
            cstr(path),
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
    ASSERT(hFile != INVALID_HANDLE_VALUE);
    DWORD fSize = GetFileSize(hFile, NULL);
    ASSERT(fSize != INVALID_FILE_SIZE);
    DWORD bytesRead = 0;
    BOOL ret = ReadFile(
            hFile,
            pOut,
            fSize,
            &bytesRead,
            NULL);
    ASSERT(ret);

    CloseHandle(hFile);
    return (uint64)bytesRead;
}

byte* readFile(Arena* pArena, String path, uint64* pOutSize)
{
    uint64 fSize = getFileSize(path);
    byte* result = (byte*)arenaPush(pArena, fSize);
    uint64 bytesRead = readFile(path, result);
    ASSERT(bytesRead == fSize);
    if(pOutSize) *pOutSize = fSize;
    return result;
}

String readFileStr(Arena* pArena, String path)
{
    uint64 fSize = getFileSize(path);
    byte* buf = (byte*)arenaPush(pArena, fSize + 1);
    uint64 len = readFile(path, buf);
    ASSERT(len == fSize);
    buf[len] = 0;   // Null terminator for c-string compatibility.
    return str(buf, len);
}

uint64 writeFile(String path, byte* pSrc, uint64 len)
{
    HANDLE hFile = CreateFile(
            cstr(path),
            GENERIC_WRITE,
            FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
    ASSERT(hFile != INVALID_HANDLE_VALUE);
    DWORD fSize = ::GetFileSize(hFile, NULL);
    ASSERT(fSize != INVALID_FILE_SIZE);
    DWORD bytesWritten = 0;
    BOOL ret = WriteFile(
            hFile,
            pSrc,
            len,
            &bytesWritten,
            NULL);
    ASSERT(ret);

    CloseHandle(hFile);
    return (uint64)bytesWritten;
}
