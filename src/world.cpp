#include "world.h"

class WorldImpl : public World {
    u64 _addRoom(const Room& room);
    u64 _addRoomRef(u64 rid, const mat4& matrix);
public:
    WorldImpl(const WorldConfig& cfg);
};

WorldImpl::WorldImpl(const WorldConfig& cfg) :
    World(cfg)
{ 
    auto rid = _addRoom(Room(vec3{8, 8, 8}));
    mat4 mat = mat4::Identity();
    mat.TRAVEC += vec3{0, 0, 0};
    _addRoomRef(rid, mat);
    mat = mat4::Identity();
    mat.TRAVEC += vec3{20, 20, 20};
    _addRoomRef(rid, mat);
    mat = mat4::Identity();
    mat.ROTMAT = Eigen::AngleAxisf(-PI/8.f, vec3{1.0f, 0.0f, 0}).matrix() * Eigen::AngleAxisf(PI/4.f, vec3{0, 1.0f, 0}).matrix();
    mat.TRAVEC += vec3{8, 0, 0};
    _addRoomRef(rid, mat);
    _state.roomRefs.at(0).color = vec3{1.0, 0, 0};
    _state.roomRefs.at(1).color = vec3{1.0, 1.0, 0};
    _state.roomRefs.at(2).color = vec3{0, 0, 1.0};
}

u64 WorldImpl::_addRoom(const Room& room) {
    auto rid = _state.rooms.acquire(room);
    auto ncells = room.size.x() * room.size.y() * room.size.z();
    _state.rooms.at(rid).firstCellIdx = _state.cells.acquire(Cell(), ncells);
    return rid;
}

u64 WorldImpl::_addRoomRef(u64 rid, const mat4& matrix) {
    return _state.roomRefs.acquire(RoomRef((u32)rid, matrix));
}

World::Ptr World::create(const WorldConfig& cfg) {
    return std::shared_ptr<World>(new WorldImpl(cfg));
}