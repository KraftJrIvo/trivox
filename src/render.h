#include "world.h"

class RendererImpl;

class Renderer 
{
public:
    virtual void startRender() {}
    using Ptr = std::shared_ptr<Renderer>;
    static Renderer::Ptr create(World::Ptr w, uvec2 sz);
};