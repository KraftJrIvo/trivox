#include "world.h"
#include "entity.hpp"
#include "shape.h"

#include <map>

class WorldImpl : public World {
    u64 _addRoom(const Room& room);
    u64 _addRoomRef(u8 rid, const mat4& matrix);
    u64 _addEntity(u8 rid, EntityType type, vec3 locpos, float params[TRIVOX_ENTITY_MAX_PARAMS] = nullptr);
    void _fillCells();
    void _fillCellDistances();
    void _fillRoomWithShapeIfInside(u8 lvl, const Shape& shape, u32 shid,
        const RoomRef& rr);
        
        public:
        WorldImpl(const WorldConfig& cfg);
        
        void update(float delta) override;
    };
    
    u64 WorldImpl::_addRoom(const Room& room) { return _state.rooms.acquire(room); }
    
    u64 WorldImpl::_addRoomRef(u8 rid, const mat4& matrix) {
        return _state.roomRefs.acquire(RoomRef((u32)rid, matrix));
    }
    
    u64 WorldImpl::_addEntity(u8 rrid, EntityType type, vec3 locpos, float params[TRIVOX_ENTITY_MAX_PARAMS]) {
        auto& rr = _state.roomRefs.at(rrid);
        mat4 matrix = rr.matrix();
        auto pos = (matrix.ROTMAT.inverse() * locpos) + matrix.POSVEC;
        auto ent = Entity{type, rrid, pos};
        if (params)
            for (int i = 0; i < TRIVOX_ENTITY_MAX_PARAMS; ++i)
                ent.params[i] = params[i];
        auto eid = _state.entities.acquire(ent);
        _state.entities.at(eid).init(rrid,& _state);
        return eid;
    }
    
    void WorldImpl::_fillCells() {
        for (u32 eid = 0; eid < _state.entities.count(); ++eid) {
            auto& ent = _state.entities.at(eid);
            std::unordered_map<u8, bool> roomRefDone;
            for (u8 lvl = _cells.MIN_LVL; lvl <= _cells.MAX_LVL; ++lvl) {
                for (u32 shid = ent.firstShapeIdx[lvl];
                    shid < ent.firstShapeIdx[lvl] + ent.nShapes(lvl); ++shid) {
                        auto& shape = _state.shapes.at(shid);
                        for (int rrid = 0; rrid < _state.roomRefs.count(); ++rrid) {
                            if (!roomRefDone.count(rrid)) {
                                auto& rr = _state.roomRefs.at(rrid);
                                _fillRoomWithShapeIfInside(lvl, shape, shid, rr);
                                roomRefDone[rr.idx] = true;
                            }
                        }
                    }
                }
            }
        }
        
        void WorldImpl::_fillRoomWithShapeIfInside(u8 lvl, const Shape& shape, u32 shid, const RoomRef& rr) {
            mat4 matrix = rr.matrix();
            std::array<vec3, 3> vs;
            auto vnum = shapeGetVnum(shape.type);
            auto pnum = shapeGetPnum(shape.type);
            for (int i = 0; i < vnum; ++i)
                vs[i] = (_state.vertices.get(shape.vIds[i]).v - matrix.POSVEC).transpose() * matrix.ROTMAT;
            for (int i = vnum; i < vnum + pnum; ++i)
                vs[i] = _state.vertices.get(shape.vIds[i]).v;
            auto& r = _state.rooms.at(rr.idx - 1);
            AABB raabb = AABB{vec3{0, 0, 0}, r.size};
            if (shapeCollidesAABB(shape.type, raabb, vs)) {
                AABB saabb = shapeGetAABB(shape.type, vs);
                float csz = 1 << (_cells.MAX_LVL - lvl);
                float maxcel = (1 << lvl) - 1;
                uvec3 startCell = {
                    (u32)floor(std::clamp(saabb.min.x() / csz, 0.f, maxcel)),
                    (u32)floor(std::clamp(saabb.min.y() / csz, 0.f, maxcel)),
                    (u32)floor(std::clamp(saabb.min.z() / csz, 0.f, maxcel))
                };
                uvec3 endCell = {
                    (u32)floor(std::clamp(saabb.max.x() / csz, 0.f, maxcel)),
                    (u32)floor(std::clamp(saabb.max.y() / csz, 0.f, maxcel)),
                    (u32)floor(std::clamp(saabb.max.z() / csz, 0.f, maxcel))
                };
                for (u32 x = startCell.x(); x <= endCell.x(); ++x) {
                    for (u32 y = startCell.y(); y <= endCell.y(); ++y) {
                        for (u32 z = startCell.z(); z <= endCell.z(); ++z) {
                            // AABB caabb = AABB{vec3{(float)x, (float)y, (float)z},
                            // vec3{x + csz, y + csz, z + csz}};
                            auto& cell = _cells.at(rr.idx - 1, lvl, {x, y, z});
                            cell.addShape(shid);
                        }
                    }
                }
            }
        }
                    
        void WorldImpl::_fillCellDistances() {
            for (u8 i = 0; i < _state.rooms.count(); ++i)
            _cells.fillDistances(i);
        }
                    
        WorldImpl::WorldImpl(const WorldConfig& cfg) : World(cfg) {
            auto rid = _addRoom(Room(vec3{8, 8, 8}));
            mat4 mat = mat4::Identity();
            mat.POSVEC += vec3{0, 0, 0};
            auto rrid0 = _addRoomRef(rid, mat);
            mat = mat4::Identity();
            mat.POSVEC += vec3{20, 20, 20};
            auto rrid1 = _addRoomRef(rid, mat);
            mat = mat4::Identity();
            mat.ROTMAT = Eigen::AngleAxisf(-PI / 8.f, vec3{1.0f, 0.0f, 0}).matrix() *
            Eigen::AngleAxisf(PI / 4.f, vec3{0, 1.0f, 0}).matrix();
            mat.POSVEC += vec3{8, 0, 0};
            auto rrid2 = _addRoomRef(rid, mat);
            _state.roomRefs.at(0).color = vec3{1.0, 0, 0};
            _state.roomRefs.at(1).color = vec3{1.0, 1.0, 0};
            _state.roomRefs.at(2).color = vec3{0, 0, 1.0};
            
            //for (int i = 0; i < 100; ++i)
            //    auto eid = _addEntity(rrid0, EntityType::BOUNCE_BALL, {RAND_FLOAT * 6 + 1, RAND_FLOAT * 6 + 1, RAND_FLOAT * 6 + 1});
            float radii[TRIVOX_ENTITY_MAX_PARAMS] = {0.5f};
            auto eid = _addEntity(rrid0, EntityType::BOUNCE_BALL, {4, 4, 4}, radii);
            radii[0] = 0.25f;
            //_state.shapes.at(_state.entities.at(eid).firstShapeIdx[0]).materialIdx = 1;
            for (int x = -1; x <= 1; x += 2) {
                for (int y = -1; y <= 1; y += 2) {
                    for (int z = -1; z <= 1; z += 2) {
                        auto eid = _addEntity(rrid0, EntityType::BOUNCE_BALL, {4.0f + x * 1.5f, 4.0f + y * 1.5f, 4.0f + z * 1.5f}, radii);
                        _state.shapes.at(_state.entities.at(eid).firstShapeIdx[0]).materialIdx = 1;
                        _state.shapes.at(_state.entities.at(eid).firstShapeIdx[0]).color = {float((x + 1) / 2), float((y + 1) / 2), float((z + 1) / 2)};
                    }
                }
            }

            float params[TRIVOX_ENTITY_MAX_PARAMS] = {1, 0, 0, 4};
            eid = _addEntity(rrid0, EntityType::PLANE, {0.001f, 4, 4}, params);
            params[1] = 1; params[0] = 0; 
            eid = _addEntity(rrid0, EntityType::PLANE, {4, 0.001f, 4}, params);
            params[2] = -1; params[1] = 0; 
            eid = _addEntity(rrid0, EntityType::PLANE, {4, 4, 8.0f - 0.001f}, params);
        }
        
        void WorldImpl::update(float delta) {
            for (u8 i = 0; i < _state.entities.count(); ++i)
                _state.entities.at(i).update(&_state, delta);
            
            _cells.clear();
            _fillCells();
            _fillCellDistances();
        }
        
        World::Ptr World::create(const WorldConfig& cfg) {
            return std::shared_ptr<World>(new WorldImpl(cfg));
        }
                    