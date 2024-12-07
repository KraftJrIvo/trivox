#pragma once

#include <iostream>
#include "raylib.h"

inline std::ostream& operator<<(std::ostream& os, Vector2 v) {
    os << v.x << ' ' <<  v.y;
    return os;
}

inline bool operator== (Vector2 v1, Vector2 v2) {
    return v1.x == v2.x && v1.y == v2.y;
}

inline Vector2 operator+ (Vector2 v1, Vector2 v2) {
    return { v1.x + v2.x, v1.y + v2.y };
}

inline Vector3 operator+ (const Vector3& v1, const Vector3& v2) {
    return { v1.x + v2.x, v1.y + v2.y, v1.z + v2.z };
}

inline Vector2 operator- (Vector2 v1, Vector2 v2) {
    return { v1.x - v2.x, v1.y - v2.y };
}

inline Vector2 operator- (Vector2 v) {
    return { -v.x, -v.y };
}

inline void operator+=(Vector2& v1, Vector2 v2) {
    v1.x += v2.x;
    v1.y += v2.y;
}

inline void operator-=(Vector2& v1, Vector2 v2) {
    v1.x -= v2.x;
    v1.y -= v2.y;
}

inline Vector2 operator* (Vector2 v, const float& coeff) {
    return { coeff * v.x, coeff * v.y };
}

inline Vector3 operator* (const Vector3& v, const float& coeff) {
    return { coeff * v.x, coeff * v.y, coeff * v.z };
}

inline Vector2 operator/ (Vector2 v, const float& coeff) {
    return { v.x / coeff, v.y / coeff };
}

inline Vector2 operator* (const float& coeff, Vector2 v) {
    return v * coeff;
}

inline Vector2 operator/ (const float& coeff, Vector2 v) {
    return v / coeff;
}

inline Vector2 operator* (Vector2 v1, Vector2 v2) {
    return { v1.x * v2.x, v1.y * v2.y };
}

inline Vector2 operator/ (Vector2 v1, Vector2 v2) {
    return { v1.x / v2.x, v1.y / v2.y };
}

inline std::ostream& operator<<(std::ostream& os, const Vector3& v) {
    os << v.x << ' ' <<  v.y << ' ' <<  v.z;
    return os;
}

inline bool operator== (const Vector3& v1, const Vector3& v2) {
    return v1.x == v2.x && v1.y == v2.y && v1.z == v2.z;
}