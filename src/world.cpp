#include "world.h"
#include "util.h"
#include "vec_funcs.h"

#include <raylib.h>
#include <raymath.h>

#include <algorithm>
#include <chrono>
#include <memory>
#include <sstream>

#define SUB_STEPS 4

using namespace trivox;

void World::generate() {
    
}

void World::update() {
    std::chrono::duration<double> seconds = std::chrono::system_clock::now() - _startTime;
    float dt = (seconds.count() - _time);
    _dt = dt;
    float subdt = dt / SUB_STEPS;
    _time = seconds.count();

    lock();


    unlock();
}

