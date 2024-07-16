#pragma once

#include "raylib.h"
#include <string>
#include <algorithm>
#include <Eigen/Core>
#include <Eigen/Geometry>

Vector3 Vector3OrthogonalVector(const Vector3& v);
Vector3 Vector3ProjectOntoPlane(const Vector3& o, const Vector3& normal, const Vector3& v);
bool Vector3InsideAABlock(const Vector3& bpos, const Vector3& bsize, const Vector3& v);
bool Vector3PlanesIntersection(const Vector3& p1, const Vector3& n1, const Vector3& p2, const Vector3& n2, Vector3& p, Vector3& n);
std::pair<Vector3, Vector3> Vector3ClosestPointsOnSegments(const Vector3& p11, const Vector3& p12, const Vector3& p21, const Vector3& p22);
Vector3 Vector3ClosestPointOnSegment(const Vector3& s, const Vector3& e, const Vector3& p);
Vector3 Vector3ClosestPointOnRectangle(Vector3 center, Vector3 normal, float width, float height, Vector3 point);
Vector3 Vector3ClosestPointOnBox(Vector3 center, Vector3 size, Vector3 v, Vector3* normal = nullptr);
Vector3 Vector3ClosestPointOnSphere(Vector3 center, float radius, Vector3 v);
Vector3 Vector3RotateByMatrix(const Vector3& v, const Matrix& m);
std::string Vector3ToString(const Vector3& v);
Vector3 fromEigen(const Eigen::Vector3d& v);
Eigen::Vector3d toEigen(const Vector3& v);
Eigen::Matrix4d toEigen(const Matrix& v);
Eigen::Quaterniond toEigen(const Quaternion& v);