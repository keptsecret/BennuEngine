#include <core/engine.h>

#include <graphics/renderingdevice.h>

namespace bennu {

void processInput(GLFWwindow* window, Camera& camera, float delta_time) {
	InputManager::update();

	Engine* engine = Engine::getSingleton();

	if (engine->getInputManager()->isButtonDown(GLFW_MOUSE_BUTTON_2)) { // hold right click to interact

		if (engine->getInputManager()->isKeyDown(GLFW_KEY_W)) {
			camera.translate(CameraMovement::Forward, delta_time);
		}
		if (engine->getInputManager()->isKeyDown(GLFW_KEY_S)) {
			camera.translate(CameraMovement::Backward, delta_time);
		}
		if (engine->getInputManager()->isKeyDown(GLFW_KEY_A)) {
			camera.translate(CameraMovement::Left, delta_time);
		}
		if (engine->getInputManager()->isKeyDown(GLFW_KEY_D)) {
			camera.translate(CameraMovement::Right, delta_time);
		}

		glm::vec2 mouse_offset = engine->getInputManager()->getMouseOffset();
		engine->getCamera()->pan(mouse_offset.x, mouse_offset.y);
	}
}

void Engine::initialize() {
	vkw::RenderingDevice* rd = vkw::RenderingDevice::getSingleton();
	rd->initialize();

	InputManager::init(rd->getWindow());
}

void Engine::renderLoop() {
	vkw::RenderingDevice* rd = vkw::RenderingDevice::getSingleton();

	float last_frame = 0.0f;

	while (!glfwWindowShouldClose(rd->getWindow())) {
		float current_frame = float(glfwGetTime());
		float delta_time = current_frame - last_frame;
		last_frame = current_frame;

		processInput(rd->getWindow(), viewCamera, delta_time);

		rd->render();

		glfwPollEvents();
	}
}

Engine::~Engine() {
}

Engine* Engine::getSingleton() {
	static Engine singleton;
	return &singleton;
}

} // bennu