#ifndef BENNU_LIGHT_H
#define BENNU_LIGHT_H

#include <glm/glm.hpp>

namespace bennu {

class PointLight {
public:
	PointLight(glm::vec3 position = glm::vec3{0.f}, glm::vec3 color = glm::vec3{1.f}, float radius = 1.f, float intensity = 1.f) :
			posr(position, radius), colori(color, intensity) {}

	void setPosition(const glm::vec3& pos);
	glm::vec3 getPosition() const { return { posr.x, posr.y, posr.z }; }

	void setColor(const glm::vec3& col);
	glm::vec3 getColor() const { return { colori.x, colori.y, colori.z }; }

private:
	// packed for std430
	glm::vec4 posr{0.0f};	// position + radius
	glm::vec4 colori{1.f};		// color + intensity
};

class DirectionalLight {
public:
	DirectionalLight(glm::vec3 direction = glm::vec3{0, -1, 0}, glm::vec3 color = glm::vec3{1.f}, float intensity = 1.f) :
			direction(glm::normalize(direction), 0), colori(color, intensity) {}

	void preprocess(glm::vec3 center, float radius) {
		worlddata.x = center.x;
		worlddata.y = center.y;
		worlddata.z = center.z;
		worlddata.w = radius;
	}

	void setDirection(const glm::vec3& dir);
	glm::vec3 getDirection() const { return { direction.x, direction.y, direction.z }; }

	void setColor(const glm::vec3& col);
	glm::vec3 getColor() const { return { colori.x, colori.y, colori.z }; }

private:
	// packed for std430
	glm::vec4 direction{0, -1, 0, 0};
	glm::vec4 colori{1.f};
	glm::vec4 worlddata;	// world center + radius
};

}  // namespace bennu

#endif	// BENNU_LIGHT_H
