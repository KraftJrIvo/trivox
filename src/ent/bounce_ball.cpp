#include "world.h"

void initBounceBall(Entity* ent, WorldState* ws) {
    u32 vid = ws->vertices.acquire(ent->pos);
    u32 vpid = ws->vertices.acquire({1.0f, 0.0f, 0.0f});
    u32 shid = ws->shapes.acquire(Shape{ShapeType::SPHERE, {vid, vpid, 0}, {1.0f, 0.0f, 0.0f}});
    for (u8 lvl = TRIVOX_MIN_LVL; lvl <= TRIVOX_MAX_LVL; ++lvl)
        ent->firstShapeIdx[lvl] = shid;
}

void updateBounceBall(Entity* ent, WorldState* ws) {

}