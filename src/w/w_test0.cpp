#include "../world.h"

class WorldTest0 : public World 
{
protected:
    WorldTest0()
    { }

public:
    virtual void init() override {
        World::init();
    }

    virtual void reset() override {
        World::reset();
    }

    virtual void update() override {
        World::update();
    }

    static World::Ptr create() {
        return std::shared_ptr<WorldTest0>(new WorldTest0());
    }
};

World::Ptr create_test0() {
    return WorldTest0::create();
}