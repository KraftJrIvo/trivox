#pragma once

#include "base.h"

struct BRDF {
    
    DECLARE_PTR_TYPE(BRDF)
};

struct Triangle {
    v3f   v;
    Color c;
    u8 mat_idx;
    u8 tex_idx;
    u8 nrm_idx;
    bool quad;
};

struct LightProbe {
    std::vector<v2f> dir;
};

struct Grid {
    float st;
    v3i   sz;

    std::vector<u8>         dsts;
    std::vector<Triangle>   tris;
    std::vector<LightProbe> lgts;

    DECLARE_PTR_TYPE(Grid);
};