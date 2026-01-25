#include "world.h"

void initPlane(Entity* ent, u8 rrid, WorldState* ws) {
    vec3 normal = vec3{ent->params[0], ent->params[1], ent->params[2]};
    float radius = ent->params[3];
    if (normal.squaredNorm() < 1e-6f)
        normal = vec3{0.0f, 1.0f, 0.0f};
    normal.normalize();
    vec3 axis = (std::abs(normal.x()) < 0.9f) ? vec3{1.0f, 0.0f, 0.0f} : vec3{0.0f, 1.0f, 0.0f};
    vec3 tangent = normal.cross(axis);
    if (tangent.squaredNorm() < 1e-6f)
        tangent = normal.cross(vec3{0.0f, 0.0f, 1.0f});
    tangent.normalize();
    vec3 bitangent = normal.cross(tangent).normalized();
    vec3 u = tangent * radius;
    vec3 v = bitangent * radius;
    vec3 center = ent->pos;
    u32 vid0 = ws->vertices.acquire({center + u - v});
    u32 vid1 = ws->vertices.acquire({center - u - v});
    u32 vid2 = ws->vertices.acquire({center - u + v});
    u32 shid = ws->shapes.acquire(Shape{ShapeType::QUAD, {vid0, vid1, vid2}, {1.0f, 1.0f, 1.0f}});
    for (u8 lvl = TRIVOX_MIN_LVL; lvl <= TRIVOX_MAX_LVL; ++lvl)
        ent->firstShapeIdx[lvl] = shid;
}

void updatePlane(Entity* ent, WorldState* ws, float delta) {
    // Plane entity does not need to be updated
}
