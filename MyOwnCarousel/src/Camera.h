#pragma once

#include <GLFW/glfw3.h>
#include <glm/common.hpp>
#include <glm/ext.hpp>

#include <iostream>


enum Movement {
	UP, DOWN, LEFT, RIGHT, FORWARD, BACKWARDS
};

/*
	A simple fps camera class made using a gymbal.
	At the current state it only uses pitch and yaw.
*/

class Camera
{
public:

	/*
		Initialize the camera by calculating pitch and yaw
		and setting position and up vectors.
	*/
	Camera(glm::vec3 position, glm::vec3 center, glm::vec3 up);
	glm::mat4 getViewMatrix();
	void lookAt(glm::vec3 position, glm::vec3 center);



	/*
		This variable controls whether or not mouse and
		keyboard movements are being captured
	*/
	bool capturingMovement = true;

	/*
		Moves camera according to a particular movement.
	*/
	void keyboard_move(Movement m, float deltaTime);

	/*
		Set this to true to enable mouse movement.
		This is used inside of the mouse_move function.
		Before setting this to true, remember to call
		set_relative_mouse_coordinates with the current mouse coordinates.
	*/
	bool dragging_mouse = false;


	/*
		Sets first_captured_mouse_coordinates.
		See mouse_move for its use.
	*/
	void set_relative_mouse_coordinates(glm::vec2 coords);

	/*
		If dragging_mouse is set to true, it rotates the camera according
		to the difference between the current mouse coordinates
		relative to the window's content area coordinates
		and first_captured_mouse_coordinates.
	*/
	void mouse_move(GLFWwindow* window, double xpos, double ypos);

	

private:
	glm::vec3 position, up;
	double pitch, yaw;

	glm::vec2 first_captured_mouse_coordinates;
	double first_captured_pitch, first_captured_yaw;
	double x_sensitivity = 1., y_sensitivity = 1.;

	float movement_speed = .3;
};

