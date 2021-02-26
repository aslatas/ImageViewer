#ifndef _ENGINECORE_H
#define _ENGINECORE_H

#define _CRT_SECURE_NO_WARNINGS
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

// Integer typedefs.
#define U8_MAX UINT8_MAX
#define U16_MAX UINT16_MAX
#define U32_MAX UINT32_MAX
#define U64_MAX UINT64_MAX
#define S8_MAX INT8_MAX
#define S16_MAX INT16_MAX
#define S32_MAX INT32_MAX
#define S64_MAX INT64_MAX
#define OUTPUT_BUFFER_SIZE 2048

typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16;
typedef int16_t s16;
//typedef uint32_t u32;
typedef unsigned int u32;
typedef int32_t s32;
typedef uint64_t u64;
typedef int64_t s64;

#define ARRAYCOUNT(x) (sizeof(x) / sizeof(x[0]))

// TODO(Matt): Do these without punting to cstdlib.
#define StrLen(string) strlen((string))
#define StrPrintF(buffer, size, format, ...) snprintf((buffer), (size), (format), __VA_ARGS__)

// Static buffer for printf calls.
static char OUTPUT_BUFFER[OUTPUT_BUFFER_SIZE];

// Print a string to stdout.
#define Print(string) Platform::PrintMessage((string))
// Formatted print to stdout, limited to OUTPUT_BUFFER_SIZE in length.
#define PrintF(format, ...)                                                \
{                                                                      \
StrPrintF(OUTPUT_BUFFER, OUTPUT_BUFFER_SIZE, format, __VA_ARGS__); \
Print(OUTPUT_BUFFER);                                              \
}

// These do the same as Print and PrintF, they just output to stderr instead.
#define ErrPrint(string) Platform::PrintError((string))
#define ErrPrintF(format, ...)                                             \
{                                                                      \
StrPrintF(OUTPUT_BUFFER, OUTPUT_BUFFER_SIZE, format, __VA_ARGS__); \
ErrPrint(OUTPUT_BUFFER);                                           \
}

// Assert macros. TODO(Matt): Use function overloading to allow both custom message and straight asserts.
#ifndef NDEBUG
#define Assert(x)                                                                      \
{                                                                                  \
if (!(x))                                                                      \
{                                                                              \
if (Platform::ShowAssertDialog(#x, __FILE__, (u32)__LINE__)) DebugBreak(); \
}                                                                              \
}
#else
#define Assert(x)
#endif // NDEBUG

#ifndef NDEBUG
#define AssertCustom(x, message)                                                            \
{                                                                                       \
if (!(x))                                                                           \
{                                                                                   \
if (Platform::ShowAssertDialog(message, __FILE__, (u32)__LINE__)) DebugBreak(); \
}                                                                                   \
}
#else
#define AssertCustom(x, message)
#endif // NDEBUG

#include "stb_ds.h"
#include "stb_image.h"
#include "stb_image_write.h"
#include "GMath.h"

// Helper for bitfield enum types. Takes an enum name (x) and underlying type
// (y), and overloads the bitwise operators.
#define BITFIELD_ENUM(x, y) enum struct x : y;\
x operator|(x a, x b) { return static_cast<x>(static_cast<y>(a) | static_cast<y>(b)); }\
x operator&(x a, x b) { return static_cast<x>(static_cast<y>(a) & static_cast<y>(b)); }\
x operator^(x a, x b) { return static_cast<x>(static_cast<y>(a) ^ static_cast<y>(b)); }\
x operator~(x a) { return static_cast<x>(~static_cast<y>(a)); }\
x##& operator|=(x##& a,x b) { return a = a | b; }\
x##& operator&=(x##& a,x b) { return a = a & b; }\
x##& operator^=(x##& a,x b) { return a = a ^ b; }\
bool operator==(y a, x b) {return static_cast<x>(a) == b;}\
bool operator==(x a, y b) {return static_cast<x>(b) == a;}\
bool operator!=(y a, x b) {return static_cast<x>(a) != b;}\
bool operator!=(x a, y b) {return static_cast<x>(b) != a;}\
enum struct x : y

#endif // _ENGINECORE_H