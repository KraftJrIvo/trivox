#include "raylib.h"
#include "world.h"

#define SPEED 1.5f
#define COLORS std::vector<Color>({RED, ORANGE, YELLOW, GREEN, BLUE, PURPLE})

void initBounceBall(Entity* ent, u8 rrid, WorldState* ws) {
    u32 vid = ws->vertices.acquire({ent->pos});
    u32 vpid = ws->vertices.acquire({{ent->params[0], 0.0f, 0.0f}});
    u32 shid = ws->shapes.acquire(Shape{ShapeType::SPHERE, {vid, vpid, 0}, {1.0f, 0.0f, 0.0f}});
    for (u8 lvl = TRIVOX_MIN_LVL; lvl <= TRIVOX_MAX_LVL; ++lvl)
        ent->firstShapeIdx[lvl] = shid;
    ent->vel = vec3{RAND_FLOAT_SIGNED, RAND_FLOAT_SIGNED, RAND_FLOAT_SIGNED}.normalized();
    //ws->vertices.at(vpid).v[0] = 0.05f + 0.5f;// * RAND_FLOAT;
    //auto col = COLORS[GetRandomValue(0, COLORS.size() - 1)];
    //ws->shapes.at(shid).color =  {col.r / 255.f, col.g / 255.f, col.b / 255.f};
    ws->shapes.at(shid).color = {RAND_FLOAT, RAND_FLOAT, RAND_FLOAT};
    ws->shapes.at(shid).materialIdx = (rand() % 10 < 1) ? 1 : 0;
}

void updateBounceBall(Entity* ent, WorldState* ws, float delta) {
    auto& shape = ws->shapes.at(ent->firstShapeIdx[0]);
    auto& c = ws->vertices.at(shape.vIds[0]).v;
    c += delta * ent->vel * SPEED;
    const auto& rr = ws->roomRefs.get(ent->rrid);
    const auto& room = ws->rooms.get(rr.idx - 1);
    auto matrix = rr.matrix();
    vec3 locC = (c - matrix.POSVEC).transpose() * matrix.ROTMAT;
    vec3 locV = (ent->vel - matrix.POSVEC).transpose() * matrix.ROTMAT;
    vec3 sgn = {locV.x() / abs(locV.x()), locV.y() / abs(locV.y()), locV.z() / abs(locV.z())};
    auto R = ws->vertices.at(shape.vIds[1]).v.x() * 1.1f;
    
    static bool moving = true;
    if (IsKeyPressed(KEY_M)) moving = !moving;
    if (shape.materialIdx == 1 && moving)
        locV = vec3{
            ((locC.x() - R < 0) || ( locC.x() + R > room.size.x())) ? -locV.x() : locV.x(),
            ((locC.y() - R < 0) || ( locC.y() + R > room.size.y())) ? -locV.y() : locV.y(),
            ((locC.z() - R < 0) || ( locC.z() + R > room.size.z())) ? -locV.z() : locV.z()
        };
    else
        locV = {0,0,0};
    if (shape.materialIdx != 1)
        locV = vec3{
            IsKeyDown(KEY_LEFT) ? -1.1f : IsKeyDown(KEY_RIGHT) ? 1.1f : 0.0f,
            IsKeyDown(KEY_PERIOD) ? -1.1f : IsKeyDown(KEY_SLASH) ? 1.1f : 0.0f,
            IsKeyDown(KEY_UP) ? -1.1f : IsKeyDown(KEY_DOWN) ? 1.1f : 0.0f
        };
    ent->vel = (locV.transpose() * matrix.ROTMAT.inverse()).transpose() + matrix.POSVEC;
    locC = vec3{
        std::clamp(locC.x(), R, room.size.x() - R),
        std::clamp(locC.y(), R, room.size.y() - R),
        std::clamp(locC.z(), R, room.size.z() - R),
    };
    c = (locC.transpose() * matrix.ROTMAT.inverse()).transpose() + matrix.POSVEC;
}