#include <scene/scene.h>

#include <assimp/Importer.hpp>
#include <iostream>

namespace bennu {

void Scene::loadModel(const std::string& filepath, uint32_t postProcessFlags) {
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(filepath, postProcessFlags);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		std::cerr << "ERROR::Model:loadFromFile: " << importer.GetErrorString() << '\n';
		return;
	}

	Model* newmodel = new Model();
	newmodel->loadFromAiScene(scene, filepath);
	model = std::unique_ptr<Model>(newmodel);

	bounds = model->bounds;
}

void Scene::createDirectionalLight(glm::vec3 direction, glm::vec3 color, float intensity) {
	directionalLight = DirectionalLight(direction, color, intensity);
}

void Scene::addPointLight(glm::vec3 position, glm::vec3 color, float radius, float intensity) {
	pointLights.emplace_back(position, color, radius, intensity);
}

void Scene::updateSceneBufferData(bool rebuildBuffers) {
	directionalLight.preprocess(bounds.centroid, bounds.radius);

	if (rebuildBuffers) {
		if (directionalLightBuffer) {
			directionalLightBuffer.reset();
		}
		if (pointLightsBuffer) {
			pointLightsBuffer.reset();
		}

		directionalLightBuffer = std::make_unique<vkw::UniformBuffer>(sizeof(DirectionalLight));
		pointLightsBuffer = std::make_unique<vkw::StorageBuffer>(sizeof(PointLight) * pointLights.size());
	}

	directionalLightBuffer->update(&directionalLight);
	pointLightsBuffer->update(pointLights.data());
}

void Scene::draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t bindImageset) {
	model->draw(commandBuffer, pipelineLayout, bindImageset);
}

void Scene::unload() {
	if (directionalLightBuffer) {
		directionalLightBuffer.reset();
	}
	if (pointLightsBuffer) {
		pointLightsBuffer.reset();
	}

	model.reset();
}

}  // namespace bennu