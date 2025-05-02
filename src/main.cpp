#include "types.hpp"

int main() {

	World w;

	Renderer r(w, {1920, 1080});
	r.startRender();

	return 0;
}