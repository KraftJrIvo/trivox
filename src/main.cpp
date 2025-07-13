#include "render.h"

#include <thread>

int main() {

	WorldConfig cfg = {};
	World::Ptr w = World::create(cfg);

	Renderer::Ptr r = Renderer::create(w, {1920, 1080});
	r->startRender();

	while (!WindowShouldClose()) {
		w->update();
		std::this_thread::sleep_for(std::chrono::milliseconds(15));
	}

	return 0;
}