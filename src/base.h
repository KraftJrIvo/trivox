#pragma once

#include <cstdint>

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <cstring>
#include "vec_ops.h"
#include "raymath.h"

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

typedef Eigen::Vector2f                vec2;
typedef Eigen::Vector3f                vec3;
typedef Eigen::Vector4f                vec4;
typedef Eigen::Matrix<unsigned int, 2, 1> uvec2;
typedef Eigen::Matrix<unsigned int, 3, 1> uvec3;
typedef Eigen::Matrix<unsigned int, 4, 1> uvec4;

typedef Eigen::Matrix2f mat2;
typedef Eigen::Matrix3f mat3;
typedef Eigen::Matrix4f mat4;

inline Vector2 toray2(const vec2& v) { return Vector2{v.x(), v.y()}; }
inline Vector3 toray3(const vec3& v) { return Vector3{v.x(), v.y(), v.z()}; }
inline Vector4 toray4(const vec4& v) { return Vector4{v.x(), v.y(), v.z(), v.w()}; }
inline Matrix  toraym(const mat4& m) { 
    return Matrix{
        m( 0), m( 4), m( 8), m(12), 
        m( 1), m( 5), m( 9), m(13),
        m( 2), m( 6), m(10), m(14),
        m( 3), m( 7), m(11), m(15),
    }; 
}

inline vec2 fromray2(const Vector2& v) { return vec2{v.x, v.y}; }
inline vec3 fromray3(const Vector3& v) { return vec3{v.x, v.y, v.z}; }
inline vec4 fromray4(const Vector4& v) { return vec4{v.x, v.y, v.z, v.w}; }
inline mat4 fromraym(const Matrix& m) { 
    auto mm = MatrixTranspose(m);
    mat4 res;
    memcpy(res.data(), &mm, 16 * sizeof(float));
    return res;
}

struct mat {
    float c00, c01, c02, c03,
          c10, c11, c12, c13,
          c20, c21, c22, c23,
          c30, c31, c32, c33;
    mat() = default;
    mat(mat4 m) {
        memcpy(this, m.data(), 16 * sizeof(float));
    }
};

typedef Eigen::Quaternionf quat;

#define ROTMAT block<3,3>(0,0)
#define TRAVEC block<3,1>(0,3)