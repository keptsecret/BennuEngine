#ifndef BENNU_LIGHT_H
#define BENNU_LIGHT_H

#include <glm/glm.hpp>

namespace bennu {

class PointLight {
public:
	PointLight(glm::vec3 position = glm::vec3{0.f}, glm::vec3 color = glm::vec3{1.f}, float radius = 1.f, float intensity = 1.f) :
			posr(position.x, position.y, position.z, radius), colori(color.x, color.y, color.z, intensity) {}

	void setPosition(const glm::vec3& pos);
	glm::vec3 getPosition() const { return { posr.x, posr.y, posr.z }; }

	void setColor(const glm::vec3& col);
	glm::vec3 getColor() const { return { colori.x, colori.y, colori.z }; }

private:
	// packed for std430
	glm::vec4 posr{0.0f};	// position + radius
	glm::vec4 colori{1.f};		// color + intensity
};

}  // namespace bennu

#endif	// BENNU_LIGHT_H
