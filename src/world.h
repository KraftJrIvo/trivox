#pragma once

#include "types.h"

namespace trivox {

    class Renderer;

    struct WorldConfig {
    };

    class World {
        friend class Renderer;
    public:
        World(WorldConfig config) : _config(config) {
            _startTime = std::chrono::system_clock::now();
        }

        void generate();
        void update();

        void lock() {
            _lock.lock();
        }

        void unlock() {
            _lock.unlock();
        }

    private:
        WorldConfig _config;
        std::mutex _lock;

        std::chrono::system_clock::time_point _startTime;
        double _time = 0.0;
        float _dt = 0.0;
        bool _first = true;
    };   
}