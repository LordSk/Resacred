#pragma once
#include <stdint.h>
#include <assert.h>

#ifdef _WIN32
    #define CONF_WINDOWS
    #include <windows.h>
#endif

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

#define I16_MAX 0x7FFF
#define I32_MAX 0x7FFFFFFF
#define I64_MAX 0x7FFFFFFFFFFFFFFF

// http://www.gingerbill.org/article/defer-in-cpp.html
template<typename F>
struct __privDefer {
    F f;
    __privDefer(F f) : f(f) {}
    ~__privDefer() { f(); }
};

template<typename F>
__privDefer<F> __defer_func(F f) {
    return __privDefer<F>(f);
}

#define DO_GLUE(x, y) x##y
#define GLUE(x, y) DO_GLUE(x, y)
#define DEFER_VAR(x) GLUE(x, __COUNTER__)
#define defer(code) auto DEFER_VAR(_defer_) = __defer_func([&](){code;})


/**
  Declare singleton parameters.
  Usage:
  class MyClass
  {
    SINGLETON_IMP(MyClass)
    ...
  };
**/
#define SINGLETON_IMP(Class) \
    private:\
    Class(){}\
    public:\
    static inline Class& get()\
    {\
        static Class instance;\
        return instance;\
    }\
    Class(Class const&) = delete;\
    Class& operator=(Class const&) = delete;


#define forceinline __forceinline


#ifndef NDEBUG
    #define assert_msg(expr, msg) assert((expr) && (msg))
#else
    #define assert_msg(expr, msg)
#endif

template<int s> struct Sizeof;
#ifdef CONF_DEBUG
    #define MESSAGE_SIZEOF(s) Sizeof<sizeof(s)> GLUE(size_, s)
#else
    #define MESSAGE_SIZEOF(s)
#endif

#include <chrono>
#include <intrin.h>

template<typename T>
inline void swap(T& a, T& b) {
    T temp = a;
    a = b;
    b = temp;
}

template<typename T>
inline T abs(T value) {
    if(value < (T)0) {
        return -value;
    }
    return value;
}

#ifndef min
    #define min(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef max
    #define max(a, b) ((a) > (b) ? (a) : (b))
#endif

template<typename T>
inline T clamp(T value, T min, T max) {
    if(value < min) {
        return min;
    }
    if(value > max) {
        return max;
    }
    return value;
}

inline f32 pow(f32 value, u32 p) {
    f32 r = 1.f;
    for(u32 i = 0; i < p; ++i) {
        r *= value;
    }
    return r;
}

inline f64 pow(f64 value, u32 p) {
    f64 r = 1.0;
    for(u32 i = 0; i < p; ++i) {
        r *= value;
    }
    return r;
}

template<typename T>
inline i32 sign(T value) {
    return value < T(0) ? -1: 1;
}

template<typename T>
inline void arrayFillIncr(T* arr, u32 count, i32 start, i32 incr) {
    for(u32 i = 0; i < count; ++i) {
        arr[i] = start + i32(i)*incr;
    }
}

inline f32 frac(f32 f) {
    return f - (i32)f;
}

inline f64 frac(f64 f) {
    return f - (i64)f;
}

template<typename T>
inline T inBounds(T value, T min, T max) {
    return (value >= min && value <= max);
}

inline i32 log10(i64 n) {
    n = abs(n);
    if(n >= 1000000000) {
        return 9;
    }
    if(n >= 100000000) {
        return 8;
    }
    if(n >= 10000000) {
        return 7;
    }
    if(n >= 1000000) {
        return 6;
    }
    if(n >= 100000) {
        return 5;
    }
    if(n >= 10000) {
        return 4;
    }
    if(n >= 1000) {
        return 3;
    }
    if(n >= 100) {
        return 2;
    }
    if(n >= 10) {
        return 1;
    }
    return 0;
}

#define FNV_PRIME 16777619
#define FNV_OFFSET_BASIS 2166136261

/**
 * @brief 32bit FNV-1a hash
 * @param data
 * @param dataSize
 * @return hash
 */
inline
u32 hash32_fnv1a(const void* data, u32 dataSize) {
    u32 hash = FNV_OFFSET_BASIS;
    for(u32 i = 0; i < dataSize; ++i) {
        hash = (hash ^ ((const char*)data)[i]) * FNV_PRIME;
    }
    return hash;
}

// compile-time utils
template<typename T, i32 count>
constexpr i32 const_arrayCount(const T(&)[count]) {
    return count;
}

template<i32 Len>
constexpr i32 const_strLen(const char(&str)[Len]) {
    return Len-1;
}

constexpr i32 const_strLen_(const char* str, u32 i) {
    return str[i] == '\0' ? i : const_strLen_(str, i + 1);
}

constexpr i32 const_strLen(const char* str) {
    return str ? const_strLen_(str, 0) : 0;
}

constexpr
u32 const_hash32_fnv1a(const void* data, u32 dataSize, u32 hash, u32 i) {
    return i < dataSize ?
        const_hash32_fnv1a(data, dataSize, (hash ^ ((const char*)data)[i]) * FNV_PRIME, i+1):
        hash;
}

#define const_strHash32_fnv1a(str)\
    const_hash32_fnv1a(str, const_strLen(str), FNV_OFFSET_BASIS, 0)

typedef std::chrono::time_point<std::chrono::high_resolution_clock> timept;
#define timeNow() std::chrono::high_resolution_clock::now()
#define timeDuration(delta) std::chrono::duration<f64>(delta).count()
#define timeDurSince(timePoint) timeDuration(timeNow() - timePoint)

inline u64 countCycles() {
    return __rdtsc();
}

void randSetSeed(u32 newSeed);
u32 randUint32(u32* inSeed = nullptr);
f64 randF64(u32* inSeed = nullptr);

template<typename T>
using TCompareFuncType = i32(*)(const T&, const T&);

template<typename T>
void qsort(T* data, u32 count, TCompareFuncType<T> compareFunc)
{
    assert(count > 1);
    if(count == 2) {
        if(compareFunc(*data, *(data+1)) > 0) {
            swap(*data, *(data+1));
        }
        return;
    }

    T* pivot = data;
    swap(*pivot, *(data + (count >> 1))); // mitigate already sorted case
    T* last = data + count;
    T* left = pivot + 1;
    T* right = data + count - 1;

    while(1) {
        while(left < last && compareFunc(*left, *pivot) <= 0) {
            ++left;
        }
        while(right > pivot && compareFunc(*right, *pivot) > 0) {
            --right;
        }
        if(left >= right) {
            break;
        }
        swap(*left, *right);
    }

    // swap left with pivot
    if(left > pivot) {
        --left;
        swap(*pivot, *left);
    }

    // eliminate some of identical elements
    // aka the "Dutch national flag problem"
    T* leftMax = left-1;
    while(leftMax >= data && compareFunc(*leftMax, *left) == 0) {
        --leftMax;
    }
    ++leftMax;

    u32 leftPartCount = leftMax - data;
    u32 rightPartCount = count - (left - data) - 1;

    // smallest first
    if(leftPartCount < rightPartCount) {
        if(leftPartCount > 1) {
            qsort(data, leftPartCount, compareFunc);
        }
        if(rightPartCount > 1) {
            qsort(left + 1, rightPartCount, compareFunc);
        }
    }
    else {
        if(rightPartCount > 1) {
            qsort(left + 1, rightPartCount, compareFunc);
        }
        if(leftPartCount > 1) {
            qsort(data, leftPartCount, compareFunc);
        }
    }
}
