#include "types.h"
#include <cmath>
#include <cstdint>
#include <iostream>

using namespace trivox;

RenderPyramid::RenderPyramid(std::pair<int8_t, int8_t> minMaxPow, uint32_t radius) :
    radius(radius), minMaxPow(minMaxPow)
{
    uint8_t nlevels = minMaxPow.second - minMaxPow.first + 1;
    levels.resize(nlevels);
    for (uint8_t i = 0; i < nlevels; ++i) {
        RenderPyramidLevel& lvl = levels[i];
        lvl.pow = uint8_t(minMaxPow.first + int8_t(i));
        lvl.rPrt = float(radius) / std::pow(2.0f, float(nlevels - i - 1));
        lvl.blkSz = std::pow(2.0f, (float)lvl.pow);
        lvl.r = ceil(lvl.rPrt / float(lvl.blkSz));
        lvl.sz = 2 * lvl.r + 1;
    }
    std::cout << "";
}