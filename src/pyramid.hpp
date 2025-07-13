#pragma once

#include "base.h"
#include "config.h"

#include <array>

struct Cell {
    u32 distance = 0;
    Arena<TRIVOX_MAX_SHAPES_PER_ROOM, u32> shIds;

    void addShape(u32 shId) {
        shIds.acquire(shId);
    }
};

template <u8 NROOMS, u8 MIN_LVL_, u8 MAX_LVL_>
class CellPyramid {
    const size_t LVLSZ = ((1 << (size_t(MAX_LVL_) + 1)) - (1 << size_t(MIN_LVL_)));
    std::array<Cell, NROOMS * ((1 << (size_t(MAX_LVL_) + 1)) - (1 << size_t(MIN_LVL_)))> _cells;
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
};