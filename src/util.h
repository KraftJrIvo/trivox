#pragma once

#include "types.h"

#include "raylib.h"

namespace trivox::util {
    
    float randFloat();
    Color getUniqueColorById(unsigned int id);
    bool valsAreClose(float val1, float val2);
}

