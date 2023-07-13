#include <core/engine.h>

#include <graphics/renderingdevice.h>

namespace bennu {

void Engine::initialize() {
	vkw::RenderingDevice* renderingDevice = vkw::RenderingDevice::getSingleton();
	renderingDevice->initialize();
}

void Engine::renderLoop() {
	vkw::RenderingDevice* renderingDevice = vkw::RenderingDevice::getSingleton();

	while (!glfwWindowShouldClose(renderingDevice->getWindow())) {
		glfwPollEvents();
		renderingDevice->render();
	}
}

Engine::~Engine() {
}

Engine* Engine::getSingleton() {
	static Engine singleton;
	return &singleton;
}

} // bennu