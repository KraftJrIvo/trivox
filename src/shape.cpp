#include "shape.h"

int shapeGetVnum(ShapeType type) {
    switch (type) {
    case ShapeType::POINT:
    case ShapeType::SPHERE:
        return 1;
    case ShapeType::PLANE:
    case ShapeType::LINE:
        return 2;
    case ShapeType::TRIANGLE:
    case ShapeType::QUAD:
        return 3;
    default:
        return 0;
    }
}

AABB shapeGetAABB(const ShapeType& type, const std::array<vec3, 3>& vs)
{
    switch (type) {
    case ShapeType::POINT:
        return AABB{vs[0], vs[0]};
    case ShapeType::LINE:
        return AABB{vs[0], vs[1]};
    case ShapeType::SPHERE: {
        float r = vs[1].x();
        return AABB{vs[0] - vec3{r, r, r}, vs[0] + vec3{r, r, r}};
    }
    case ShapeType::TRIANGLE:
        return AABB{
                vec3{
                    std::min(vs[0].x(), std::min(vs[1].x(), vs[2].x())), 
                    std::min(vs[0].y(), std::min(vs[1].y(), vs[2].y())), 
                    std::min(vs[0].z(), std::min(vs[1].z(), vs[2].z()))
                }, 
                vec3{
                    std::max(vs[0].x(), std::max(vs[1].x(), vs[2].x())), 
                    std::max(vs[0].y(), std::max(vs[1].y(), vs[2].y())), 
                    std::max(vs[0].z(), std::max(vs[1].z(), vs[2].z()))
                }
        };
    case ShapeType::QUAD:
        return AABB{
                vec3{
                    std::min(vs[0].x(), std::min(vs[1].x(), std::min(vs[2].x(), vs[3].x()))), 
                    std::min(vs[0].y(), std::min(vs[1].y(), std::min(vs[2].y(), vs[3].y()))), 
                    std::min(vs[0].z(), std::min(vs[1].z(), std::min(vs[2].z(), vs[3].z())))
                }, 
                vec3{
                    std::max(vs[0].x(), std::max(vs[1].x(), std::max(vs[2].x(), vs[3].x()))), 
                    std::max(vs[0].y(), std::max(vs[1].y(), std::max(vs[2].y(), vs[3].y()))), 
                    std::max(vs[0].z(), std::max(vs[1].z(), std::max(vs[2].z(), vs[3].z())))
                }
        };
    default:
        return AABB{};
    }
}

void shapeFillRoomIfInside(u8 lvl, const Shape& shape, u32 sid, const WorldVertices& vertices, const RoomRef& rr, const Room& r, WorldCells& cells) 
{
    mat4 matrix = rr.matrix();
    vec3 r0 = matrix.POSVEC;
    std::array<vec3, 3> vs;
    auto vnum = shapeGetVnum(shape.type);
    for (int i = 0; i < vnum; ++i)
        vs[i] = matrix.ROTMAT * (vertices.get(shape.vIds[i]) - r0);
    AABB raabb = AABB{vec3{0, 0, 0}, r.size};
    if (shapeCollidesAABB(shape.type, raabb, vs)) {
        AABB saabb = shapeGetAABB(shape.type, vs);
        float csz = 1 << (cells.MAX_LVL - lvl);
        uvec3 startCell = {(u32)floor((saabb.min.x() / csz)), (u32)floor((saabb.min.y() / csz)), (u32)floor((saabb.min.z() / csz))};
        uvec3 endCell = {(u32)floor((saabb.max.x() / csz)), (u32)floor((saabb.max.y() / csz)), (u32)floor((saabb.max.z() / csz))};
        for (u32 x = startCell.x(); x <= endCell.x(); ++x) {
            for (u32 y = startCell.y(); y <= endCell.y(); ++y) {
                for (u32 z = startCell.z(); z <= endCell.z(); ++z) {
                    //AABB caabb = AABB{vec3{(float)x, (float)y, (float)z}, vec3{x + csz, y + csz, z + csz}};
                    cells.at(rr.idx, lvl, {x, y, z}).addShape(sid);
                }
            }
        }
    }
}

bool shapeCollidesAABB(const ShapeType& type, const AABB& aabb, const std::array<vec3, 3>& vs) {
    switch (type) {
    case ShapeType::POINT: {
        const vec3& p = vs[0];
        return vs[0].x() >= aabb.min.x() && vs[0].x() <= aabb.max.x() &&
               vs[0].y() >= aabb.min.y() && vs[0].y() <= aabb.max.y() &&
               vs[0].z() >= aabb.min.z() && vs[0].z() <= aabb.max.z();
    }
    case ShapeType::SPHERE: {
        vec3 closest;
        const vec3& c = vs[0];
        closest[0] = std::max(aabb.min.x(), std::min(c.x(), aabb.max.x()));
        closest[1] = std::max(aabb.min.y(), std::min(c.y(), aabb.max.y()));
        closest[2] = std::max(aabb.min.z(), std::min(c.z(), aabb.max.z()));
        float r = vs[1].x();
        return (closest - c).squaredNorm() <= r * r;
    }
    case ShapeType::LINE: {
        const vec3& p1 = vs[0];
        const vec3& p2 = vs[0];
        vec3 dir = p2 - p1;
        float tmin = 0.0f, tmax = 1.0f;
        for (int i = 0; i < 3; ++i) {
            if (std::abs(dir[i]) < 1e-6) {
                if (p1[i] < aabb.min[i] || p1[i] > aabb.max[i])
                    return false;
            } else {
                float ood = 1.0f / dir[i];
                float t1 = (aabb.min[i] - p1[i]) * ood;
                float t2 = (aabb.max[i] - p1[i]) * ood;
                if (t1 > t2) std::swap(t1, t2);
                tmin = std::max(tmin, t1);
                tmax = std::min(tmax, t2);
                if (tmin > tmax) return false;
            }
        }
        return true;
    }
    case ShapeType::TRIANGLE: {
        vec3 edges[3] = { vs[1] - vs[0], vs[2] - vs[1], vs[0] - vs[2] };
        vec3 triNormal = edges[0].cross(edges[1]).normalized();
        vec3 aabbVerts[8];
        for (int i = 0; i < 8; ++i) {
            aabbVerts[i] = vec3{
                (i & 1) ? aabb.max.x() : aabb.min.x(),
                (i & 2) ? aabb.max.y() : aabb.min.y(),
                (i & 4) ? aabb.max.z() : aabb.min.z()
            };
        }
        vec3 axes[] = {
            vec3{1, 0, 0}, vec3{0, 1, 0}, vec3{0, 0, 1},
            triNormal,
            vec3{1, 0, 0}.cross(edges[0]), vec3{1, 0, 0}.cross(edges[1]), vec3{1, 0, 0}.cross(edges[2]),
            vec3{0, 1, 0}.cross(edges[0]), vec3{0, 1, 0}.cross(edges[1]), vec3{0, 1, 0}.cross(edges[2]),
            vec3{0, 0, 1}.cross(edges[0]), vec3{0, 0, 1}.cross(edges[1]), vec3{0, 0, 1}.cross(edges[2])
        };
        for (const auto& axis : axes) {
            if (axis.squaredNorm() < 1e-6) continue; // Skip degenerate axes
            float minTri = std::numeric_limits<float>::max();
            float maxTri = -std::numeric_limits<float>::max();
            float minAABB = std::numeric_limits<float>::max();
            float maxAABB = -std::numeric_limits<float>::max();
            for (const auto& v : {vs[0], vs[1], vs[2]}) {
                float proj = v.dot(axis);
                minTri = std::min(minTri, proj);
                maxTri = std::max(maxTri, proj);
            }
            for (const auto& v : aabbVerts) {
                float proj = v.dot(axis);
                minAABB = std::min(minAABB, proj);
                maxAABB = std::max(maxAABB, proj);
            }
            if (maxTri < minAABB || maxAABB < minTri)
                return false;
        }
        return true;
    }
    case ShapeType::QUAD: {
        vec3 edges[4];
        for (int i = 0; i < 4; ++i)
            edges[i] = vs[(i + 1) % 4] - vs[i];
        vec3 normal = edges[0].cross(edges[1]).normalized();
        vec3 aabbVerts[8];
        for (int i = 0; i < 8; ++i) {
            aabbVerts[i] = vec3{
                (i & 1) ? aabb.max.x() : aabb.min.x(),
                (i & 2) ? aabb.max.y() : aabb.min.y(),
                (i & 4) ? aabb.max.z() : aabb.min.z()
            };
        }
        vec3 axes[] = {
            vec3{1, 0, 0}, vec3{0, 1, 0}, vec3{0, 0, 1},
            normal,
            vec3{1, 0, 0}.cross(edges[0]), vec3{1, 0, 0}.cross(edges[1]),
            vec3{1, 0, 0}.cross(edges[2]), (vec3{1, 0, 0}.cross(edges[3])),
            vec3{0, 1, 0}.cross(edges[0]), vec3{0, 1, 0}.cross(edges[1]),
            vec3{0, 1, 0}.cross(edges[2]), (vec3{0, 1, 0}.cross(edges[3])),
            vec3{0, 0, 1}.cross(edges[0]), vec3{0, 0, 1}.cross(edges[1]),
            vec3{0, 0, 1}.cross(edges[2]), (vec3{0, 0, 1}.cross(edges[3]))
        };
        for (const auto& axis : axes) {
            if (axis.squaredNorm() < 1e-6) continue;
            float minShape = std::numeric_limits<float>::max();
            float maxShape = -std::numeric_limits<float>::max();
            float minAABB = std::numeric_limits<float>::max();
            float maxAABB = -std::numeric_limits<float>::max();
            for (int i = 0; i < 4; ++i) {
                float proj = vs[i].dot(axis);
                minShape = std::min(minShape, proj);
                maxShape = std::max(maxShape, proj);
            }
            for (const auto& v : aabbVerts) {
                float proj = v.dot(axis);
                minAABB = std::min(minAABB, proj);
                maxAABB = std::max(maxAABB, proj);
            }
            if (maxShape < minAABB || maxAABB < minShape)
                return false;
        }
        return true;
    }
    default:
        return false;
    }
}