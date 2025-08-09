#pragma once

#include "base.h"
#include "config.h"
#include "raymath.h"

#include <array>

struct Cell {
    u32 shids[TRIVOX_MAX_SHAPES_PER_ROOM];
    u32 distance = 0;
    u32 nShapes = 0;
    u32 n1, n2;
    
    void addShape(u32 shid) {
        distance = 1;
        shids[nShapes++] = shid;
    }
};

#define _ROOMSZ_ (1 << (MIN_LVL_ * 3)) * ((1 << ((MAX_LVL_ - MIN_LVL_ + 1) * 3)) - 1) / (8 - 1)

template <u8 NROOMS, u8 MIN_LVL_, u8 MAX_LVL_> class CellPyramid {
    const size_t ROOMSZ = _ROOMSZ_;
    std::array<Cell, NROOMS * _ROOMSZ_> _cells;
    
    public:
    const u8 MIN_LVL = MIN_LVL_;
    const u8 MAX_LVL = MAX_LVL_;
    Cell *data() { return _cells.data(); }
    
    size_t size() { return _cells.size() * sizeof(Cell); }
    
    Cell &at(u8 room, u8 lvl, uvec3 pos) {
        float ncells = 1 << lvl;
        size_t roomoff = ROOMSZ * room;
        size_t lvloff = std::pow(8, MIN_LVL_) * (std::pow(8, (lvl - 1) - MIN_LVL_ + 1) - 1) / (8 - 1);
        size_t celloff = ncells * ncells * pos.z() + ncells * pos.y() + pos.x();
        return _cells[roomoff + lvloff + celloff];
    }
    
    const Cell &get(u8 lvl, vec3 pos) const { return at(lvl, pos); }
    
    void clear() { _cells.fill(Cell{0}); }
    
    void _trySetDistFromCell(u8 rid, u8 lvl, u32 ncells, Cell &curcell, int x,
        int y, int z) {
            if (x >= 0 && x < ncells && y >= 0 && y < ncells && z >= 0 && z < ncells) {
                auto otherCell = at(rid, lvl, {(u32)x, (u32)y, (u32)z});
                if (otherCell.distance > 0) {
                    curcell.distance = otherCell.distance + 1;
                }
            }
        }
        
        void fillDistances(u8 rid) {
            for (u8 lvl = MIN_LVL; lvl <= MAX_LVL; ++lvl) {
                bool foundZeroDistCells = true;
                bool foundNonZeroDistCells = true;
                while (foundZeroDistCells && foundNonZeroDistCells) {
                    foundZeroDistCells = false;
                    foundNonZeroDistCells = false;
                    u32 ncells = 1 << lvl;
                    for (int x = 0; x < ncells; ++x) {
                        for (int y = 0; y < ncells; ++y) {
                            for (int z = 0; z < ncells; ++z) {
                                auto &cell = at(rid, lvl, {(u32)x, (u32)y, (u32)z});
                                if (cell.distance == 0) {
                                    foundZeroDistCells = true;
                                    _trySetDistFromCell(rid, lvl, ncells, cell, x - 1, y, z);
                                    _trySetDistFromCell(rid, lvl, ncells, cell, x + 1, y, z);
                                    _trySetDistFromCell(rid, lvl, ncells, cell, x, y - 1, z);
                                    _trySetDistFromCell(rid, lvl, ncells, cell, x, y + 1, z);
                                    _trySetDistFromCell(rid, lvl, ncells, cell, x, y, z - 1);
                                    _trySetDistFromCell(rid, lvl, ncells, cell, x, y, z + 1);
                                } else {
                                    foundNonZeroDistCells = true;
                                }
                            }
                        }
                    }
                }
            }
        }
    };
    