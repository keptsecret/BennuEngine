#ifndef BENNU_MODEL_H
#define BENNU_MODEL_H

#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <graphics/buffer.h>
#include <graphics/texture.h>

#include <memory>

namespace bennu {

struct Texture {
	std::unique_ptr<vkw::Texture> texture;
	std::string filepath;
};

struct Material {
	std::shared_ptr<Texture> albedoTexture = nullptr;
	// TODO: stub, move into separate class later

	VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
	void createDescriptorSet(const VkDescriptorPool& descriptorPool, const VkDescriptorSetLayout& descriptorSetLayout, uint32_t descriptorBindingFlags);
};

struct Triangle {
	uint32_t firstIndex;
	uint32_t indexCount;	// should be 3
	std::shared_ptr<Material> material;

	struct Bounds {
		glm::vec3 pMin{ FLT_MAX };
		glm::vec3 pMax{ -FLT_MAX };
		glm::vec3 size;
		glm::vec3 centroid;
		float radius;
	} bounds;

	void updateBounds(glm::vec3 pmin, glm::vec3 pmax);
};

struct Mesh {
	std::vector<std::shared_ptr<Triangle>> primitives;
	std::string name;

	vkw::UniformBuffer uniformBuffer;	// TODO: use push constants?

	Mesh(glm::mat4 matrix);
	~Mesh();
};

struct Node {
	Node* parent;
	uint32_t index;
	std::vector<std::shared_ptr<Node>> children;
	glm::mat4 transform;

	std::unique_ptr<Mesh> mesh;
	std::string name;

	glm::vec3 translation{};
	glm::vec3 scale{ 1.f };
	glm::vec3 rotation{};

	glm::mat4 getLocalTransform();
	glm::mat4 getWorldTransform();
	void update();
	~Node();
};

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 uv;
	glm::vec3 tangent;

	static VkVertexInputBindingDescription getBindingDescription(uint32_t binding);
	static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions(uint32_t binding);
};

class Model {
public:
	Model() {}
	~Model();

	std::vector<std::shared_ptr<Node>> nodes;
	std::vector<Node*> linearNodes;

	std::vector<std::shared_ptr<Texture>> textures;
	std::vector<std::shared_ptr<Material>> materials;

	std::unique_ptr<vkw::Buffer> vertexBuffer;
	std::unique_ptr<vkw::Buffer> indexBuffer;

	struct Bounds {
		glm::vec3 pMin{ FLT_MAX };
		glm::vec3 pMax{ -FLT_MAX };
		glm::vec3 size;
		glm::vec3 centroid;
		float radius;
	} bounds;

	std::string path;

	void loadFromFile(const std::string& filepath, uint32_t postProcessFlags = 0);
	void loadFromAiScene(const aiScene* scene, const std::string& filepath);

	void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout = VK_NULL_HANDLE, uint32_t bindImageset = 1);

private:
	void loadMaterials(const aiScene* scene);
	std::shared_ptr<Texture> loadTexture(const aiMaterial* mat, aiTextureType type);
	void processNode(aiNode* node, const aiScene* scene, std::shared_ptr<Node> parent, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
	std::unique_ptr<Mesh> processMesh(aiMesh* mesh, const aiScene* scene, glm::mat4& transform, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);

	void drawNode(std::shared_ptr<Node> node, VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout = VK_NULL_HANDLE, uint32_t bindImageset = 1);
};

}  // namespace bennu

#endif	// BENNU_MODEL_H
