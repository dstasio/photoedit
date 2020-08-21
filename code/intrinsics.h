#if !defined(INTRINSICS_H)
// ==============================================================
// Currently uses math.h, but it will be converted to intrinsics.
// ==============================================================
#include <math.h>

//
// all trigonometric functions are in radians
//

inline r32
Sin(r32 rad)
{
    r32 result = (r32)sin(rad);
    return result;
}

inline r32
Cos(r32 rad)
{
    r32 result = (r32)cos(rad);
    return result;
}

inline r32
Tan(r32 rad)
{
    r32 result = (r32)tan(rad);
    return result;
}

inline r32
Sqrt(r32 a)
{
    r32 result = (r32)sqrt(a);
    return result;
}

inline i32
Abs(i32 a)
{
    i32 result = (a < 0) ? -a : a;
    return result;
}

inline r32
Abs(r32 a)
{
    r32 result = (a < 0) ? -a : a;
    return result;
}

inline r32
Clamp(r32 a, r32 min, r32 max)
{
    r32 result = a;
    if (a < min) result = min;
    if (a > max) result = max;
    return result;
}

inline u8
Floor(r32 x)
{
    u8 result = (u8)x;
    return result;
}

//inline u16
//byte_swap(u16 x)
//{
//    u16 result = ((x >> 8) |   // 0xAA00 -> 0x00AA
//                  (x << 8));   // 0x00BB -> 0xBB00
//    return result;
//}

//inline u32
//byte_swap(u32 x)
//{
//#if 0
//    u32 result = ((x >> 24) |   // 0xAA000000 -> 0x000000AA
//                  ((x >>  8) & 0x0000FF00) |   // 0x00BB0000 -> 0x0000BB00
//                  ((x <<  8) & 0x00FF0000) |   // 0x0000CC00 -> 0x00CC0000
//                  (x << 24));   // 0x000000DD -> 0xDD000000
//#else
//    u32 result = ((x >> 24) |   // 0xAA000000 -> 0x000000AA
//                  ((x & 0x00FF0000) >>  8) |   // 0x00BB0000 -> 0x0000BB00
//                  ((x <<  8) & 0x00FF0000) |   // 0x0000CC00 -> 0x00CC0000
//                  (x << 24));   // 0x000000DD -> 0xDD000000
//#endif
//    return result;
//}

inline void
byte_swap(u16 *x)
{
    u16 result = ((*x >> 8) | (*x << 8));
    *x = result;
}

inline void
byte_swap(u32 *x) {
    u32 result = ((*x >> 24) | ((*x & 0x00FF0000) >>  8) | ((*x <<  8) & 0x00FF0000) | (*x << 24));
    *x = result;
}

#define INTRINSICS_H
#endif
