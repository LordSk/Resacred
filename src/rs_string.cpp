#include "rs_string.h"

i32 strCmp(const char* str1, const char* str2, u32 length)
{
    for(u32 i = 0; i < length; ++i) {
        if(str1[i] != str2[i]) {
            return i;
        }
    }
    return -1;
}

bool strEq(const char* str1, const char* str2)
{
    i32 length =  strLen(str1);
    if(length != strLen(str2)) {
        return false;
    }
    return (memcmp(str1, str2, length) == 0);
}


/*void toLower(char* dest, const char* src, u32 length)
{
    // TODO: implement this
    for(u32 i = 0; i < length; ++i) {

    }
}*/

u32 strHash(const char* src, u32 length)
{
    // Jenkins One At A Time Hash
    u32 hash = 0;

    for(u32 i = 0; i < length; i++)
    {
        hash += src[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }

    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}

i64 parseI64(const char* str, i32 strLength)
{
    if(strLength < 0) {
        strLength = strLen(str);
    }

    // −9,223,372,036,854,775,808 to 9,223,372,036,854,775,807
    // 19
    constexpr u32 maxDigits = 19;
    i64 result = 0;
    bool negative = false;
    i32 offset = 0;

    if(str[0] == '-') {
        negative = true;
        offset = 1;
    }

    i32 start = min((u32)strLength, maxDigits) - 1;

    for(i32 i = start; i >= offset; --i) {
        result += (i64)pow(10.0, start - i) * (i64)(str[i] - '0');
    }

    if(negative) {
        return -result;
    }

    return result;
}

f64 parseF64(const char* str, i32 strLength)
{
    if(strLength < 0) {
        strLength = strLen(str);
    }

    i32 dotPos = -1;
    for(i32 i = 0; i < strLength && dotPos == -1; ++i) {
        if(str[i] == '.') {
            dotPos = i;
        }
    }

    bool negative = false;
    if(str[0] == '-') {
        negative = true;
    }

    if(dotPos == -1) {
        return parseI64(str, strLength);
    }

    i64 wholePart = parseI64(str, dotPos);
    i64 fractPart = parseI64(str + dotPos + 1, strLength - dotPos - 1);
    i32 nc = log10(fractPart) + 1;

    // fract part first zeroes (0.00xx)
    i32 i = 0;
    while(*(str + dotPos + 1 + i) == '0') ++i;
    nc += i;

    i32 div = pow(10.f, nc);

    f64 result = wholePart;
    if(negative) {
        result -= (f64)fractPart / div;
    }
    else {
        result += (f64)fractPart / div;
    }

    return result;
}


void f32ToStr(f32 fnum, char* dest, u32 precision)
{
    f32 absFloating = abs(fnum);
    i32 wholePart = absFloating;
    i32 index = 0;

    // negative
    if(fnum < 0.f) {
        dest[index++] = '-';
    }

    if(wholePart == 0) {
        dest[index++] = '0';
    }
    else {
        i32 logVal = log10(wholePart);
        index += logVal;

        // read whole part
        for(i32 i = 0 ; i < logVal + 1 && index > -1; ++i) {
            u32 wt = pow(10.f, i + 1); // 10 100 1000...
            u32 reminder = wholePart % wt;
            u32 digit = reminder / (wt / 10);
            dest[index--] = digit + 48; // ASCII value
        }

        index += logVal + 2;
    }

    dest[index] = '.';

    f32 fracPart  = absFloating - wholePart;
    f32 curF = fracPart; // cursor

    // read fractional part
    for(u32 i = 0; i < precision; ++i) {
        f32 f10 = curF * 10; // multiply by 10 to extract 1st decimal from curF
        u32 digit = f10; // round to get only decimal (x.yzwxacb -> x)
        dest[index + i + 1] = digit + 48; // ASCII value
        curF = f10 - digit; // advance cursor (x.yzwxacb - x = 0.yzwxacb)
    }

    dest[index + precision + 1] = 0;
}

void f64ToStr(f64 fnum, char* dest, u32 precision)
{
    f64 absFloating = abs(fnum);
    i32 wholePart = absFloating;
    i32 index = 0;

    // negative
    if(fnum < 0.0) {
        dest[index++] = '-';
    }

    if(wholePart == 0) {
        dest[index++] = '0';
    }
    else {
        i32 logVal = log10(wholePart);
        index += logVal;

        // read whole part
        for(i32 i = 0 ; i < logVal + 1 && index > -1; ++i) {
            u32 wt = pow(10.f, i + 1); // 10 100 1000...
            u32 reminder = wholePart % wt;
            u32 digit = reminder / (wt / 10);
            dest[index--] = digit + 48; // ASCII value
        }

        index += logVal + 2;
    }

    dest[index] = '.';

    f64 fracPart  = absFloating - wholePart;
    f64 curF = fracPart; // cursor

    // read fractional part
    for(u32 i = 0; i < precision; ++i) {
        f64 f10 = curF * 10; // multiply by 10 to extract 1st decimal from curF
        u32 digit = f10; // round to get only decimal (x.yzwxacb -> x)
        dest[index + i + 1] = digit + 48; // ASCII value
        curF = f10 - digit; // advance cursor (x.yzwxacb - x = 0.yzwxacb)
    }

    dest[index + precision + 1] = 0;
}

void strAppendEx(char* str, u32* pSize, const char* toAppend, u32 length)
{
    memcpy(str + *pSize, toAppend, length);
    *pSize += length;
}
