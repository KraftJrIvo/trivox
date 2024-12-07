#pragma once

#include "grid.h"

class Scene 
{
public:
    virtual void init() = 0;
    virtual void reset() = 0;
    virtual void update(float dt) = 0;
    virtual void fillGrid(Grid& g) = 0; 

    DECLARE_PTR_TYPE(Scene);
};

struct PosedScene {
    Pose pose;
    Scene::Ptr scene;

    DECLARE_PTR_TYPE(PosedScene);
};
typedef std::set<PosedScene::Ptr> Scenes;
