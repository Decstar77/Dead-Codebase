#pragma once

#define Assert(expr, msg)                                            \
    {                                                                \
        if (expr) {                                                  \
        } else {                                                     \
            __debugbreak();                                          \
        }                                                            \
}

#define REAL_MAX FLT_MAX
#define REAL_MIN -FLT_MAX

#define Kilobytes(val) (val * 1024LL)
#define Megabytes(val) (Kilobytes(val) * 1024LL)
#define Gigabytes(val) (Megabytes(val) * 1024LL)

#define SetABit(x) (1 << x)

#define Stringify(x) #x

#define ATTO_DEBUG 1
#define ATTO_DEBUG_RENDERING 1
#define ATTO_EDITOR 1

#define LOG_WARN_ENABLED 1
#define LOG_INFO_ENABLED 1
#define LOG_DEBUG_ENABLED 1
#define LOG_TRACE_ENABLED 1

#define DISABLE_COPY_AND_MOVE(clss)						\
    clss(const clss&) = delete;							\
    clss(clss&&) = delete;								\
    clss& operator=(const clss&) = delete;				\
    clss& operator=(clss&&) = delete;			

#define MACRO_COMBINE1(x, y) x##y
#define MACRO_COMBINE(x, y) MACRO_COMBINE1(x, y)

namespace atto
{
    typedef unsigned char u8;
    typedef unsigned short u16;
    typedef unsigned int u32;
    typedef unsigned long long u64;
    typedef signed char i8;
    typedef signed short i16;
    typedef signed int i32;
    typedef signed long long i64;
    typedef float f32;
    typedef double f64;
    typedef int b32;
    typedef bool b8;
    typedef u8 byte;

    static_assert(sizeof(u8) == 1, "Expected uint8 to be 1 byte.");
    static_assert(sizeof(u16) == 2, "Expected uint16 to be 2 bytes.");
    static_assert(sizeof(u32) == 4, "Expected uint32 to be 4 bytes.");
    static_assert(sizeof(u64) == 8, "Expected uint64 to be 8 bytes.");

    static_assert(sizeof(i8) == 1, "Expected int8 to be 1 byte.");
    static_assert(sizeof(i16) == 2, "Expected int16 to be 2 bytes.");
    static_assert(sizeof(i32) == 4, "Expected int32 to be 4 bytes.");
    static_assert(sizeof(i64) == 8, "Expected int64 to be 8 bytes.");

    static_assert(sizeof(f32) == 4, "Expected real32 to be 4 bytes.");
    static_assert(sizeof(f64) == 8, "Expected real64 to be 8 bytes.");

    template <typename T>
    inline T AlignUpWithMask(T value, u64 mask)
    {
        return (T)(((u64)value + mask) & ~mask);
    }

    template <typename T>
    inline T AlignDownWithMask(T value, u64 mask)
    {
        return (T)((u64)value & ~mask);
    }

    template <typename T>
    inline T AlignUp(T value, u64 alignment)
    {
        return AlignUpWithMask(value, alignment - 1);
    }

    template <typename T>
    inline T AlignDown(T value, u64 alignment)
    {
        return AlignDownWithMask(value, alignment - 1);
    }

    template <typename T>
    inline bool IsAligned(T value, u64 alignment)
    {
        return 0 == ((u64)value & (alignment - 1));
    }
}
