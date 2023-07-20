#ifndef BENNU_BUFFER_H
#define BENNU_BUFFER_H

#include <vulkan/vulkan.h>

namespace bennu {

namespace vkw {

class Buffer {
public:
	Buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, const void* data = nullptr);
	virtual ~Buffer();

	const VkBuffer& getBuffer() const { return buffer; }
	const VkDeviceMemory& getMemory() const { return deviceMemory; }

protected:
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory deviceMemory = VK_NULL_HANDLE;
	VkDeviceSize size;
};

class UniformBuffer : public Buffer {
public:
	UniformBuffer(VkDeviceSize size, const void* data = nullptr);

	void update(const void* data);

private:
	void* mapped = nullptr;
};

class StorageBuffer : public Buffer {
public:
	StorageBuffer(VkDeviceSize size, const void* data = nullptr);

	void update(const void* data);

private:
	void* mapped = nullptr;
};

}  // namespace vkw

}  // namespace bennu

#endif	// BENNU_BUFFER_H
