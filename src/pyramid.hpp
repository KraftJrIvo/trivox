#pragma once

#include "base.h"
#include "config.h"

#include <array>

struct Cell {
    u32 distance = 0;
    u32 nShapes = 0;
    u32 shids[TRIVOX_MAX_SHAPES_PER_ROOM];

    void addShape(u32 shid) {
        shids[nShapes++] = shid;
    }
};

#define _LVLSZ_ ((1 << (size_t(MAX_LVL_) + 1)) - (1 << size_t(MIN_LVL_)))

template <u8 NROOMS, u8 MIN_LVL_, u8 MAX_LVL_>
class CellPyramid {
    const size_t LVLSZ = _LVLSZ_;
    std::array<Cell, NROOMS * _LVLSZ_> _cells;
public:
    const u8 MIN_LVL = MIN_LVL_;
    const u8 MAX_LVL = MAX_LVL_;
    Cell* data() {
        return _cells.data();
    }

    Cell& at(u8 room, u8 lvl, uvec3 pos) {
        float ncells = 1 << lvl;
        size_t roomoff = LVLSZ * room;
        size_t lvloff = ((1 << (size_t(lvl) + 1)) - (1 << size_t(MIN_LVL)));
        size_t celloff = ncells * ncells * pos.x() + ncells * pos.y() + pos.x();
        return _cells[roomoff + lvloff + celloff];
    }

    const Cell& get(u8 lvl, vec3 pos) const {
        return at(lvl, pos);
    }

    void clear() {
        _cells.fill(Cell{0});
    }

    void _trySetDistFromCell(u8 rid, u8 lvl, u32 ncells, Cell& curcell, const uvec3& otherPos) {
        if (otherPos.x() >= 0 && otherPos.x() < ncells && otherPos.y() >= 0 && otherPos.y() < ncells && otherPos.z() >= 0 && otherPos.z() < ncells) {
            auto otherCell = at(rid, lvl, otherPos);
            if (otherCell.distance > 0) {
                curcell.distance = otherCell.distance + 1;
            }
        }
    }

    void fillDistances(u8 rid) {
        for (u8 lvl = MIN_LVL; lvl <= MAX_LVL; ++lvl) {
            bool foundZeroDistCells = true;
            while (foundZeroDistCells) {
                foundZeroDistCells = false;
                u32 ncells = 1 << lvl;
                uvec3 pos = {0, 0, 0};
                for (; pos[0] < ncells; ++pos[0]) {
                    for (; pos[1] < ncells; ++pos[1]) {
                        for (; pos[2] < ncells; ++pos[2]) {
                            auto& cell = at(rid, lvl, pos);
                            if (cell.distance == 0) {
                                foundZeroDistCells = true;
                                _trySetDistFromCell(rid, lvl, ncells, cell, {pos[0] - 1, pos[1], pos[2]});
                                _trySetDistFromCell(rid, lvl, ncells, cell, {pos[0] + 1, pos[1], pos[2]});
                                _trySetDistFromCell(rid, lvl, ncells, cell, {pos[0], pos[1] - 1, pos[2]});
                                _trySetDistFromCell(rid, lvl, ncells, cell, {pos[0], pos[1] + 1, pos[2]});
                                _trySetDistFromCell(rid, lvl, ncells, cell, {pos[0], pos[1], pos[2] - 1});
                                _trySetDistFromCell(rid, lvl, ncells, cell, {pos[0], pos[1], pos[2] + 1});
                            }
                        }
                    }
                }
            }
        }
    }
};