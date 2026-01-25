#pragma once

#include "base.h"
#include "config.h"

class WorldState;

enum class EntityType { BOUNCE_BALL, PLANE, ROT_BOX, INF_POINT };

template <u8 MIN_LVL, u8 MAX_LVL> struct EntityT {
    EntityType type;
    u32 rrid;
    vec3 pos;
    vec3 vel;
    float params[TRIVOX_ENTITY_MAX_PARAMS];
    std::array<u32, MAX_LVL - MIN_LVL + 1> firstShapeIdx;
    bool exists = false;
    
    u32 nShapes(u8 lvl);
    void init(u8 rrid, WorldState *wsptr);
    void update(WorldState *wsptr, float delta);
};
typedef EntityT<TRIVOX_MIN_LVL, TRIVOX_MAX_LVL> Entity;

void initBounceBall(Entity *, u8, WorldState *);
void initPlane(Entity *, u8, WorldState *);
void initRotBox(Entity *, u8, WorldState *);
void initInfPoint(Entity *, u8, WorldState *);
void updateBounceBall(Entity *, WorldState *, float);
void updatePlane(Entity *, WorldState *, float);
void updateRotBox(Entity *, WorldState *, float);
void updateInfPoint(Entity *, WorldState *, float);

template <> inline void Entity::init(u8 rrid, WorldState *wsptr) {
    exists = true;
    switch (type) {
        case EntityType::BOUNCE_BALL: initBounceBall(this, rrid, wsptr); break;
        case EntityType::PLANE:       initPlane(this, rrid, wsptr); break;
        case EntityType::ROT_BOX:     initRotBox(this, rrid, wsptr); break;
        case EntityType::INF_POINT:   initInfPoint(this, rrid, wsptr); break;
        default: break;
    }
}

template <> inline void Entity::update(WorldState *wsptr, float delta) {
    switch (type) {
        case EntityType::BOUNCE_BALL: updateBounceBall(this, wsptr, delta); break;
        case EntityType::PLANE:       updatePlane(this, wsptr, delta); break;
        case EntityType::ROT_BOX:     updateRotBox(this, wsptr, delta); break;
        case EntityType::INF_POINT:   updateInfPoint(this, wsptr, delta); break;
        default: break;
    }
}

template <> inline u32 Entity::nShapes(u8 lvl) {
    switch (type) {
        case EntityType::BOUNCE_BALL: return 1;
        case EntityType::PLANE:       return 1;
        case EntityType::ROT_BOX:     return 6;
        case EntityType::INF_POINT:   return 1;
        default: return 0;
    }
}
