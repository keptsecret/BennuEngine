#ifndef BENNU_MATERIAL_H
#define BENNU_MATERIAL_H

#include <graphics/buffer.h>
#include <graphics/texture.h>
#include <glm/gtc/type_ptr.hpp>

#include <memory>

namespace bennu {

struct Texture {
	std::unique_ptr<vkw::Texture> texture;

	std::string filepath;
	glm::vec4 constant;
	bool isConstantValue;
};

struct MaterialAux{
	uint32_t normalMapMode = 0;	// 0 = use vertex normals, 1 = use normal map, 2 = use bump map
	uint32_t roughnessGlossyMode = 0;	// 0 = roughness, 1 = glossy (value inverted  in shader)
};

class Material {
public:
	Material();
	~Material();

	glm::vec3 albedo{0.8f, 0.8f, 0.8f};
	float metallic = 0.f;
	float roughness = 0.5f;
	float ambient = 1.f;

	std::shared_ptr<Texture> albedoTexture = nullptr;
	std::shared_ptr<Texture> metallicTexture = nullptr;
	std::shared_ptr<Texture> roughnessTexture = nullptr;
	std::shared_ptr<Texture> ambientTexture = nullptr;

	std::shared_ptr<Texture> normalMap = nullptr;

	MaterialAux aux;
	vkw::UniformBuffer auxUbo;

	void apply();

	VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
	void createDescriptorSet(const VkDescriptorPool& descriptorPool, const VkDescriptorSetLayout& descriptorSetLayout, uint32_t descriptorBindingFlags);
};

}  // namespace bennu

#endif	// BENNU_MATERIAL_H
