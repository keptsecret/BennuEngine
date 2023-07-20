#ifndef BENNU_LIGHT_H
#define BENNU_LIGHT_H

#include <glm/glm.hpp>

namespace bennu {

// TODO: packed for std430, find more readable way
struct PointLight {
	glm::vec4 positionr{0.0f};	// position + radius
	glm::vec4 colori{1.f};		// color + intensity
};

}  // namespace bennu

#endif	// BENNU_LIGHT_H
