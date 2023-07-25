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

glm::vec3 AABB::center() {
	return (pMax + pMin) * 0.5f;
}

float AABB::radius() {
	return glm::distance(pMin, pMax) * 0.5f;
}

void AABB::transform(const glm::mat4& m) {
	glm::vec3 pmin = pMin, pmax = pMax;

	glm::vec3 translation{ m[3] };
	glm::mat4 basis{ m[0][0], m[0][1], m[0][2], m[0][3],
		m[1][0], m[1][1], m[1][2], m[1][3],
		m[2][0], m[2][1], m[2][2], m[2][3],
		0, 0, 0, 1 };

	glm::vec3 tmin = translation, tmax = translation;
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			float e = basis[i][j] * pmin[j];
			float f = basis[i][j] * pmax[j];
			if (e < f) {
				tmin[i] += e;
				tmax[i] += f;
			} else {
				tmin[i] += f;
				tmax[i] += e;
			}
		}
	}

	pMin = tmin;
	pMax = tmax;
}

void AABB::expand(const glm::vec3& p) {
	pMin = glm::min(pMin, p);
	pMax = glm::max(pMax, p);
}

}  // namespace bennu