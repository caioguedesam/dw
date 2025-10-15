#pragma once
#include "base.hpp"
#include "string.hpp"
#include "memory.hpp"

bool pathExists(String path);
bool pathIsDir(String path);

String getExt(String path);         // No dot before extension
String getNoExt(String path);
String getFileName(String path, bool ext = false);
String getFileDir(String path);     // No trailing slash
uint64 getFileSize(String path);

bool createFile(String path);
bool createDir(String path);
bool deleteFile(String path);
bool deleteDir(String path);

uint64  readFile(String path, byte* pOut);
byte*   readFile(Arena* pArena, String path, uint64* pOutSize = NULL);
String  readFileStr(Arena* pArena, String path);

uint64  writeFile(String path, byte* pSrc, uint64 len);
