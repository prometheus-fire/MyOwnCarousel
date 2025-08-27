#include "Camera.h"


Camera::Camera(glm::vec3 position, glm::vec3 center, glm::vec3 up)
{
	this->up = glm::normalize(up);
	this->lookAt(position, center);
}

glm::mat4 Camera::getViewMatrix()
{
	glm::vec3 direction = glm::vec3(
		glm::cos(this->pitch) * glm::sin(this->yaw),
		glm::sin(this->pitch),
		glm::cos(this->pitch) * glm::cos(this->yaw)
	);
	return glm::lookAt(this->position, this->position + direction, this->up);
}


void Camera::lookAt(glm::vec3 position, glm::vec3 center)
{
	this->position = position;
	glm::vec3 direction = glm::normalize(center - position);
	// calculate pitch and yaw
	this->pitch = glm::asin(direction.y);
	this->yaw = glm::atan(direction.x, direction.z);
}

void Camera::keyboard_move(Movement m, float deltaTime)
{
	glm::vec3 direction = glm::vec3(
		glm::cos(this->pitch) * glm::sin(this->yaw),
		glm::sin(this->pitch),
		glm::cos(this->pitch) * glm::cos(this->yaw)
	);
	glm::vec3 right = glm::normalize(glm::cross(direction, this->up));

	float vel = this->movement_speed * deltaTime;

	switch (m) {
	case UP:
		this->position += this->up * vel;
		break;
	case DOWN: 
		this->position -= this->up * vel;
		break;
	case RIGHT: 
		this->position += right * vel;
		break;
	case LEFT: 
		this->position -= right * vel;
		break;
	case FORWARD: 
		this->position += direction * vel;
		break;
	case BACKWARDS: 
		this->position -= direction * vel;
		break;
	}
}


void Camera::mouse_move(GLFWwindow* window, double xpos, double ypos)
{
	if (this->dragging_mouse) {
		GLint viewport_info[4];
		glGetIntegerv(GL_VIEWPORT, viewport_info);

		double width = (double)viewport_info[2];
		double height = (double)viewport_info[3];

		double deltax = (xpos - this->first_captured_mouse_coordinates.x) / width;
		double deltay = (ypos - this->first_captured_mouse_coordinates.y) / height;
		
		this->yaw = this->first_captured_yaw + deltax * this->x_sensitivity;
		this->pitch = this->first_captured_pitch + deltay * this->y_sensitivity;
	}
}


void Camera::set_relative_mouse_coordinates(glm::vec2 coords)
{
	this->first_captured_mouse_coordinates = coords;
	this->first_captured_pitch = this->pitch;
	this->first_captured_yaw = this->yaw;
}



