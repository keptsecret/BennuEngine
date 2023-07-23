#include <scene/model.h>

#include <graphics/utilities.h>
#include <graphics/renderingdevice.h>
#include <assimp/Importer.hpp>
#include <array>
#include <iostream>

#ifndef GLM_FORCE_RADIANS
#define GLM_FORCE_RADIANS
#endif
#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace bennu {

void MaterialPH::createDescriptorSet(const VkDescriptorPool& descriptorPool, const VkDescriptorSetLayout& descriptorSetLayout, uint32_t descriptorBindingFlags) {
	VkDevice device = vkw::RenderingDevice::getSingleton()->getDevice();

	VkDescriptorSetAllocateInfo allocateInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = descriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts = &descriptorSetLayout
	};
	CHECK_VKRESULT(vkAllocateDescriptorSets(device, &allocateInfo, &descriptorSet));

	std::vector<VkDescriptorImageInfo> imageDescriptors{};
	std::vector<VkWriteDescriptorSet> writeDescriptorSets{};
	if (albedoTexture) {
		VkDescriptorImageInfo imageInfo{
			.sampler = albedoTexture->texture->getSampler(),
			.imageView = albedoTexture->texture->getImageView(),
			.imageLayout = albedoTexture->texture->getLayout()
		};
		imageDescriptors.push_back(imageInfo);

		VkWriteDescriptorSet writeDescriptorSet{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = descriptorSet,
			.dstBinding = 0,	// TODO: check, depends on layout
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.pImageInfo = &imageInfo
		};
		writeDescriptorSets.push_back(writeDescriptorSet);
	}

	vkUpdateDescriptorSets(device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
}

void Triangle::updateBounds(glm::vec3 pmin, glm::vec3 pmax) {
	bounds.pMin = pmin;
	bounds.pMax = pmax;
	bounds.size = pmax - pmin;
	bounds.centroid = (pmax + pmin) * 0.5f;
	bounds.radius = glm::distance(pmin, pmax) * 0.5f;
}

Mesh::Mesh(glm::mat4 matrix) {
}

Mesh::~Mesh() {
}

glm::mat4 Node::getLocalTransform() {
	return glm::translate(translation) * glm::rotate(rotation.x, glm::vec3(1, 0, 0))
			* glm::rotate(rotation.y, glm::vec3(0, 1, 0)) * glm::rotate(rotation.z, glm::vec3(0, 0, 1))
			* glm::scale(scale);
}

glm::mat4 Node::getWorldTransform() {
	glm::mat4 m = getLocalTransform();
	auto p = parent;
	while (p) {
		m = p->getLocalTransform() * m;
		p = p->parent;
	}
	return m;
}

void Node::update() {
	if (mesh) {
		glm::mat4 m = getWorldTransform();
		mesh->pushConstants.model = m;
	}

	for (auto& child : children) {
		child->update();
	}
}

Node::~Node() {
}

VkVertexInputBindingDescription Vertex::getBindingDescription(uint32_t binding) {
	return VkVertexInputBindingDescription{
		.binding = binding,
		.stride = sizeof(Vertex),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
	};
}

std::array<VkVertexInputAttributeDescription, 4> Vertex::getAttributeDescriptions(uint32_t binding) {
	std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};

	attributeDescriptions[0].binding = binding;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Vertex, position);

	attributeDescriptions[1].binding = binding;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Vertex, normal);

	attributeDescriptions[2].binding = binding;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[2].offset = offsetof(Vertex, uv);

	attributeDescriptions[3].binding = binding;
	attributeDescriptions[3].location = 3;
	attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[3].offset = offsetof(Vertex, tangent);

	return attributeDescriptions;
}

void Model::loadFromFile(const std::string& filepath, uint32_t postProcessFlags) {
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(filepath, postProcessFlags);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		std::cerr << "ERROR::Model:loadFromFile: " << importer.GetErrorString() << '\n';
		return;
	}

	loadFromAiScene(scene, filepath);
}

void Model::loadFromAiScene(const aiScene* scene, const std::string& filepath) {
	path = filepath.substr(0, filepath.find_last_of('/'));

	loadMaterials(scene);

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	processNode(scene->mRootNode, scene, nullptr, vertices, indices);

	for (auto& node : linearNodes) {
		if (node->mesh) {
			node->update();
		}
	}

	uint32_t vertexBufferSize = vertices.size() * sizeof(Vertex);
	uint32_t indexBufferSize = indices.size() * sizeof(uint32_t);
	vkw::RenderingDevice* rd = vkw::RenderingDevice::getSingleton();

	// Vertex data
	{
		vkw::Buffer vertexStagingBuffer(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vertices.data());

		vertexBuffer = std::make_unique<vkw::Buffer>(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		VkCommandBuffer commandBuffer;
		CHECK_VKRESULT(rd->createCommandBuffer(&commandBuffer, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true));

		VkBufferCopy copyRegion{
			.size = vertexBufferSize
		};
		vkCmdCopyBuffer(commandBuffer, vertexStagingBuffer.getBuffer(), vertexBuffer->getBuffer(), 1, &copyRegion);

		rd->commandBufferSubmitIdle(&commandBuffer, VK_QUEUE_GRAPHICS_BIT);
	}

	// Index data
	{
		vkw::Buffer indexStagingBuffer(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, indices.data());

		indexBuffer = std::make_unique<vkw::Buffer>(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		VkCommandBuffer commandBuffer;
		CHECK_VKRESULT(rd->createCommandBuffer(&commandBuffer, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true));

		VkBufferCopy copyRegion{
			.size = indexBufferSize
		};
		vkCmdCopyBuffer(commandBuffer, indexStagingBuffer.getBuffer(), indexBuffer->getBuffer(), 1, &copyRegion);

		rd->commandBufferSubmitIdle(&commandBuffer, VK_QUEUE_GRAPHICS_BIT);
	}

	for (auto& material : materials) {
		if (material->albedoTexture) {
			material->createDescriptorSet(rd->getDescriptorPool(), rd->getDescriptorSetLayout(1), 0);
		}
	}
}

void Model::loadMaterials(const aiScene* scene) {
	for (size_t i = 0; i < scene->mNumMaterials; i++) {
		const aiMaterial* aimaterial = scene->mMaterials[i];

		std::shared_ptr<Material> newMaterial = std::make_shared<Material>();

		// Load material values
		aiColor3D color;
		if (aimaterial->Get(AI_MATKEY_BASE_COLOR, color) == AI_SUCCESS) {
			newMaterial->albedo = glm::vec3(color.r, color.g, color.b);
		} else if (aimaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
			newMaterial->albedo = glm::vec3(color.r, color.g, color.b);
		}

		if (aimaterial->Get(AI_MATKEY_METALLIC_FACTOR, newMaterial->metallic) != AI_SUCCESS) {
			aimaterial->Get(AI_MATKEY_SPECULAR_FACTOR, newMaterial->metallic);
		}
		if (aimaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, newMaterial->roughness) != AI_SUCCESS) {
			if (aimaterial->Get(AI_MATKEY_GLOSSINESS_FACTOR, newMaterial->roughness) == AI_SUCCESS) {
				newMaterial->aux.roughnessGlossyMode = 1;
			}
		}

		// Load material textures, nullptr if not
		newMaterial->albedoTexture = loadTexture(aimaterial, aiTextureType_DIFFUSE);
		newMaterial->metallicTexture = loadTexture(aimaterial, aiTextureType_METALNESS);
		if (newMaterial->metallicTexture == nullptr) {
			newMaterial->metallicTexture = loadTexture(aimaterial, aiTextureType_SPECULAR);
		}
		newMaterial->roughnessTexture = loadTexture(aimaterial, aiTextureType_DIFFUSE_ROUGHNESS);
		newMaterial->ambientTexture = loadTexture(aimaterial, aiTextureType_AMBIENT_OCCLUSION);

		newMaterial->normalMap = loadTexture(aimaterial, aiTextureType_NORMALS);

		if (newMaterial->normalMap) {
			newMaterial->aux.normalMapMode = 1;
		} else {
			// no normal map found, try loading the bump map instead
			newMaterial->normalMap = loadTexture(aimaterial, aiTextureType_HEIGHT);
			if (newMaterial->normalMap) {
				newMaterial->aux.normalMapMode = 2;
			}
		}

		newMaterial->apply();

		materials.push_back(newMaterial);
	}
}

std::shared_ptr<Texture> Model::loadTexture(const aiMaterial* mat, aiTextureType type) {
	std::shared_ptr<Texture> texture;
	uint32_t typeCount = mat->GetTextureCount(type);
	if (typeCount == 0) {
		return nullptr;
	} else if (typeCount > 1) {
		std::cout << "INFO::Model:loadTexture: found more than 1 texture for material " << mat->GetName().C_Str()
				  << "of type " << aiTextureTypeToString(type) << ", selecting only first one\n";
	}

	aiString filepath;
	mat->GetTexture(type, 0, &filepath);
	bool foundLoaded = false;
	for (unsigned int i = 0; i < textures.size(); i++) {
		if (std::strcmp(textures[i]->filepath.c_str(), filepath.C_Str()) == 0) {
			texture = textures[i];
			foundLoaded = true;
			break;
		}
	}

	if (!foundLoaded) {
		texture = std::make_shared<Texture>();
		texture->filepath = filepath.C_Str();
		vkw::Texture2D* textureObject = new vkw::Texture2D(path + '/' + filepath.C_Str());	///< assume they're all 2D textures, probably is
		texture->texture = std::unique_ptr<vkw::Texture>(textureObject);
	}

	return texture;
}

void Model::processNode(aiNode* node, const aiScene* scene, std::shared_ptr<Node> parent, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
	std::shared_ptr<Node> newNode = std::make_shared<Node>();
	newNode->parent = parent.get();
	newNode->name = node->mName.C_Str();

	aiVector3t<float> translate, rotate, scale;
	node->mTransformation.Decompose(scale, rotate, translate);
	newNode->translation = glm::vec3(translate.x, translate.y, translate.z);
	newNode->rotation = glm::vec3(glm::degrees(rotate.x), glm::degrees(rotate.y), glm::degrees(rotate.z));
	newNode->scale = glm::vec3(scale.x, scale.y, scale.z);
	newNode->transform = glm::mat4(node->mTransformation.a1, node->mTransformation.a2, node->mTransformation.a3, node->mTransformation.a4,
			node->mTransformation.b1, node->mTransformation.b2, node->mTransformation.b3, node->mTransformation.b4,
			node->mTransformation.c1, node->mTransformation.c2, node->mTransformation.c3, node->mTransformation.c4,
			node->mTransformation.d1, node->mTransformation.d2, node->mTransformation.d3, node->mTransformation.d4);

	for (unsigned int i = 0; i < node->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		std::shared_ptr<Node> childNode = std::make_shared<Node>();
		childNode->parent = newNode.get();
		childNode->name = mesh->mName.C_Str();
		childNode->transform = glm::mat4(1.f);

		childNode->mesh = processMesh(mesh, scene, newNode->transform, vertices, indices);	// TODO: check transform param
		newNode->children.push_back(childNode);
		linearNodes.push_back(childNode.get());
	}
	for (unsigned int i = 0; i < node->mNumChildren; i++) {
		processNode(node->mChildren[i], scene, newNode, vertices, indices);
	}

	if (parent) {
		parent->children.push_back(newNode);
	} else {
		nodes.push_back(newNode);
	}
	linearNodes.push_back(newNode.get());
}

std::unique_ptr<Mesh> Model::processMesh(aiMesh* mesh, const aiScene* scene, glm::mat4& transform, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
	std::unique_ptr<Mesh> newMesh = std::make_unique<Mesh>(transform);
	newMesh->name = mesh->mName.C_Str();

	for (uint32_t i = 0; i < mesh->mNumVertices; i++) {
		Vertex vertex{
			.position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z),
			.normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z),
			.uv = glm::vec2(0.f)
		};

		if (mesh->mTangents) {
			vertex.tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
		}

		if (mesh->mTextureCoords[0]) {
			vertex.uv = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
		}

		vertices.push_back(vertex);
	}

	for (uint32_t i = 0; i < mesh->mNumFaces; i++) {
		aiFace face = mesh->mFaces[i];
		uint32_t firstIndex = indices.size();
		uint32_t indexCount = face.mNumIndices;

		glm::vec3 pmin{ FLT_MAX }, pmax{ -FLT_MAX };
		for (uint32_t j = 0; j < face.mNumIndices; j++) {
			indices.push_back(face.mIndices[j]);

			glm::vec3 pos = vertices[face.mIndices[j]].position;
			pmin = glm::vec3{
				std::min(pos.x, pmin.x),
				std::min(pos.y, pmin.y),
				std::min(pos.z, pmin.z)
			};

			pmax = glm::vec3{
				std::max(pos.x, pmax.x),
				std::max(pos.y, pmax.y),
				std::max(pos.z, pmax.z)
			};
		}

		std::shared_ptr<Triangle> triangle = std::make_shared<Triangle>();
		triangle->firstIndex = firstIndex;
		triangle->indexCount = indexCount;
		triangle->material = mesh->mMaterialIndex > -1 ? materials[mesh->mMaterialIndex] : materials.back();
		triangle->updateBounds(pmin, pmax);
		newMesh->primitives.push_back(triangle);
	}

	return newMesh;
}

void Model::draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t bindImageset) {
	const VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer->getBuffer(), offsets);
	vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);

	for (auto& node : nodes) {
		drawNode(node, commandBuffer, pipelineLayout, bindImageset);
	}
}

void Model::drawNode(std::shared_ptr<Node> node, VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t bindImageset) {
	if (node->mesh) {
		vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &node->mesh->pushConstants);
		for (auto primitive : node->mesh->primitives) {
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, bindImageset, 1, &primitive->material->descriptorSet, 0, nullptr);

			vkCmdDrawIndexed(commandBuffer, primitive->indexCount, 1, primitive->firstIndex, 0, 0);
		}
	}

	for (auto& child : node->children) {
		drawNode(child, commandBuffer, pipelineLayout, bindImageset);
	}
}

Model::~Model() {
}

}  // namespace bennu