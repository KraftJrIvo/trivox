#pragma once

#include "raylib.h"
#include "scene.h"

class World {
protected:
    float _start, _time = 0;
    Scenes _scenes;

public:

    float time() {
        return _time;
    }

    const Scenes& scenes() {
        return _scenes;
    }

    virtual void init() {
        _start = GetTime();
    };

    virtual void reset() {
        init();
    }

    virtual void update() {
        _time = GetTime() - _start;
    }

    DECLARE_PTR_TYPE(World);
};

World::Ptr create_test0();