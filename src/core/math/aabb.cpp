#include <core/math/aabb.h>

namespace bennu {

AABB::AABB(const glm::vec3& p1, const glm::vec3& p2) {
	pMin = glm::vec3{ std::min(p1.x, p2.x),
		std::min(p1.y, p2.y),
		std::min(p1.z, p2.z) };
	pMax = glm::vec3{ std::max(p1.x, p2.x),
		std::max(p1.y, p2.y),
		std::max(p1.z, p2.z) };
}

void AABB::expand(const glm::vec3& p) {
	pMin = glm::min(pMin, p);
	pMax = glm::max(pMax, p);
}

glm::vec3 AABB::center() {
	return (pMax + pMin) * 0.5f;
}

float AABB::radius() {
	return glm::distance(pMin, pMax) * 0.5f;
}

}  // namespace bennu