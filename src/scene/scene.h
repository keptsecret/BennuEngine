#ifndef BENNU_SCENE_H
#define BENNU_SCENE_H

#include <graphics/vulkan/buffer.h>

#include <scene/light.h>
#include <scene/model.h>

namespace bennu {

struct GlobalUniforms {
	glm::mat4 view;
	glm::mat4 projection;
	glm::vec4 cameraPosition;
	float camNear;
	float camFar;
};

class Scene {
public:
	Scene() {}

	void loadModel(const std::string& filepath, uint32_t postProcessFlags = 0);

	void createDirectionalLight(glm::vec3 direction, glm::vec3 color, float intensity);
	void addPointLight(glm::vec3 position, glm::vec3 color, float radius, float intensity);
	void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout = VK_NULL_HANDLE, uint32_t renderFlags = 0, uint32_t bindImageset = 1);

	void updateSceneBufferData(bool rebuildBuffers = false);
	const vkw::UniformBuffer* getDirectionalLightBuffer() const { return directionalLightBuffer.get(); }
	const vkw::StorageBuffer* getPointLightsBuffer() const { return pointLightsBuffer.get(); }
	uint32_t getNumLights() const { return pointLights.size(); }

	void unload();

private:
	std::unique_ptr<Model> model;
	AABB bounds;

	std::vector<PointLight> pointLights;
	DirectionalLight directionalLight{glm::vec3{0, -1, 0}, glm::vec3{1.f}, 0.f};

	std::unique_ptr<vkw::UniformBuffer> directionalLightBuffer;
	std::unique_ptr<vkw::StorageBuffer> pointLightsBuffer;
};

}  // namespace bennu

#endif	// BENNU_SCENE_H
