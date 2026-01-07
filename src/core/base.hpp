#pragma once

// Base includes
#include <windows.h>    // TODO_DW: MULTIPLATFORM
#include <stdint.h>
#include <float.h>

// Base types
typedef int8_t      int8;
typedef int16_t     int16;
typedef int32_t     int32;
typedef int64_t     int64;
typedef uint8_t     uint8;
typedef uint16_t    uint16;
typedef uint32_t    uint32;
typedef uint64_t    uint64;

typedef unsigned char byte;

// Base defines
#define MAX_UINT8   (0xFF)
#define MAX_UINT16  (0xFFFF)
#define MAX_UINT32  (0xFFFFFFFFUL)
#define MAX_UINT64  (0xFFFFFFFFFFFFFFFFULL)
#define MAX_FLOAT   (FLT_MAX)
#define MIN_FLOAT   (FLT_MIN)
#define MAX_DOUBLE  (DBL_MAX)
#define MIN_DOUBLE  (DBL_MIN)

#define EPSILON_FLOAT (FLT_EPSILON)
#define EPSILON_DOUBLE (DBL_EPSILON)

#undef KB
#undef MB
#undef GB
#define KB(V) ((V)   * 1024ULL)
#define MB(V) (KB(V) * 1024ULL)
#define GB(V) (MB(V) * 1024ULL)

#undef MIN
#undef MAX
#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define CLAMP(V, A, B) (MAX((A), MIN((V), (B))))
#define CLAMP_CEIL(V, A) MIN(V, A)
#define CLAMP_FLOOR(V, A) MAX(V, A)
#define ABS(V) ((V) < 0 ? -(V) : (V))
#define WITHIN(A, X, B) ((A) <= (X) && (X) <= (B))

#define STMT(S) do { S; } while(0)

#define STRINGIFY(S) #S     // For macro arg expansion
#define CONCATENATE_IMPL(A, B) A##B
#define CONCATENATE(A, B) CONCATENATE_IMPL(A, B)

#define IS_POW2(V) (((V) & ((V) - 1)) == 0)
#define ALIGN_TO(SIZE, BOUND) (((SIZE) + (BOUND) - 1) & ~((BOUND) - 1))   // Aligns to powers of 2 only
#define IS_ALIGNED(SIZE, BOUND) ((uint64)(SIZE) % (BOUND) == 0)

#define ARR_LEN(arr) (sizeof(arr)/sizeof(*(arr)))   // # of elements in array
#define ARR_SIZE(arr) (sizeof(arr))                 // Total size in bytes of array

#define OFFSET_IN(type, member) ((uint64)&((type*)0)->member)  // Offset of member variable in struct type

#define BIT(x) (1UL << (x))
#define SET_BIT(x, pos) ((x) |= (1UL << (pos)))
#define CLEAR_BIT(x, pos) ((x) &= (~(1UL << (pos))))
#define TOGGLE_BIT(x, pos) ((x) ^= (1UL << (pos)))
#define CHECK_BIT(x, pos) ((x) & (1UL << (pos)))

#define PTR_DIFF(PA, PB) (uint64)((byte*)(PA) - (byte*)(PB))
#define PTR_OFFSET(P, OFFSET) ((void*)((char*)(P) + (OFFSET)))

#define DEFAULT_ARRAY(ARR, SIZE) for(int32 _arr = 0; _arr < (SIZE); _arr++) { (ARR)[_arr] = {}; }

#define UNUSED(V) (void)(V)
