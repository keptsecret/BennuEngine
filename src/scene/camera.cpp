#include <scene/camera.h>

#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace bennu {

Camera::Camera(const glm::vec3& pos, const glm::vec3& up, float yaw, float pitch) :
		position(pos), front(0, 0, -1), world_up(up), yaw(yaw), pitch(pitch),
		speed(1.f), sensitivity(0.05f), fov(50.f), near_plane(0.1f), far_plane(100.f) {
	updateCameraVectors();
}

glm::mat4 Camera::getProjectionTransform() const {
	glm::mat4 proj = glm::perspective(glm::radians(45.f), (float)width / (float)height, near_plane, far_plane);
	return proj;
}

glm::mat4 Camera::getViewTransform() const {
	return glm::lookAt(position, position + front, up);
}

void Camera::translate(CameraMovement direction, float delta_time) {
	float velocity = speed * delta_time;
	switch (direction) {
		case CameraMovement::Forward:
			position += front * velocity;
			break;
		case CameraMovement::Backward:
			position -= front * velocity;
			break;
		case CameraMovement::Left:
			position -= right * velocity;
			break;
		case CameraMovement::Right:
			position += right * velocity;
			break;
	}
}

void Camera::pan(float xoffset, float yoffset, bool constrain_pitch) {
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	if (constrain_pitch) {
		pitch = std::clamp(pitch, -89.0f, 89.0f);
	}

	updateCameraVectors();
}

void Camera::updateCameraVectors() {
	glm::vec3 direction{
		std::cos(glm::radians(yaw)) * std::cos(glm::radians(pitch)),
		std::sin(glm::radians(pitch)),
		std::sin(glm::radians(yaw)) * std::cos(glm::radians(pitch))
	};

	front = glm::normalize(direction);
	right = glm::normalize(glm::cross(front, world_up));
	up = glm::normalize(glm::cross(right, front));
}

}  // namespace bennu