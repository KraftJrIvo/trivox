#include "base.h"
#include "raylib.h"
#include "raymath.h"
#include "types.hpp"
#include <Eigen/src/Geometry/AngleAxis.h>

World::World() :
    vertices(TRIVOX_MAX_TOTAL_VERTS),
    shUVs(TRIVOX_MAX_TOTAL_SHAPE_UVS),
    shMaterials(TRIVOX_MAX_TOTAL_SHAPE_MATERIALS),
    shapes(TRIVOX_MAX_TOTAL_SHAPES),
    cells(TRIVOX_MAX_TOTAL_CELLS),
    rooms(TRIVOX_MAX_ROOMS),
    roomRefs(TRIVOX_MAX_ROOMS)
{ 
    auto rid = addRoom(Room(vec3{8, 8, 8}));
    mat4 mat = mat4::Identity();
    mat.TRAVEC += vec3{0, 0, 0};
    addRoomRef(rid, mat);
    mat = mat4::Identity();
    mat.TRAVEC += vec3{20, 20, 20};
    addRoomRef(rid, mat);
    mat = mat4::Identity();
    mat.ROTMAT = Eigen::AngleAxisf(-PI/8.f, vec3{1.0f, 0.0f, 0}).matrix() * Eigen::AngleAxisf(PI/4.f, vec3{0, 1.0f, 0}).matrix();
    mat.TRAVEC += vec3{8, 0, 0};
    addRoomRef(rid, mat);
    roomRefs.at(0).color = vec3{1.0, 0, 0};
    roomRefs.at(1).color = vec3{1.0, 1.0, 0};
    roomRefs.at(2).color = vec3{0, 0, 1.0};
}

u64 World::addRoom(const Room& room) {
    auto rid = rooms.acquire(room);
    auto ncells = room.size.x() * room.size.y() * room.size.z();
    rooms.at(rid).firstCellIdx = cells.acquire(Cell(), ncells);
    return rid;
}

u64 World::addRoomRef(u64 rid, const mat4& matrix) {
    return roomRefs.acquire(RoomRef((u32)rid, matrix));
}

void drawRoomGrid(vec3 A, vec3 B, vec3 C, vec3 D, int n1, int n2, vec3 campos, bool front) {
    auto center = A + (B - A) / 2 + (D - A) / 2;
    auto tocam = (campos - center).normalized();
    auto normal = ((B - A).cross((C - A))).normalized();
    if (front == (normal.dot(tocam) > 0)) {
        for (int i = 0; i <= n1; ++i) {
            auto st1 = i * (D - A) / ((float)n1);
            DrawLine3D(toray3(A + st1), toray3(B + st1), GRAY);
        }
        for (int i = 0; i <= n2; ++i) {
            auto st2 = i * (B - A) / ((float)n2);
            DrawLine3D(toray3(A + st2), toray3(D + st2), GRAY);
        }
    }
}

void World::drawRoomGrids(Vector3 campos, bool front) 
{    
    auto ecampos = fromray3(campos);

    for (int i = 0; i < roomRefs.count(); ++i) {

        auto& rr = roomRefs.at(i);
        auto& room = rooms.at(roomRefs.at(i).idx - 1);
        Room rrr = room;
        
        vec3 roomHalfSz = room.size * 0.5f;
        mat4 roomMat = rr.matrix();
        mat3 roomRot = roomMat.ROTMAT;
        vec3 roomCenter = roomMat.TRAVEC + roomMat.ROTMAT * roomHalfSz;
        
        vec3 A = roomCenter + roomRot * (-roomHalfSz.cwiseProduct(vec3{ 1, -1,  1}));
        vec3 B = roomCenter + roomRot * (-roomHalfSz.cwiseProduct(vec3{ 1, -1, -1}));
        vec3 C = roomCenter + roomRot * (-roomHalfSz.cwiseProduct(vec3{-1, -1,  1}));
        vec3 D = roomCenter + roomRot * (-roomHalfSz.cwiseProduct(vec3{-1, -1, -1}));
        vec3 E = roomCenter + roomRot * (-roomHalfSz.cwiseProduct(vec3{ 1,  1,  1}));
        vec3 F = roomCenter + roomRot * (-roomHalfSz.cwiseProduct(vec3{ 1,  1, -1}));
        vec3 G = roomCenter + roomRot * (-roomHalfSz.cwiseProduct(vec3{-1,  1,  1}));
        vec3 H = roomCenter + roomRot * (-roomHalfSz.cwiseProduct(vec3{-1,  1, -1}));
        
        drawRoomGrid(A, B, D, C, room.size.z(), room.size.x(), ecampos, front);
        drawRoomGrid(F, B, A, E, room.size.y(), room.size.z(), ecampos, front);
        drawRoomGrid(C, D, H, G, room.size.z(), room.size.y(), ecampos, front);
        drawRoomGrid(B, F, H, D, room.size.y(), room.size.x(), ecampos, front);
        drawRoomGrid(E, A, C, G, room.size.y(), room.size.x(), ecampos, front);
        drawRoomGrid(G, H, F, E, room.size.z(), room.size.x(), ecampos, front);
    }
}