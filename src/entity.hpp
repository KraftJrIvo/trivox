#pragma once

#include "base.h"
#include "config.h"

class WorldState;

enum class EntityType {
    BOUNCE_BALL,
    ROT_BOX,
    INF_POINT
};

template <u8 MIN_LVL, u8 MAX_LVL>
struct EntityT {
    EntityType type;
    vec3 pos;
    float params[TRIVOX_ENTITY_MAX_PARAMS];
    std::array<u32, MAX_LVL-MIN_LVL+1> firstShapeIdx;
    
    u32 nShapes(u8 lvl);
    void init(WorldState* wsptr);
    void update(WorldState* wsptr);
};
typedef EntityT<TRIVOX_MIN_LVL, TRIVOX_MAX_LVL> Entity;

void initBounceBall(Entity*, WorldState*);
void initRotBox(Entity*, WorldState*);
void initInfPoint(Entity*, WorldState*);
void updateBounceBall(Entity*, WorldState*);
void updateRotBox(Entity*, WorldState*);
void updateInfPoint(Entity*, WorldState*);

inline void Entity::init(WorldState* wsptr) {
    switch (type) {
        case EntityType::BOUNCE_BALL:
            initBounceBall(this, wsptr);
            break;
        case EntityType::ROT_BOX:
            initRotBox(this, wsptr);
            break;
        case EntityType::INF_POINT:
            initInfPoint(this, wsptr);
            break;
        default:
            break;
    }
}

inline void Entity::update(WorldState* wsptr) {
    switch (type) {
        case EntityType::BOUNCE_BALL:
            updateBounceBall(this, wsptr);
            break;
        case EntityType::ROT_BOX:
            updateRotBox(this, wsptr);
            break;
        case EntityType::INF_POINT:
            updateInfPoint(this, wsptr);
            break;
        default:
            break;
    }   
}

inline u32 Entity::nShapes(u8 lvl) {
    switch (type) {
        case EntityType::BOUNCE_BALL:
            return 1;
        case EntityType::ROT_BOX:
            return 6;
        case EntityType::INF_POINT:
            return 1;
        default:
            return 0;
    }   
}