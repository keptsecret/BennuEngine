#ifndef BENNU_AABB_H
#define BENNU_AABB_H

#include <glm/glm.hpp>

namespace bennu {

class GPUBB;

class AABB {
public:
	AABB() {}
	AABB(const glm::vec3& p1, const glm::vec3& p2);

	void transform(const glm::mat4& m);
	void expand(const glm::vec3& p);

	const glm::vec3& min() const { return pMin; }
	const glm::vec3& max() const { return pMax; }

	glm::vec3 center();
	float radius();

	GPUBB toGPUBB();

private:
	glm::vec3 pMin{ FLT_MAX };
	glm::vec3 pMax{ -FLT_MAX };
};

class GPUBB {
public:
	GPUBB() {}
	GPUBB(const glm::vec3& p1, const glm::vec3& p2);

	AABB toAABB();

private:
	glm::vec4 pMin{ FLT_MAX };
	glm::vec4 pMax{ -FLT_MAX };
};

}  // namespace bennu

#endif	// BENNU_AABB_H
