#include "vec_funcs.h"
#include "raylib.h"
#include "raymath.h"

#include "vec_ops.h"
#include <cmath>
#include <string>
#include <algorithm>

Vector3 Vector3OrthogonalVector(const Vector3& v) {
    const float s = std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
    const float g = std::copysign(s, v.z);
    const float h = v.z + g;
    return {g * h - v.x * v.x, -v.x*v.y, -v.x*h};
}

Vector3 Vector3ProjectOntoPlane(const Vector3& o, const Vector3& normal, const Vector3& p) {
    auto v = p - o;
    double dist = Vector3DotProduct(v, normal);
    return p - dist * normal;
}

bool Vector3InsideAABlock(const Vector3& bpos, const Vector3& bsize, const Vector3& v) {
    return (bpos.x - 0.5f * bsize.x < v.x && bpos.x + 0.5f * bsize.x > v.x &&
            bpos.y - 0.5f * bsize.y < v.y && bpos.y + 0.5f * bsize.y > v.y &&
            bpos.z - 0.5f * bsize.z < v.z && bpos.z + 0.5f * bsize.z > v.z);
}

bool Vector3PlanesIntersection(const Vector3& p1, const Vector3& n1, const Vector3& p2, const Vector3& n2, Vector3& p, Vector3& n) {
    auto n3 = Vector3CrossProduct(n1, n2);
    const float det = Vector3LengthSqr(n3);
    if (det != 0) {
        p = -((Vector3CrossProduct(n3, n2) * Vector3DotProduct(n1, p1)) + (Vector3CrossProduct(n1, n3) * Vector3DotProduct(n2, p2))) / det;
        n = n3;
        return true;
    } else {
        p = p1;
        n = Vector3Normalize(p2 - p1);
        return true;
    }
}

std::pair<Vector3, Vector3> Vector3ClosestPointsOnSegments(const Vector3& p11, const Vector3& p12, const Vector3& p21, const Vector3& p22) {
    auto p1 = p11;
    auto p2 = p21;
    auto v1 = p12 - p11;
    auto v2 = p22 - p21;
    auto v21 = p2 - p1;
    auto v22 = Vector3DotProduct(v2, v2);
    auto v11 = Vector3DotProduct(v1, v1);
    auto v21_0 = Vector3DotProduct(v2, v1);
    auto v21_1 = Vector3DotProduct(v21, v1);
    auto v21_2 = Vector3DotProduct(v21, v2);
    float denom = v21_0 * v21_0 - v22 * v11;
    float s, t;
    if (denom == 0) {
        s = 0;
        t = (v11 * s - v21_1) / v21_0;
    } else {
        s = (v21_2 * v21_0 - v22 * v21_1) / denom;
        t = (-v21_1 * v21_0 + v11 * v21_2) / denom;
    }
    s = std::clamp(s, 0.0f, 1.0f);
    t = std::clamp(t, 0.0f, 1.0f);
    std::pair<Vector3, Vector3> res;
    res.first = p1 + s * v1; 
    res.second = p2 + t * v2;
    return res;
}


Vector3 Vector3ClosestPointOnSegment(const Vector3& s, const Vector3& e, const Vector3& p) {
    Vector3 dir = Vector3Normalize(e - s);
    Vector3 sp = p - s;
    float proj = std::clamp(Vector3DotProduct(sp, dir), 0.0f, Vector3Length(sp));
    Vector3 closestPoint = s + dir * proj;
    return closestPoint;
}

Vector3 Vector3ClosestPointOnSphere(Vector3 center, float radius, Vector3 point) {
    Vector3 v = Vector3Normalize(point - center);
    Vector3 closestPoint = center + v * radius;
    return closestPoint;
}

Vector3 Vector3ClosestPointOnRectangle(Vector3 center, Vector3 normal, float width, float height, Vector3 point) {
    auto proj = Vector3ProjectOntoPlane(center, normal, point);
    auto px = std::clamp((normal.x == 0) ? (proj.x - center.x) : (proj.z - center.z), -width * 0.5f, width * 0.5f);
    auto py = std::clamp((normal.y == 0) ? (proj.y - center.y) : (proj.z - center.z), -height * 0.5f, height * 0.5f);
    proj = center + Vector3{
        (normal.x == 0) ? px : 0, 
        (normal.y == 0) ? py : 0, 
        (normal.z != 0) ? 0 : (normal.x == 0) ? py : px
    };
    return proj;
}

Vector3 Vector3ClosestPointOnBox(Vector3 center, Vector3 size, Vector3 v, Vector3* normal) {
    Vector3 wd = {(v.x - center.x) / size.x, (v.y - center.y) / size.y, (v.z - center.z) / size.z};
    Vector3 dir = Vector3Normalize(wd);
    Vector3 bestNormal = Vector3Zero();
    float bestDot = -INFINITY;
    for (int i = 0; i < 6; ++i) {
        Vector3 normal = (float(i % 2) * 2.0f - 1.0f) * Vector3{float(i < 2),float(1 < i && i < 4),float(i > 3)};
        auto dot = Vector3DotProduct(dir, normal);
        if (dot > bestDot) {
            bestDot = dot;
            bestNormal = normal;
        }
    }
    auto halfSide = fabs(Vector3DotProduct(bestNormal, size) * 0.5);
    auto cSide = center + halfSide * bestNormal;
    auto w = (bestNormal.x == 0) ? size.x : size.z;
    auto h = (bestNormal.y == 0) ? size.y : size.z;
    auto proj = Vector3ClosestPointOnRectangle(cSide, bestNormal, w, h, v);
    if (normal)
        *normal = bestNormal;
    return proj;
}

Vector3 Vector3RotateByMatrix(const Vector3 &v, const Matrix &m) {
    Matrix vm = MatrixIdentity();
    vm.m3 = v.x; vm.m7 = v.y; vm.m11 = v.z;
    auto mul = MatrixMultiply(m, vm);
    return { mul.m3, mul.m7, mul.m11 };
}

std::string Vector3ToString(const Vector3& v) {
    return "[" + std::to_string(v.x) + "," + std::to_string(v.y) + "," + std::to_string(v.z) + "]";
}

Vector3 fromEigen(const Eigen::Vector3d& v) {
    return Vector3{(float)v.x(), (float)v.y(), (float)v.z()};
}

Eigen::Vector3d toEigen(const Vector3& v) {
    return Eigen::Vector3d(v.x, v.y, v.z);
}

Eigen::Matrix4d toEigen(const Matrix& v) {
    Eigen::Matrix4d mat;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            mat(i, j) = at(v, i, j);
    return mat;
}

Eigen::Quaterniond toEigen(const Quaternion& v) {
    Eigen::Quaterniond q;
    q.w() = v.w;
    q.x() = v.x;
    q.y() = v.y;
    q.z() = v.z;
    return q;
}