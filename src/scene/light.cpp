#include <scene/light.h>

namespace bennu {

void PointLight::setPosition(const glm::vec3& pos) {
	posr.x = pos.x;
	posr.y = pos.y;
	posr.z = pos.z;
}

void PointLight::setColor(const glm::vec3& col) {
	colori.x = col.x;
	colori.y = col.y;
	colori.z = col.z;
}

void DirectionalLight::setDirection(const glm::vec3& dir) {
	glm::vec3 tmp_dir = glm::normalize(dir);
	direction.x = tmp_dir.x;
	direction.y = tmp_dir.y;
	direction.z = tmp_dir.z;
}

void DirectionalLight::setColor(const glm::vec3& col) {
	colori.x = col.x;
	colori.y = col.y;
	colori.z = col.z;
}

}  // namespace bennu