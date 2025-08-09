#include "render.h"

#include <thread>

int main() {

	WorldConfig cfg = {};
	World::Ptr w = World::create(cfg);

	Renderer::Ptr r = Renderer::create(w, {1920, 1080});
	r->startRender();

	return 0;
}