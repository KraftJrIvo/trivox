#include "base.h"
#include "raylib.h"
#include "raymath.h"
#include "types.hpp"

World::World() :
    vertices(TRIVOX_MAX_TOTAL_VERTS),
    shUVs(TRIVOX_MAX_TOTAL_SHAPE_UVS),
    shMaterials(TRIVOX_MAX_TOTAL_SHAPE_MATERIALS),
    shapes(TRIVOX_MAX_TOTAL_SHAPES),
    cells(TRIVOX_MAX_TOTAL_CELLS),
    rooms(TRIVOX_MAX_ROOMS),
    roomRefs(TRIVOX_MAX_ROOMS)
{ 
    auto rid = addRoom(Room{uvec3{10, 10, 10}, vec3{1.0f, 1.0f, 1.0f}});
    addRoomRef(rid, MatrixIdentity());
    addRoomRef(rid, MatrixTranslate(20, 20, 20));
    addRoomRef(rid, MatrixMultiply(MatrixRotateY(PI/4.f), MatrixTranslate(10, 0, 0)));
}

u64 World::addRoom(const Room& room) {
    auto rid = rooms.acquire(room);
    auto ncells = room.size.x * room.size.y * room.size.z;
    rooms.at(rid).firstCellIdx = cells.acquire(Cell(), ncells);
    return rid;
}

u64 World::addRoomRef(u64 rid, const Matrix& matrix) {
    return roomRefs.acquire(RoomRef((u32)rid, matrix));
}

void drawRoomGrid(Vector3 A, Vector3 B, Vector3 C, Vector3 D, int n1, int n2, Vector3 campos, bool front) {
    auto center = A + (B - A) / 2 + (D - A) / 2;
    auto tocam = Vector3Normalize(campos - center);
    auto normal = Vector3Normalize(Vector3CrossProduct((B - A), (C - A)));
    if (front == (Vector3DotProduct(normal, tocam) > 0)) {
        for (int i = 0; i <= n1; ++i) {
            auto st1 = i * (D - A) / ((float)n1);
            DrawLine3D(A + st1, B + st1, GRAY);
        }
        for (int i = 0; i <= n2; ++i) {
            auto st2 = i * (B - A) / ((float)n2);
            DrawLine3D(A + st2, D + st2, GRAY);
        }
    }
}

void World::drawRoomGrids(Vector3 campos, bool front) {

    for (int i = 0; i < roomRefs.count(); ++i) {

        auto& rr = roomRefs.at(i);
        auto& room = rooms.at(roomRefs.at(i).idx - 1);
        Room rrr = room;
        
        auto roomCenter = vec3{rr.matrix.m12,rr.matrix.m13,rr.matrix.m14};
        auto roomHalfSz = room.cellSz * vec3{(float)room.size.x, (float)room.size.y, (float)room.size.z} * 0.5f;
        auto roomQuat = QuaternionFromMatrix(rr.matrix);
        
        auto A = roomCenter + Vector3RotateByQuaternion(-roomHalfSz * vec3{ 1, -1,  1}, roomQuat);
        auto B = roomCenter + Vector3RotateByQuaternion(-roomHalfSz * vec3{ 1, -1, -1}, roomQuat);
        auto C = roomCenter + Vector3RotateByQuaternion(-roomHalfSz * vec3{-1, -1,  1}, roomQuat);
        auto D = roomCenter + Vector3RotateByQuaternion(-roomHalfSz * vec3{-1, -1, -1}, roomQuat);
        auto E = roomCenter + Vector3RotateByQuaternion(-roomHalfSz * vec3{ 1,  1,  1}, roomQuat);
        auto F = roomCenter + Vector3RotateByQuaternion(-roomHalfSz * vec3{ 1,  1, -1}, roomQuat);
        auto G = roomCenter + Vector3RotateByQuaternion(-roomHalfSz * vec3{-1,  1,  1}, roomQuat);
        auto H = roomCenter + Vector3RotateByQuaternion(-roomHalfSz * vec3{-1,  1, -1}, roomQuat);
        
        drawRoomGrid(A, B, D, C, room.size.z, room.size.x, campos, front);
        drawRoomGrid(F, B, A, E, room.size.y, room.size.z, campos, front);
        drawRoomGrid(C, D, H, G, room.size.z, room.size.y, campos, front);
        drawRoomGrid(B, F, H, D, room.size.y, room.size.x, campos, front);
        drawRoomGrid(E, A, C, G, room.size.y, room.size.x, campos, front);
        drawRoomGrid(G, H, F, E, room.size.z, room.size.x, campos, front);
    }
}