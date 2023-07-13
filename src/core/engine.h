#ifndef BENNU_ENGINE_H
#define BENNU_ENGINE_H

#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>
#include <graphics/vulkancontext.h>

#include <memory>

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

private:
	void initialize();
	void renderLoop();

	std::unique_ptr<vkw::VulkanContext> context = nullptr;
	bool useValidationLayers = true;
};

}  // namespace bennu

#endif	// BENNU_ENGINE_H
