#include <core/engine.h>

int main() {
    bennu::Engine* engine = bennu::Engine::getSingleton();
	engine->run();

    return 0;
}
