#include "world.h"
#include "shape.h"

class WorldImpl : public World {
    u64 _addRoom(const Room& room);
    u64 _addRoomRef(u64 rid, const mat4& matrix);
    u64 _addEntity(const Entity& ent);
    void _fillCells();
    void _fillCellDistances();
    void _fillCellsWithEntity(u8 lvl, u8 eid);
    void _fillCellsWithShape(u8 lvl, u32 sid);
public:
    WorldImpl(const WorldConfig& cfg);

    void update() override;
};

WorldImpl::WorldImpl(const WorldConfig& cfg) :
    World(cfg)
{ 
    auto rid = _addRoom(Room(vec3{8, 8, 8}));
    mat4 mat = mat4::Identity();
    mat.POSVEC += vec3{0, 0, 0};
    _addRoomRef(rid, mat);
    mat = mat4::Identity();
    mat.POSVEC += vec3{20, 20, 20};
    _addRoomRef(rid, mat);
    mat = mat4::Identity();
    mat.ROTMAT = Eigen::AngleAxisf(-PI/8.f, vec3{1.0f, 0.0f, 0}).matrix() * Eigen::AngleAxisf(PI/4.f, vec3{0, 1.0f, 0}).matrix();
    mat.POSVEC += vec3{8, 0, 0};
    _addRoomRef(rid, mat);
    _state.roomRefs.at(0).color = vec3{1.0, 0, 0};
    _state.roomRefs.at(1).color = vec3{1.0, 1.0, 0};
    _state.roomRefs.at(2).color = vec3{0, 0, 1.0};
}

u64 WorldImpl::_addRoom(const Room& room) {
    return _state.rooms.acquire(room);
}

u64 WorldImpl::_addRoomRef(u64 rid, const mat4& matrix) {
    return _state.roomRefs.acquire(RoomRef((u32)rid, matrix));
}

void WorldImpl::_fillCells() {
    for (u8 lvl = TRIVOX_MIN_LVL; lvl <= TRIVOX_MAX_LVL; ++lvl)
        for (u8 i = 0; i < _state.entities.count(); ++i)
            _fillCellsWithEntity(lvl, i);
}

void WorldImpl::_fillCellsWithEntity(u8 lvl, u8 eid) {
    auto& ent = _state.entities.at(eid);
    for (u32 sid = ent.firstShapeIdx[lvl]; sid < ent.firstShapeIdx[lvl] + ent.nShapes[lvl]; ++sid)
    _fillCellsWithShape(lvl, sid);
}

void WorldImpl::_fillCellsWithShape(u8 lvl, u32 sid) {
    auto& shape = _state.shapes.at(sid);
    for (int i = 0; i < _state.roomRefs.count(); ++i) {
        auto& rr = _state.roomRefs.at(i);
        auto& r = _state.rooms.at(rr.idx);
        shapeFillRoomIfInside(lvl, shape, sid, _state.vertices, rr, r, _state.cells);
    }
}

void WorldImpl::_fillCellDistances() {
    for (u8 i = 0; i < _state.rooms.count(); ++i)
       _state.cells.fillDistances(i);
}

void WorldImpl::update() {
    _state.cells.clear();
    _fillCells();
    _fillCellDistances();
}

World::Ptr World::create(const WorldConfig& cfg) {
    return std::shared_ptr<World>(new WorldImpl(cfg));
}
