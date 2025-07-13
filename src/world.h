#include "types.h"

struct WorldState {
    WorldVertices       vertices;
    WorldShapeUVs       shUVs;
    WorldShapeMaterials shMaterials;
    WorldShapes         shapes;
    WorldCells          cells;
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
public:
    WorldConfig cfg;
    World(const WorldConfig& cfg) : 
        cfg(cfg)
    { }

    virtual void update() = 0;

    using Ptr = std::shared_ptr<World>;
    static Ptr create(const WorldConfig& cfg);
};