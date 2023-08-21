#ifndef BENNU_MODEL_H
#define BENNU_MODEL_H

#include <graphics/vulkan/buffer.h>

#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <core/math/aabb.h>
#include <scene/material.h>

namespace bennu {

struct Triangle {
	uint32_t firstIndex;
	uint32_t indexCount;	// should be 3
	Material* material;

	AABB bounds;
};

struct MeshPushConstants {
	glm::mat4 model;
};

struct Mesh {
	std::vector<std::shared_ptr<Triangle>> primitives;
	std::string name;

	MeshPushConstants pushConstants;

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

enum RenderFlag {
	None = 0x00000000,
	BindImages = 0x00000001,
	Opaque = 0x00000002,
	AlphaMask = 0x00000004
};

class Model {
public:
	Model() {}
	~Model();

	std::vector<std::shared_ptr<Node>> nodes;
	std::vector<Node*> linearNodes;

	std::vector<std::shared_ptr<Texture>> textures;
	std::vector<std::unique_ptr<Material>> materials;

	std::unique_ptr<vkw::Buffer> vertexBuffer;
	std::unique_ptr<vkw::Buffer> indexBuffer;

	AABB bounds;

	std::string path;

	void loadFromFile(const std::string& filepath, uint32_t postProcessFlags = 0);
	void loadFromAiScene(const aiScene* scene, const std::string& filepath);

	void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout = VK_NULL_HANDLE, uint32_t renderFlags = 0, uint32_t bindImageset = 1);

private:
	void loadMaterials(const aiScene* scene);
	std::shared_ptr<Texture> loadTexture(const aiMaterial* mat, aiTextureType type);
	void processNode(aiNode* node, const aiScene* scene, std::shared_ptr<Node> parent, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
	std::unique_ptr<Mesh> processMesh(aiMesh* mesh, const aiScene* scene, glm::mat4& transform, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);

	void updateModelBounds();
	void updateNodeBounds(Node* node, glm::vec3& pmin, glm::vec3& pmax);

	void drawNode(std::shared_ptr<Node> node, VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout = VK_NULL_HANDLE, uint32_t renderFlags = 0, uint32_t bindImageset = 1);
};

}  // namespace bennu

#endif	// BENNU_MODEL_H
