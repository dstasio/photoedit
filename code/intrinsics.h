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

#define INTRINSICS_H
#endif
