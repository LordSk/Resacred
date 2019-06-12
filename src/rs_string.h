#pragma once
#include <io.h>
#include <stdarg.h>
#include "rs_base.h"
#include "rs_allocator.h"

/**
 * @brief Length of a null terminated string
 * @param src
 * @return
 */
inline i32 strLen(const char* str)
{
    if(!str) return 0;
    const char* cur = str;
    while(*cur != 0) {
        ++cur;
    }
    return (cur - str);
}

/**
 * @brief Compare two strings
 * @param str1
 * @param str2
 * @param length
 * @return -1 if equal, or where the string differ
 */
i32 strCmp(const char* str1, const char* str2, u32 length);

/**
 * @brief Compare two strings
 * @param str1
 * @param str2
 * @return equality
 */
bool strEq(const char* str1, const char* str2);

/**
 * @brief Copies src to dest
 * @param src
 * @param dest
 * @param length
 */
inline void strCopy(char* dest, const char* src, u32 length)
{
    memcpy(dest, src, length);
}

/**
 * @brief toLower
 * @param dest
 * @param src
 * @param length
 */
//void toLower(char* dest, const char* src, u32 length);

/**
 * @brief Hash a string
 * @param src
 * @param length
 * @return hash
 */
u32 strHash(const char* src, u32 length);

/**
 * @brief Hash a string using FNV-1a
 * @param src
 * @param length
 * @return
 */
inline u32 strHash_fnv1a(const char* src, u32 length)
{
    return hash32_fnv1a(src, length);
}

/**
 * @brief Is character a digit [0-9]
 * @param c
 * @return
 */
inline bool isDigit(char c)
{
    return (c >= '0' && c <= '9');
}

#if 0
/**
 * @brief String to i64
 * @param str
 * @param strLength
 * @return
 */
i64 parseI64(const char* str, i32 strLength = -1);

/**
 * @brief String to f64
 * @param str
 * @param strLength
 * @return
 */
f64 parseF64(const char* str, i32 strLength = -1);

/**
 * @brief integer to string
 * @param number
 * @param dest
 * @param base
 */
template<typename T>
void integerToStr(T number, char* dest, u32 base = 10)
{
    assert(base >= 2 && base <= 16);
    constexpr const char* baseStr = "0123456789abcdef";

    if(number == 0) {
        dest[0] = '0';
        dest[1] = 0; // null terminate
        return;
    }

    bool negative = false;
    if(number < 0) {
        negative = true;
        number = -number;
    }

    i32 d = 0;
    char rev[64];
    while(number != 0) {
        rev[d++] = baseStr[number%base];
        number /= base;
    }

    // negative sign if base 10
    i32 offset = 0;
    if(negative && base == 10) {
        offset = 1;
        dest[0] = '-';
    }

    // reverse
    for(i32 i = 0; i < d ; ++i) {
        dest[d-i-1 + offset] = rev[i];
    }

    dest[d + offset] = 0; // null terminate
}

/**
 * @brief float to string
 * @param fnum
 * @param dest
 * @param precision
 */
void f32ToStr(f32 fnum, char* dest, u32 precision = 5);

/**
 * @brief double to string
 * @param fnum
 * @param dest
 * @param precision
 */
void f64ToStr(f64 fnum, char* dest, u32 precision = 5);
#endif
//u32 strFormat(char* buff, const char* formatStr, ...);
//u32 vstrFormat(char* buff, const char* formatStr, va_list args);


template<u32 stackCount = 1>
struct String
{
    char _stackData[stackCount];
    IAllocator* allocator = MEM_CONTEXT.allocator;
    char* _data = _stackData;
    i32 _length = 0;
    i32 _capacity = stackCount;
    MemBlock _memBlock = NULL_MEMBLOCK;

    String() = default;

    String(const char* cstr) {
        set(cstr);
    }

    ~String() {
        destroy();
    }

    // deep copy
    template<u32 from_stackCount>
    String(const String<from_stackCount>& from) {
        set(from.c_str(), from.len());
    }

    template<u32 from_stackCount>
    String& operator=(const String<from_stackCount>& from) {
        set(from.c_str(), from.len());
        return *this;
    }

    /**
     * @brief Reserve new capacity
     * @param newCapacity
     */
    void reserve(i32 newCapacity) {
        if(newCapacity <= _capacity) {
            return;
        }

        _memBlock = allocator->reallocate(_memBlock, newCapacity);
        assert(_memBlock.ptr);
        _data = (char*)_memBlock.ptr;
        _capacity = newCapacity;
    }

    inline void _growToFit(i32 newLen) {
        newLen++;
        if(newLen < _capacity) return;
        i32 newCapacity = newLen > (_capacity * 2) ? newLen : (_capacity * 2);
        reserve(newCapacity);
    }

    void destroy() {
        _memBlock.dealloc();
        _memBlock = NULL_MEMBLOCK;
    }

    inline void clear() {
        _length = 0;
    }

    void set(const char* cstr, i32 len = -1) {
        if(!cstr) return;
        if(len == -1) len = strLen(cstr);
        _growToFit(len);
        memmove(_data, cstr, len);
        _length = len;
        _data[_length] = 0;
    }

    void setf(const char* fmt, ...) {
        if(!fmt) return;
        va_list args;
        va_start(args, fmt);
        i32 len = vsnprintf(0, 0, fmt, args);
        va_end(args);

        _growToFit(len);

        va_start(args, fmt);
        vsprintf(_data, fmt, args);
        va_end(args);

        _length = len;
        _data[_length] = 0;
    }

    void append(const char* cstr, i32 len = -1) {
        if(!cstr || len == 0) return;
        if(len == -1) len = strLen(cstr);
        _growToFit(_length + len);
        memmove(_data + _length, cstr, len);
        _length += len;
        _data[_length] = 0;
    }

    void appendf(const char* fmt, ...) {
        if(!fmt) return;
        va_list args;
        va_start(args, fmt);
        i32 len = vsnprintf(0, 0, fmt, args);
        va_end(args);

        _growToFit(_length + len);

        va_start(args, fmt);
        vsprintf(_data + _length, fmt, args);
        va_end(args);

        _length += len;
        _data[_length] = 0;
    }

    inline i32 len() const {
        return _length;
    }

    inline const char* c_str() const {
        return _data;
    }

    inline i32 capacity() const {
        return _capacity;
    }

    inline bool equals(const char* cstr, i32 len = -1) const {
        if(len == -1) len = strLen(cstr);
        return (_length == len && strCmp(_data, cstr, len) == -1);
    }

    template<u32 other_stackCount>
    inline bool equals(const String<other_stackCount>& str) const {
        return (_length == str._length && strCmp(_data, str._data, _length) == -1);
    }

    inline bool startsWith(const char* cstr, i32 len = -1) const {
        if(len == -1) len = strLen(cstr);
        if(_length >= len) {
            return (strCmp(_data, cstr, len) == -1);
        }
        return false;
    }

    inline bool endsWith(const char* cstr, i32 len = -1) const {
        if(len == -1) len = strLen(cstr);
        if(_length >= len) {
            return (strCmp(_data + (_length - len), cstr, len) == -1);
        }
        return false;
    }

    void replace(char searchChar, char replaceChar) {
        for(i32 i = 0; i < _length; ++i) {
            if(_data[i] == searchChar) {
                _data[i] = replaceChar;
            }
        }
    }

    inline void rstrip(i32 count) {
        if(count >= _length) {
            _length = 0;
        }
        else {
            _length -= count;
        }
        _data[_length] = 0;
    }

    inline String& operator+=(const char* cstr) {
        append(cstr);
        return *this;
    }

    template<u32 other_stackCount>
    inline String& operator+=(const String<other_stackCount>& str) {
        append(str.c_str(), str.len());
        return *this;
    }

    template<u32 other_stackCount>
    inline bool operator==(const String<other_stackCount>& str) const {
        return equals(str);
    }

    inline bool operator==(const char* cstr) const {
        return equals(cstr);
    }

    inline String& operator=(const char* cstr) {
        set(cstr);
        return *this;
    }
};

typedef String<32> String32;
typedef String<64> String64;
typedef String<128> String128;
typedef String<256> String256;
