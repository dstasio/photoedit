#if !defined(DATATYPES_H)
#include <stdint.h>

#define global static
#define internal static
#define local_persist static

#define kilobytes(k) (1024LL*(k))
#define megabytes(m) (1024LL*kilobytes(m))
#define gigabytes(g) (1024LL*megabytes(g))
#define terabytes(t) (1024LL*gigabytes(t))

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef i32 b32;

typedef size_t memory_index;

typedef float  r32;
typedef double r64;

union v2i
{
    struct { i32 x, y; };
    i32    row[2];
};

union v2
{
    struct { r32 x, y; };
    r32    row[2];
};

union v3
{
    struct { r32 x, y, z; };
    r32    row[3];
    v2     xy;

    operator bool()
    {
        return x||y||z;
    }

};

union v4
{
    struct { r32 x, y, z, w; };
    r32    row[4];
};

#define DATATYPES_H
#endif
