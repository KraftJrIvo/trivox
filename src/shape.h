#include "types.h"

int  shapeGetVnum(ShapeType type);
AABB shapeGetAABB(const ShapeType& shape, const std::array<vec3, 3>& vs);
void shapeFillRoomIfInside(u8 lvl, const Shape& shape, u32 sid, const WorldVertices& vertices, const RoomRef& rr, const Room& r, WorldCells& cells);
bool shapeCollidesAABB(const ShapeType& shape, const AABB& aabb, const std::array<vec3, 3>& vs);