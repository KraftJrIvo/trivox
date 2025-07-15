#include "types.h"
#include "entity.hpp"

typedef Arena<TRIVOX_MAX_TOTAL_VERTS, vec3>                    WorldVertices;
typedef Arena<TRIVOX_MAX_TOTAL_SHAPE_UVS, ShapeUV>             WorldShapeUVs;
typedef Arena<TRIVOX_MAX_TOTAL_SHAPE_MATERIALS, ShapeMaterial> WorldShapeMaterials;
typedef Arena<TRIVOX_MAX_TOTAL_SHAPES, Shape>                  WorldShapes;
typedef Arena<TRIVOX_MAX_ROOMS, Room>                          WorldRooms;
typedef Arena<TRIVOX_MAX_ROOMS, RoomRef>                       WorldRoomRefs;
typedef Arena<TRIVOX_MAX_ENTITIES, Entity>                     WorldEntities;

typedef CellPyramid<TRIVOX_MAX_ROOMS, TRIVOX_MIN_LVL, TRIVOX_MAX_LVL> WorldCellPyramid;

struct WorldState {
    WorldVertices       vertices;
    WorldShapeUVs       shUVs;
    WorldShapeMaterials shMaterials;
    WorldShapes         shapes;
    WorldRooms          rooms;
    WorldRoomRefs       roomRefs;
    WorldEntities       entities;
};

struct WorldConfig {

};

class RendererImpl;

class World {
    friend class RendererImpl;
protected:
    WorldState _state;
    WorldCellPyramid _cells;
public:
    WorldConfig cfg;
    World(const WorldConfig& cfg) : 
        cfg(cfg)
    { }

    virtual void update() = 0;

    using Ptr = std::shared_ptr<World>;
    static Ptr create(const WorldConfig& cfg);
};