#include <array>
#include <cstring>

#include "base.h"
#include "memory.hpp"

#include "raylib.h"

#define TRIVOX_MAX_TOTAL_SHAPE_UVS       1024
#define TRIVOX_MAX_TOTAL_SHAPE_MATERIALS 256
#define TRIVOX_MAX_TOTAL_CELLS           65536
#define TRIVOX_MAX_SHAPES_PER_ROOM       256
#define TRIVOX_MAX_VERTS_PER_SHAPE       3
#define TRIVOX_MAX_ROOMS                 16
#define TRIVOX_MAX_TOTAL_SHAPES          TRIVOX_MAX_SHAPES_PER_ROOM * TRIVOX_MAX_ROOMS
#define TRIVOX_MAX_TOTAL_VERTS           TRIVOX_MAX_VERTS_PER_SHAPE * TRIVOX_MAX_TOTAL_SHAPES

enum ShapeType : u32 {
    NONE,
    POINT,
    SPHERE,
    SPRITE,
    LINE,
    TRIANGLE
};

// vec4 x 3 : rx ry rw rh | uv0x uv0y uv1x uv1y | uv2x uv2y null null
struct ShapeUV {
    Rectangle atlasRect;
    vec2 uv[3];
    vec2 _null;
};

// vec4 x 6 : tidx nidx null null | diff spec refl null | [refmat : 4 x vec4]
struct ShapeMaterial {
    u32 texUVidx;
    u32 normUVidx;
    u32 _null0;
    u32 _null1;
    float diffuse;
    float specular;
    float reflection;
    float luminance;
    Matrix reflectionMat;
};

// uvec3 x 2 : type vid0 vid1/param0 | vid2/param1 rgba matidx
struct Shape {
    ShapeType type;
    u32 vIds[TRIVOX_MAX_VERTS_PER_SHAPE];
    Color color;
    u32 materialIdx;
};

// uvec2 : shidx dist
struct Cell {
    u32 firstShapeIdx = 0;
    u32 distance      = 0;
};

// size pad | cellsz pad | fcid
struct Room {
    vec3 size       = {0, 0, 0};
    u32 firstCellIdx = 0;
    Room() = default;
    Room(vec3 size, u32 firstCellIdx = 0) : size(size), firstCellIdx(firstCellIdx) {}
};

// idx | [mat : 4 x vec4]
struct RoomRef {
    mat _matrix;
    vec3 color;
    u32 idx;
    RoomRef() = default;
    RoomRef(u32 idx, const mat4& mat) : idx(idx + 1), _matrix(mat) { }
    mat4 matrix() {
        mat4 res;
        memcpy(res.data(), &_matrix, sizeof(mat));
        return res;
    }
};