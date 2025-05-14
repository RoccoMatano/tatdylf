////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2007-2025 Rocco Matano
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
////////////////////////////////////////////////////////////////////////////////
//
// tatdylf is a stripped-down Windows DHCP server whose sole task is to provide
// Basler GigE-Vision cameras with IP addresses from a class C private network.
// The limitation to this operational objective allows many simplifications
// that would be inadmissible in the general case. E.g. the processing of DHCP
// options and memory reservation for those can be reduced to just handling
// a handful of those.
//
////////////////////////////////////////////////////////////////////////////////

#pragma once

////////////////////////////////////////////////////////////////////////////////

// Do not care to support XP anymore and require at least Vista
#define _WIN32_WINNT _WIN32_WINNT_VISTA

// Do not warn about inet_ntoa and inet_addr
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#ifndef _MSC_VER
#error This has to be adapted for other compilers
#endif

#ifndef __cplusplus
#error C++ is required
#endif

#if _MSC_VER < 1800
#define nullptr NULL
#endif

// using winsock2.h requires to define WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
// since we defined WIN32_LEAN_AND_MEAN, we have to include shellapi.h
// explicitly.
#include <shellapi.h>

#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>

////////////////////////////////////////////////////////////////////////////////

#if defined(_MSC_VER) && (_MSC_VER < 1600)

typedef signed char        int8_t;
typedef short              int16_t;
typedef int                int32_t;
typedef long long          int64_t;
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;
#define INT8_MIN   (-127i8 - 1)
#define INT16_MIN  (-32767i16 - 1)
#define INT32_MIN  (-2147483647i32 - 1)
#define INT64_MIN  (-9223372036854775807i64 - 1)
#define INT8_MAX   127i8
#define INT16_MAX  32767i16
#define INT32_MAX  2147483647i32
#define INT64_MAX  9223372036854775807i64
#define UINT8_MAX  0xffui8
#define UINT16_MAX 0xffffui16
#define UINT32_MAX 0xffffffffui32
#define UINT64_MAX 0xffffffffffffffffui64

#ifdef _WIN64
    typedef unsigned long long  size_t;
    typedef long long           ptrdiff_t;
    typedef long long           intptr_t;
    typedef unsigned long long  uintptr_t;
    #define INTPTR_MIN          INT64_MIN
    #define INTPTR_MAX          INT64_MAX
    #define UINTPTR_MAX         UINT64_MAX
#else
    typedef unsigned int        size_t;
    typedef int                 ptrdiff_t;
    typedef int                 intptr_t;
    typedef unsigned int        uintptr_t;
    #define INTPTR_MIN          INT32_MIN
    #define INTPTR_MAX          INT32_MAX
    #define UINTPTR_MAX         UINT32_MAX
#endif

#else
#include <stdint.h>
#endif

////////////////////////////////////////////////////////////////////////////////


#include <intrin.h> // for __stosb and __movsb

template <class T> void FORCEINLINE zero_init(T& obj)
{
#if defined(_M_IX86) || defined(_M_AMD64)
    __stosb(reinterpret_cast<BYTE*>(&obj), 0, sizeof(obj));
#else
    memset(&obj, 0, sizeof(obj));
#endif
}

void FORCEINLINE mem_cpy(void* dst, const void* src, size_t size)
{
#if defined(_M_IX86) || defined(_M_AMD64)
    __movsb(static_cast<BYTE*>(dst), static_cast<const BYTE*>(src), size);
#else
    memcpy(dst, src, size);
#endif
}

////////////////////////////////////////////////////////////////////////////////

uint32_t inline sz_len(const char* str)
{
    uint32_t len = ~0U;
    do
    {
        ++len;
    }
    while (str[len] != 0);
    return len;
}

////////////////////////////////////////////////////////////////////////////////

inline char* sz_cpy(char* dst, const char* src)
{
    char* const result = dst;
    char c;
    do
    {
        c = *dst++ = *src++;
    }
    while (c != 0);
    return result;
}

////////////////////////////////////////////////////////////////////////////////

inline char* sz_chr(const char* searchee, int ch)
{
    const char cch = static_cast<char>(ch);
    while (*searchee && *searchee != cch)
    {
        searchee++;
    }
    return *searchee == cch ? const_cast<char*>(searchee) : nullptr;
}

////////////////////////////////////////////////////////////////////////////////

// Always terminate destination with zero (like lstrcpyn).
// Of course we cannot do that if len == 0.

inline char* sz_cpyn(char* dst, const char* src, uint32_t len)
{
    char* const result = dst;
    if (len != 0)
    {
        while (len--)
        {
            const char c = *dst++ = *src++;
            if (c == 0) return result;
        }
        *(--dst) = 0;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////
