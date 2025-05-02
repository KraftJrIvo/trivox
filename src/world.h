#include "types.hpp"

struct WorldState {
    ObjArena<vec3>          vertices;
    ObjArena<ShapeUV>       shUVs;
    ObjArena<ShapeMaterial> shMaterials;
    ObjArena<Shape>         shapes;
    ObjArena<Cell>          cells;
    ObjArena<Room>          rooms;
    ObjArena<RoomRef>       roomRefs;

    WorldState() :
    vertices(TRIVOX_MAX_TOTAL_VERTS),
    shUVs(TRIVOX_MAX_TOTAL_SHAPE_UVS),
    shMaterials(TRIVOX_MAX_TOTAL_SHAPE_MATERIALS),
    shapes(TRIVOX_MAX_TOTAL_SHAPES),
    cells(TRIVOX_MAX_TOTAL_CELLS),
    rooms(TRIVOX_MAX_ROOMS),
    roomRefs(TRIVOX_MAX_ROOMS)
    { }
};

struct WorldConfig {

};

class RendererImpl;

class World {
    friend class RendererImpl;
protected:
    WorldState _state;
public:
    WorldConfig cfg;
    World(const WorldConfig& cfg) : 
        cfg(cfg)
    { }

    using Ptr = std::shared_ptr<World>;
    static Ptr create(const WorldConfig& cfg);
};