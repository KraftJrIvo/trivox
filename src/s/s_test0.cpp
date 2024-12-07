#include "../scene.h"

class SceneTest0 : public Scene 
{
private:
    SceneTest0()
    { }

public:
    virtual void init() override;
    virtual void reset() override;
    virtual void update(float dt) override;
    virtual void fillGrid(Grid& g) override; 

    static Scene::Ptr create() {
        return std::shared_ptr<SceneTest0>(new SceneTest0());
    }
};