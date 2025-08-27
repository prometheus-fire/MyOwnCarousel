#pragma once

#include <GL/glew.h>

#include <glm/common.hpp>
#include <glm/ext.hpp>

#include "./common/shaders.h"


/*
	A Light is modeled simply as a framebuffer object
	with only a depth attachment.
*/

/*
	This is the id of the shader used to
	render the scene from the light's point of view.
*/
static bool update_shader_initialized = false;
static shader update_shader;
static shader update_shader_models;


class Light
{
public:
	/*
		Initialize a light by providing a view frame,
		a projection matrix and an image resoltion.
	*/
	void init(glm::mat4 view, glm::mat4 proj, GLuint width, GLuint height, glm::vec3 color=glm::vec3(1));

	/*
		This function updates the depth map buffer by rendering the scene
		with update_shader.
	*/
	// (glm::mat4 proj, glm::mat4 view, shader* replacement_shader = NULL, shader* replacemet_shader_models = NULL) 
	void updateDepthMap(void (*render_scene)(glm::mat4 proj, glm::mat4 view, shader* replacement_shader, shader* replacemet_shader_models));

	void setView(glm::mat4 view);
	void setProj(glm::mat4 proj);
	void setColor(glm::vec3 color) { this->color = color; };

	GLuint getDepthMap();
	GLuint getFBO();

	glm::vec3 getColor();
	glm::vec3 getLightDir();

	/*
		Returns a matrix which transforms points from world space
		to the light's view space.
	*/
	glm::mat4 lightSpaceTransform();


private:
	glm::mat4 view, proj;
	GLuint FBO, depthMap;
	GLuint width, height;
	glm::vec3 color;
};

