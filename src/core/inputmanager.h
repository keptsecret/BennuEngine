#ifndef BENNU_INPUTMANAGER_H
#define BENNU_INPUTMANAGER_H

#include <glfw/glfw3.h>
#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>

namespace bennu {

class InputManager {
public:
	InputManager(bool inverted_y = false) :
			prev_mouse_coords(0, 0), invert_y(inverted_y) {
		InputManager::instances.push_back(this);
	}
	~InputManager() {
		instances.erase(std::remove(instances.begin(), instances.end(), this), instances.end());
	}

	static void init(GLFWwindow* window);
	static void update();

	bool isKeyDown(unsigned int key_id);
	bool isKeyPressed(unsigned int key_id);
	bool isButtonDown(unsigned int butt_id);

	glm::vec2 getMouseOffset() { return mouse_offset; }
	glm::vec2 getMousePosition() { return mouse_coords; }

private:
	void setKeyDown(unsigned int key_id, bool is_down);
	void setKeyPressed(unsigned int key_id);

	void setButtonDown(unsigned int butt_id, bool is_down);

	void setMouseCoords(glm::vec2 coords);
	void setMouseOffset();

	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
	static void mouseCallback(GLFWwindow* window, double x_pos_in, double y_pos_in);

	std::unordered_map<unsigned int, bool> key_down_map;
	std::unordered_map<unsigned int, bool> key_pressed_map;

	std::unordered_map<unsigned int, bool> mb_map;

	glm::vec2 mouse_coords;
	glm::vec2 prev_mouse_coords;
	glm::vec2 mouse_offset;
	bool invert_y;
	bool first_mouse_input = true;

	static std::vector<InputManager*> instances;
};

}  // namespace bennu

#endif	// BENNU_INPUTMANAGER_H
