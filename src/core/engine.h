#ifndef BENNU_ENGINE_H
#define BENNU_ENGINE_H

#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>

#include <scene/camera.h>
#include <core/inputmanager.h>

namespace bennu {

class Engine {
protected:
	Engine() {}

public:
	~Engine();
	static Engine* getSingleton();

	bool isValidationLayersEnabled() { return useValidationLayers; }

	void run() {
		initialize();
		renderLoop();
	}

	Camera* getCamera() { return &viewCamera; }
	InputManager* getInputManager() { return &inputManager; }

private:
	void initialize();
	void renderLoop();

	Camera viewCamera;
	InputManager inputManager;

	bool useValidationLayers = true;
};

}  // namespace bennu

#endif	// BENNU_ENGINE_H
