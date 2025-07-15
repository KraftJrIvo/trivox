#include "types.h"

int  shapeGetVnum(ShapeType type);
AABB shapeGetAABB(const ShapeType& shape, const std::array<vec3, 3>& vs);
bool shapeCollidesAABB(const ShapeType& shape, const AABB& aabb, const std::array<vec3, 3>& vs);