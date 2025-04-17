#pragma once

#include "raylib.h"
#include <cstdint>

#include "vec_ops.h"

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;

typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t  i8;

typedef double f64;
typedef float  f32;
typedef short  f16;

typedef Vector2 vec2;
typedef Vector3 vec3;

struct uvec3 {
    u8 x, y, z;
};