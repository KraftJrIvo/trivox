#include "renderer.h"


int main()
{
	trivox::WorldConfig cfgW;
	trivox::World w(cfgW);
	w.generate();

	trivox::RendererConfig cfgR{10, {0, 3}};
	trivox::Renderer r(w, cfgR, {1024.0f, 1024.0f}, "TRIVOX");

	while (!r.done) {
		w.update();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}