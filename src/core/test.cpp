#include "app.hpp"
#include "memory.hpp"
#include "time.hpp"
#include "debug.hpp"
#include "string.hpp"
#include "file.hpp"
#include "array.hpp"
#include "hash_map.hpp"

bool testMemory()
{
    Arena arena = {};
    // Testing initialization
    {
        initArena(1024, &arena);
    
        ASSERT(arena.pStart != NULL);
        ASSERT(arena.mCapacity == 1024);
        ASSERT(arena.mOffset == 0);
    
        destroyArena(&arena);
    
        ASSERT(arena.pStart == NULL);
        ASSERT(arena.mCapacity == 0);
        ASSERT(arena.mOffset == 0);
    }

    // Testing basic allocation
    {
        initArena(1024, &arena);

        void* a = arenaPush(&arena, 128);
        void* b = arenaPush(&arena, 128);
        void* c = arenaPush(&arena, 128);

        ASSERT(a);
        ASSERT(b);
        ASSERT(c);
        ASSERT(PTR_DIFF(b, a) >= 128);
        ASSERT(PTR_DIFF(c, b) >= 128);
        ASSERT(arena.mOffset <= arena.mCapacity);

        destroyArena(&arena);
    }

    // Testing aligned allocation
    {
        initArena(1024, &arena);

        void* a = arenaPush(&arena, 4, 16);
        ASSERT(IS_ALIGNED(a, 16));

        void* b = arenaPush(&arena, 8, 32);
        ASSERT(IS_ALIGNED(a, 32));

        destroyArena(&arena);
    }
    
    // Testing zeroed allocation
    {
        initArena(256, &arena);

        byte* mem = (byte*)arenaPushZero(&arena, 128);
        ASSERT(mem);

        for(int32 i = 0; i < 128; i++)
        {
            if(mem[i] != 0)
            {
                ASSERT(0);
            }
        }

        destroyArena(&arena);
    }

    // Testing copy allocation
    {
        initArena(256, &arena);

        const char srcData[] = "ArenaCopyTest";
        uint64 srcSize = (uint64)sizeof(srcData);
        void* mem = arenaPushCopy(&arena, srcSize, (void*)srcData, srcSize);

        ASSERT(mem);
        ASSERT(memcmp(mem, srcData, srcSize) == 0);

        destroyArena(&arena);
    }

    // Testing arena clear and offset
    {
        initArena(512, &arena);

        void* a = arenaPush(&arena, 64);
        void* b = arenaPush(&arena, 64);
        ASSERT(arena.mOffset > 0);

        void* top = arenaGetTop(&arena);
        ASSERT(top == arena.pStart + arena.mOffset);

        uint64 savedOffset = arena.mOffset;
        arenaFallback(&arena, savedOffset - 64);
        ASSERT(arena.mOffset == savedOffset - 64);

        arenaClear(&arena);
        ASSERT(arena.mOffset == 0);

        destroyArena(&arena);
    }

    return true;
}

bool testString()
{
    Arena arena = {};
    initArena(2048, &arena);

    // Testing literal and buffer-based construction
    {
        String s1 = str("Hello");
        ASSERT(s1.mLen == 5);
        ASSERT(s1.mData != NULL);
        ASSERT(memcmp(s1.mData, "Hello", 5) == 0);

        byte raw[] = { 'A', 'B', 'C', 0 };
        String s2 = str(raw, 3);
        ASSERT(s2.mLen == 3);
        ASSERT(s2.mData == raw);
        ASSERT(memcmp(s2.mData, "ABC", 3) == 0);
    }

    // Testing arena-based construction
    {
        const char* msg = "ArenaString";
        String s = str(&arena, msg);
        ASSERT(s.mLen == strlen(msg));
        ASSERT(s.mData != NULL);
        ASSERT(memcmp(s.mData, msg, s.mLen) == 0);

        // Verify arena advanced
        ASSERT(arena.mOffset > 0);
    }

    // Testing equality and inequality
    {
        String s1 = str("Test");
        String s2 = str("Test");
        String s3 = str("Different");

        ASSERT(s1 == s2);
        ASSERT(!(s1 != s2));
        ASSERT(s1 != s3);
        ASSERT(s1 == "Test");
        ASSERT("Test" == s1);
        ASSERT(s1 != "Other");
        ASSERT("Other" != s1);
    }

    // Testing cstr()
    {
        String s = str("Hello");
        char* c = cstr(s);
        ASSERT(strcmp(c, "Hello") == 0);
    }

    // Testing find() and rfind() with chars
    {
        String s = str("banana");
        ASSERT(find(s, 'b') == 0);
        ASSERT(find(s, 'n') == 2);
        ASSERT(find(s, 'z') == -1);

        ASSERT(rfind(s, 'a') == 5);
        ASSERT(rfind(s, 'b') == 0);
        ASSERT(rfind(s, 'z') == -1);
    }

    // Testing find() and rfind() with substrings
    {
        String s = str("abracadabra");

        ASSERT(find(s, str("abra")) == 0);
        ASSERT(find(s, str("cad")) == 4);
        ASSERT(find(s, str("zzz")) == -1);

        ASSERT(rfind(s, str("abra")) == 7);
        ASSERT(rfind(s, str("cad")) == 4);
        ASSERT(rfind(s, str("zzz")) == -1);
    }

    // Testing substr()
    {
        String s = str("substring");
        String sub1 = substr(s, 3);
        ASSERT(memcmp(sub1.mData, "string", sub1.mLen) == 0);
        ASSERT(sub1.mLen == 6);

        String sub2 = substr(s, 0, 3);
        ASSERT(memcmp(sub2.mData, "sub", sub2.mLen) == 0);
        ASSERT(sub2.mLen == 3);
    }

    // Testing join()
    {
        String s1 = str("Hello");
        String s2 = str("World");
        String joined = join(&arena, s1, s2);

        ASSERT(joined.mLen == s1.mLen + s2.mLen);
        ASSERT(memcmp(joined.mData, "HelloWorld", joined.mLen) == 0);
    }

    // Testing formatted string (strf)
    {
        String formatted = strf(&arena, "Value: %d %s", 42, "ok");
        ASSERT(formatted.mLen > 0);
        ASSERT(strstr((char*)formatted.mData, "Value: 42 ok"));
    }

    // Testing operator[]
    {
        String s = str("Index");
        ASSERT(s[0] == 'I');
        ASSERT(s[1] == 'n');
        ASSERT(s[4] == 'x');
    }

    destroyArena(&arena);
    return true;
}

bool testArray()
{
    Arena arena = {};
    initArena(4096, &arena);

    // Basic array creation
    {
        Array<int> arr = array<int>(&arena, 8);

        ASSERT(arr.mData != NULL);
        ASSERT(arr.mCapacity == 8);
        ASSERT(arr.mCount == 0);
    }

    // Push and index access
    {
        Array<int> arr = array<int>(&arena, 4);
        arr.push(10);
        arr.push(20);
        arr.push(30);

        ASSERT(arr.mCount == 3);
        ASSERT(arr[0] == 10);
        ASSERT(arr[1] == 20);
        ASSERT(arr[2] == 30);

        // Modify via operator[]
        arr[1] = 25;
        ASSERT(arr[1] == 25);
    }

    // Pop operation
    {
        Array<int> arr = array<int>(&arena, 4);
        arr.push(1);
        arr.push(2);
        arr.push(3);
        ASSERT(arr.mCount == 3);

        arr.pop();
        ASSERT(arr.mCount == 2);
        ASSERT(arr[0] == 1);
        ASSERT(arr[1] == 2);
    }

    // Clear operation
    {
        Array<int> arr = array<int>(&arena, 4);
        arr.push(5);
        arr.push(6);
        ASSERT(arr.mCount == 2);

        arr.clear();
        ASSERT(arr.mCount == 0);
    }

    // Initial count constructor
    {
        Array<int> arr = array<int>(&arena, 8, 3, 42);

        ASSERT(arr.mCapacity == 8);
        ASSERT(arr.mCount == 3);

        for (uint64 i = 0; i < arr.mCount; i++)
            ASSERT(arr[i] == 42);
    }

    // Aligned allocation
    {
        Array<int> arr = arrayAlign<int>(&arena, 16, 64);
        ASSERT(arr.mData != NULL);
        ASSERT(IS_ALIGNED(arr.mData, 64));
        ASSERT(arr.mCapacity == 16);
        ASSERT(arr.mCount == 0);
    }

    // Aligned allocation with initial values
    {
        Array<float> arr = arrayAlign<float>(&arena, 8, 32, 4, 3.14f);

        ASSERT(arr.mCapacity == 8);
        ASSERT(arr.mCount == 4);
        ASSERT(IS_ALIGNED(arr.mData, 32));

        for (uint64 i = 0; i < arr.mCount; i++)
            ASSERT(arr[i] == 3.14f);
    }

    destroyArena(&arena);
    return true;
}

void testHashMap()
{
    Arena arena = {};
    initArena(MB(1), &arena);

    // Basic creation
    {
        HashMap<const char*, int> map = hashmap<const char*, int>(&arena, 8);
        ASSERT(map.mBuckets.mCount == 8);
        ASSERT(map.mBuckets.mCapacity == 8);
        ASSERT(map.mBuckets[0].valid == false);
        ASSERT(map.mBuckets[7].valid == false);
    }

    // Insert & retrieve
    {
        HashMap<const char*, int> map = hashmap<const char*, int>(&arena, 8);
        map.insert("apple", 5);
        map.insert("banana", 10);
        map.insert("cherry", 15);

        ASSERT(map.contains("apple"));
        ASSERT(map.contains("banana"));
        ASSERT(map.contains("cherry"));
        ASSERT(map["apple"] == 5);
        ASSERT(map["banana"] == 10);
        ASSERT(map["cherry"] == 15);
    }

    // Replace value for same key
    {
        HashMap<const char*, int> map = hashmap<const char*, int>(&arena, 8);
        bool ret = map.insert("apple", 1);
        ASSERT(ret);
        ASSERT(map["apple"] == 1);
        ret = map.insert("apple", 99);
        ASSERT(!ret);
        ASSERT(map["apple"] == 1);
    }

    // Contains & remove
    {
        HashMap<const char*, int> map = hashmap<const char*, int>(&arena, 8);
        map.insert("dog", 50);
        map.insert("cat", 25);
        map.insert("mouse", 75);

        ASSERT(map.contains("cat"));
        ASSERT(map.contains("mouse"));
        map.remove("cat");
        ASSERT(!map.contains("cat"));
        ASSERT(map.contains("dog"));
        ASSERT(map.contains("mouse"));
    }

    // Linear probing behavior
    {
        HashMap<const char*, int> map = hashmap<const char*, int>(&arena, 4);
        map.insert("keyA", 1);
        map.insert("keyB", 2);
        map.insert("keyC", 3);
        map.insert("keyD", 4);

        ASSERT(map.contains("keyA"));
        ASSERT(map.contains("keyB"));
        ASSERT(map.contains("keyC"));
        ASSERT(map.contains("keyD"));
        ASSERT(map["keyB"] == 2);
    }

    // String keys
    {
        HashMap<String, int> map = hashmap<String, int>(&arena, 8);
        String key1 = str("foo");
        String key2 = str("bar");
        String key3 = str("baz");

        map.insert(key1, 10);
        map.insert(key2, 20);
        map.insert(key3, 30);

        ASSERT(map.contains(key1));
        ASSERT(map.contains(key2));
        ASSERT(map.contains(key3));
        ASSERT(map[key1] == 10);
        ASSERT(map[key2] == 20);
        ASSERT(map[key3] == 30);
    }

    // Pointer keys
    {
        HashMap<void*, const char*> map = hashmap<void*, const char*>(&arena, 8);
        int a, b, c;

        map.insert(&a, "alpha");
        map.insert(&b, "beta");
        map.insert(&c, "gamma");

        ASSERT(map.contains(&a));
        ASSERT(map.contains(&b));
        ASSERT(map.contains(&c));
        ASSERT(strcmp(map[&b], "beta") == 0);

        map.remove(&b);
        ASSERT(!map.contains(&b));
    }

    destroyArena(&arena);
}

void testTime(App* pApp)
{
    ASSERT(pApp);

    // Testing timer initialization
    {
        Timer t = createTimer(pApp);

        ASSERT(t.mFreq > 0);
    }

    // Testing start and end
    {
        Timer t = createTimer(pApp);
        startTimer(&t);
        ASSERT(t.mStartTick != TIMER_INVALID);
        ASSERT(t.mEndTick == TIMER_INVALID);

        waitBusyMS(pApp, 10.0);
        endTimer(&t);

        ASSERT(t.mStartTick != TIMER_INVALID);
        ASSERT(t.mEndTick != TIMER_INVALID);
        ASSERT(t.mEndTick > t.mStartTick);
    }
    
    // Testing time measurement
    {
        Timer t = createTimer(pApp);

        startTimer(&t);
        waitBusyMS(pApp, 25.0);
        endTimer(&t);

        double s    = getS(&t);
        double ms   = getMS(&t);
        double ns   = getNS(&t);

        ASSERT(s > 0.0);
        ASSERT(ms > 0.0);
        ASSERT(ns > 0.0);

        // Testing consistency accross units
        ASSERT(ABS(ms - s * 1000.0) < 0.5);
        ASSERT(ABS(ms * 1e6 - ns)   < 1e7); // ~10ms tolerance

        // Testing duration (within a range)
        ASSERT(ms >= 20.0);
        ASSERT(ms <= 30.0);
    }
    
    // Testing multiple measurements
    {
        Timer t = createTimer(pApp);

        startTimer(&t);
        waitBusyMS(pApp, 5.0);
        endTimer(&t);
        double first = getMS(&t);

        startTimer(&t);
        waitBusyMS(pApp, 10.0);
        endTimer(&t);
        double second = getMS(&t);

        ASSERT(second > first);
    }
}

bool testFile()
{
    Arena arena = {};
    initArena(4096, &arena);

    // Setup
    String dirPath  = str("test_dir");
    String filePath = str("test_dir/test_file.txt");
    const char* fileContent = "Hello, Arena File System!";
    uint64 contentLen = (uint64)strlen(fileContent);

    // Directory creation
    {
        bool created = createDir(dirPath);
        ASSERT(created);
        ASSERT(pathExists(dirPath));
        ASSERT(pathIsDir(dirPath));
    }

    // File creation and writing
    {
        bool fileCreated = createFile(filePath);
        ASSERT(fileCreated);
        ASSERT(pathExists(filePath));
        ASSERT(!pathIsDir(filePath));

        uint64 bytesWritten = writeFile(filePath, (byte*)fileContent, contentLen);
        ASSERT(bytesWritten == contentLen);
        ASSERT(getFileSize(filePath) == contentLen);
    }

    // Path parsing helpers
    {
        String ext = getExt(filePath);
        ASSERT(ext.mLen > 0);
        ASSERT(ext == "txt");

        String noExt = getNoExt(filePath);
        ASSERT(noExt.mLen > 0);
        ASSERT(noExt == "test_dir/test_file");

        String fileNameWithExt = getFileName(filePath, true);
        ASSERT(fileNameWithExt == "test_file.txt");

        String fileNameNoExt = getFileName(filePath, false);
        ASSERT(fileNameNoExt == "test_file");

        String fileDir = getFileDir(filePath);
        ASSERT(fileDir == "test_dir");
    }

    // Reading file (manual buffer)
    {
        byte tempBuf[256] = {};
        uint64 bytesRead = readFile(filePath, tempBuf);
        ASSERT(bytesRead == contentLen);
        ASSERT(memcmp(tempBuf, fileContent, contentLen) == 0);
    }

    // Reading file (arena buffer)
    {
        uint64 readSize = 0;
        byte* data = readFile(&arena, filePath, &readSize);
        ASSERT(data);
        ASSERT(readSize == contentLen);
        ASSERT(memcmp(data, fileContent, readSize) == 0);
    }

    // Reading file as String
    {
        String content = readFileStr(&arena, filePath);
        ASSERT(content.mData);
        ASSERT(content.mLen == contentLen);
        ASSERT(content == fileContent);
    }

    // Cleanup and deletion
    {
        ASSERT(deleteFile(filePath));
        ASSERT(!pathExists(filePath));

        ASSERT(deleteDir(dirPath));
        ASSERT(!pathExists(dirPath));
    }

    destroyArena(&arena);
    return true;
}

void testCore(App* pApp)
{
    ASSERT(pApp);
    LOG("[TEST] Testing memory...");
    testMemory();

    LOG("[TEST] Testing string...");
    testString();

    LOG("[TEST] Testing file...");
    testFile();

    LOG("[TEST] Testing array...");
    testArray();

    LOG("[TEST] Testing hash map...");
    testHashMap();

    LOG("[TEST] Testing time...");
    testTime(pApp);

    LOG("[TEST] All core tests passed.");
}

void debugInput(App* pApp)
{
    ASSERT(pApp);

    // Testing keys
    for(uint8 i = 0; i < 255; i++)
    {
        if(isJustDown(&pApp->mKeys, (KeyInput)i))
        {
            LOGF("[INPUT] Key just down: %u", i);
        }
        if(isDown(&pApp->mKeys, (KeyInput)i))
        {
            LOGF("[INPUT] Key down: %u", i);
        }

        if(isJustUp(&pApp->mKeys, (KeyInput)i))
        {
            LOGF("[INPUT] Key just up: %u", i);
        }
    }

    // Testing cursor
    int32 x, y;
    float dx, dy;
    getPos(&pApp->mCursor, &x, &y);
    getDelta(&pApp->mCursor, &dx, &dy);
    if(dx != 0.f || dy != 0.f)
    {
        LOGF("[INPUT] Cursor pos    (%d, %d)", x, y);
        LOGF("[INPUT] Cursor delta  (%.3f, %.3f)", dx, dy);
    }
}
