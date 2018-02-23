#pragma once

#include <stdint.h>
#include <sys/types.h>
#include "jni.h"
#include "stdbool.h"

typedef int8_t i8;
const i8 I8_MIN = INT8_MIN;
const i8 I8_MAX = INT8_MAX;

typedef uint8_t u8;
const u8 U8_MAX = UINT8_MAX;

typedef int16_t i16;
const i16 I16_MIN = INT16_MIN;
const i16 I16_MAX = INT16_MAX;

typedef uint16_t u16;
const u16 U16_MAX = UINT16_MAX;

typedef int32_t i32;
const i32 I32_MIN = INT32_MIN;
const i32 I32_MAX = INT32_MAX;

typedef uint32_t u32;
const u32 U32_MAX = UINT32_MAX;

typedef int64_t i64;
const i64 I64_MIN = INT64_MIN;
const i64 I64_MAX = INT64_MAX;

typedef uint64_t u64;
const u64 U64_MAX = UINT64_MAX;

typedef size_t usize;
typedef ssize_t isize;

typedef float f32;
typedef double f64;
