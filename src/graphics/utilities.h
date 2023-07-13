#ifndef BENNU_UTILITIES_H
#define BENNU_UTILITIES_H

#include <vulkan/vulkan.h>

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <assert.h>

#define CHECK_VKRESULT(f) {																				\
	VkResult res = (f);																					\
	const char* func = #f;																				\
	if (res != VK_SUCCESS) {																			\
		std::cerr << "FATAL ERROR : " << func << " VkResult=\"" << bennu::vkw::utils::errorString(res) << "\" in " << __FILE__ << " at line " << __LINE__ << "\n"; \
		assert(res == VK_SUCCESS);																		\
	}																									\
}

namespace bennu {

namespace vkw {

namespace utils {

std::string errorString(VkResult errorCode);

VkShaderModule loadShader(const std::string& filename, VkDevice device);

}

}  // namespace vkw

}  // namespace bennu

#endif	// BENNU_UTILITIES_H
