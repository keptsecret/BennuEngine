#include <graphics/buffer.h>

#include <graphics/renderingdevice.h>
#include <graphics/utilities.h>

namespace bennu {

namespace vkw {

Buffer::Buffer(VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags properties, const void* data) :
		size(size) {
	CHECK_VKRESULT(RenderingDevice::getSingleton()->createBuffer(&buffer, usageFlags, &deviceMemory, properties, size, data));
}

Buffer::~Buffer() {
	VkDevice device = RenderingDevice::getSingleton()->getDevice();
	vkDestroyBuffer(device, buffer, nullptr);
	vkFreeMemory(device, deviceMemory, nullptr);
}

UniformBuffer::UniformBuffer(VkDeviceSize size, const void* data) :
		Buffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, nullptr) {
	VkDevice device = RenderingDevice::getSingleton()->getDevice();
	vkMapMemory(device, deviceMemory, 0, size, 0, &mapped);
	if (data != nullptr) {
		update(data);
	}
}

void UniformBuffer::update(const void* data) {
	memcpy(mapped, data, size);
}

StorageBuffer::StorageBuffer(VkDeviceSize size, const void* data) :
		Buffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, nullptr) {
	VkDevice device = RenderingDevice::getSingleton()->getDevice();
	vkMapMemory(device, deviceMemory, 0, size, 0, &mapped);
	if (data != nullptr) {
		update(data);
	}
}
void StorageBuffer::update(const void* data) {
	memcpy(mapped, data, size);
}

}  // namespace vkw

}  // namespace bennu