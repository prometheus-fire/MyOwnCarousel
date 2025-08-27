#include "Light.h"

void Light::init(glm::mat4 view, glm::mat4 proj, GLuint width, GLuint height, glm::vec3 color)
{
	this->view = view;
	this->proj = proj;
	this->width = width;
	this->height = height;
	this->color = color;

	// initialize fbo and depth map
	glGenTextures(1, &this->depthMap);
	glBindTexture(GL_TEXTURE_2D, this->depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[] = { .0f, .0f, .0f, .0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	glGenFramebuffers(1, &this->FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, this->FBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, this->depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// initialize update shader if needed
	if (!update_shader_initialized) {
		update_shader_initialized = true;
		update_shader.create_program("./shaders/light.vert", "./shaders/light.frag");
		update_shader_models.create_program("./shaders/light_model.vert", "./shaders/light.frag");
	}
}



GLuint Light::getFBO()
{
	return this->FBO;
}

glm::vec3 Light::getColor()
{
	return this->color;
}

glm::vec3 Light::getLightDir()
{
	glm::mat4 inv_lspace = glm::inverse(this->lightSpaceTransform());
	glm::vec4 start, end;
	start = inv_lspace * glm::vec4(0, 0, -1, 1); start /= start.w;
	end = inv_lspace * glm::vec4(0, 0, 1, 1); end /= end.w;
	glm::vec3 result = glm::normalize(glm::vec3(end - start));
	return result;
}

glm::mat4 Light::lightSpaceTransform()
{
	return this->proj * this->view;
}


void Light::updateDepthMap(void(*render_scene)(glm::mat4 proj, glm::mat4 view, shader* replacement_shader, shader* replacemet_shader_models))
{
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	glViewport(0, 0, this->width, this->height);
	glBindFramebuffer(GL_FRAMEBUFFER, this->FBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	glUseProgram(update_shader.program);
	render_scene(this->proj, this->view, &update_shader, &update_shader_models);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
}

void Light::setView(glm::mat4 view)
{
	this->view = view;
}

void Light::setProj(glm::mat4 proj)
{
	this->proj = proj;
}

GLuint Light::getDepthMap()
{
	return this->depthMap;
}
