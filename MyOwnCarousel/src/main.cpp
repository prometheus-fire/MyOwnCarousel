#define NANOSVG_IMPLEMENTATION	// Expands implementation
#include "../3rdparty/nanosvg/src/nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "../3rdparty/nanosvg/src/nanosvgrast.h"

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>
#include <iostream>

#define GLM_ENABLE_EXPERIMENTAL

#include ".\common\debugging.h"
#include ".\common\renderable.h"
#include ".\common\shaders.h"
#include ".\common\simple_shapes.h"
#include ".\common\carousel\carousel.h"
#include ".\common\carousel\carousel_to_renderable.h"
#include ".\common\carousel\carousel_loader.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#include ".\common\gltf_loader.h"
#include ".\common\texture.h"

#include <iostream>
#include <algorithm>
#include <conio.h>
#include <direct.h>
#include ".\common\matrix_stack.h"
#include ".\common\intersection.h"
#include ".\common\trackball.h"


#include "Light.h"



/*
	This file contains a list of useful global
	variables. It is mainly used to mediate between
	the main file and the ImGui interface.
*/
#include "globals.h"

/*
	Time variables
*/
double oldTime;
double deltaTime;


/*
	User interface stuff with ImGui.
*/
#include "user_interface.h"


trackball tb[2];
int curr_tb;

/* projection matrix*/
glm::mat4 proj;

/* view matrix */
glm::mat4 view;

matrix_stack stack;
float scaling_factor = 1.0;


/*
	This shader will be used to render lamps, trees, cameras, ...
*/
shader model_shader;

/*
	this shader can be used with an fsq to visualize a light's depth map.
*/
shader see_lighted;

/*
	this shader is used to visualize a view frustum (proj * view).
*/
shader show_frame;
renderable cube;

/*
	this struct is used for structuring the data for the model shader
	Model ubo.
*/
struct Modelmatrices {
	glm::mat4 Model[25];
};


/*
	tree renderable
*/
std::vector<renderable> tree_renderable;
box3 tree_box;
GLuint tree_modelmatrices_UBO;

/*
	car renderable
*/
std::vector<renderable> car_renderable;
box3 car_box;
GLuint car_modelmatrices_UBO;


/*
	camera renderable
*/
std::vector<renderable> camera_renderable;
box3 camera_box;
GLuint camera_modelmatrices_UBO;

/*
	lamp renderable
*/
std::vector<renderable> lamp_renderable;
box3 lamp_box;
std::vector<glm::vec3> closest_points; // a vector containing for each lamp its closest point to the track
GLuint lamp_modelmatrices_UBO;

/*
	This is used for rendering the skybox.
*/
renderable quad;
shader skybox_shader;
texture skybox_cubemap;


renderable fram;
renderable r_cube;
renderable r_trees;
renderable r_lamps;

/*
	See the ...::to_track(..) function to know
	how the track is set up.
*/
renderable r_track;
shader track_shader;
texture track_texture;

/*
	See the ...::to_heightfield(..) function to know
	how the terrain is set up.
*/
renderable r_terrain;
shader terrain_shader;
texture terrain_diffuse;

shader basic_shader;


/*
	Lights
*/

Light sun;
std::vector<Light> lampLights;
std::vector<Light> carLights;

unsigned int lightsUBO;

// lamps + sun
struct light_struct {
	glm::mat4 lightSpaceMatrix[19 + 1 + 10];
	glm::vec4 lightDir[19 + 1 + 10];
	glm::vec4 lightColor[19 + 1 + 10]; // vec4 is used to account for padding
};


bool lights_initialized = false;

void loadLights(shader& s) {

	// link lights uniform block

	GLuint lblockIndex = glGetUniformBlockIndex(s.program, "Lights");
	GLuint bindingPoint = 0;
	glUniformBlockBinding(s.program, lblockIndex, bindingPoint);
	glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, lightsUBO);

	GLuint ids[30];
	for (GLuint i = 1; i <= 30; i++) ids[i-1] = i;
	glUniform1iv(s["depthmap"], 30, (const GLint *)ids);

	if (!lights_initialized) {
		lights_initialized = true;

		// load sun depthmap
		GLuint sun_depthmap = sun.getDepthMap();
		glActiveTexture(GL_TEXTURE0 + 1);
		glBindTexture(GL_TEXTURE_2D, sun_depthmap);
		//glUniform1i(s["depthmap[0]"], 1);

		// load lamp depthmaps
		for (int i = 0; i < 19; i++) {
			Light& l = lampLights[i];
			GLuint l_depthmap = l.getDepthMap();
			//std::string ldepth = "depthmap[" + std::to_string(i + 1) + "]";
			glActiveTexture(GL_TEXTURE0 + 2 + i);
			glBindTexture(GL_TEXTURE_2D, l_depthmap);
			//glUniform1i(s[ldepth], 2 + i);
		}

		// load car lights
		for (int i = 0; i < 10; i++) {
			Light& l = carLights[i];
			GLuint l_depthmap = l.getDepthMap();
			//std::string ldepth = "depthmap[" + std::to_string(i + 20) + "]";
			glActiveTexture(GL_TEXTURE0 + 21 + i);
			glBindTexture(GL_TEXTURE_2D, l_depthmap);
			//glUniform1i(s[ldepth], 21 + i);
		}
	}
}

/*
	Given a day time in range [0.,1.] (where 0. and 1. are midnight and 0.5 is midday)
	this function returns the sky's color.
*/
glm::vec3 skyColor(float daytime) {
	glm::vec3 dayColor(1.0, 1.0, 1.0);
	glm::vec3 nightColor(0.16, 0.176, 0.24);
	glm::vec3 sunriseColor(1.0, 0.55, 0.2);
	glm::vec3 sunsetColor(1.0, 0.40, 0.25);

	float sunrisePeak = 0.25;
	float sunriseEnd = 0.30;
	float sunsetStart = 0.70;
	float sunsetPeak = 0.75;

	glm::vec3 finalColor = mix(nightColor, sunriseColor, glm::smoothstep(sunrisePeak - 0.05f, sunrisePeak, daytime));
	finalColor = mix(finalColor, dayColor, glm::smoothstep(sunrisePeak, sunriseEnd, daytime));
	finalColor = mix(finalColor, sunsetColor, glm::smoothstep(sunsetStart, sunsetPeak, daytime));
	finalColor = mix(finalColor, nightColor, glm::smoothstep(sunsetPeak, sunsetPeak + 0.05f, daytime));

	return finalColor;
}



/* callback function called when the mouse is moving */
static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	//tb[curr_tb].mouse_move(proj, view, xpos, ypos);
	global_camera.mouse_move(window, xpos, ypos);
}


/* callback function called when a mouse button is pressed */
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		tb[curr_tb].mouse_press(proj, view, xpos, ypos);

		global_camera.set_relative_mouse_coordinates(glm::vec2(xpos, ypos));
		global_camera.dragging_mouse = true;
	}
	else
		if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
			tb[curr_tb].mouse_release();

			global_camera.dragging_mouse = false;
		}
}

/* callback function called when a mouse wheel is rotated */
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	/*
	if (curr_tb == 0)
		tb[0].mouse_scroll(xoffset, yoffset);
	*/
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	/* every time any key is presse it switch from controlling trackball tb[0] to tb[1] and viceversa */
	/*
	if (action == GLFW_PRESS)
		curr_tb = 1 - curr_tb;
		*/

}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
	proj = glm::perspective(glm::radians(45.f), float(width) / float(height), .001f, 2.f);
}

void processInput(GLFWwindow* window) {
	float newTime = glfwGetTime();
	deltaTime = newTime - oldTime;
	oldTime = newTime;

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		global_camera.keyboard_move(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		global_camera.keyboard_move(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		global_camera.keyboard_move(BACKWARDS, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		global_camera.keyboard_move(RIGHT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		global_camera.keyboard_move(UP, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
		global_camera.keyboard_move(DOWN, deltaTime);
}


/*
	This function is used to render a loaded model consisting of 
	an array of renderables and a bounding box according to the transformations
	that are currently stored on a matrix stack with shader s.

	When with_textures is set to false the uniform calls for uploading the texture
	ids are bypassed.
*/

void drawLoadedModelInstanced(std::vector<renderable> obj, box3 bbox, shader s, GLuint instances, bool with_textures = true) {
	matrix_stack stack;
	stack.load_identity();
	stack.push();
	float scale = 1.f / bbox.diagonal();
	stack.mult(glm::scale(glm::mat4(1.f), glm::vec3(scale)));
	stack.mult(glm::translate(glm::mat4(1.f), glm::vec3(-bbox.center())));

	// render each renderable
	for (unsigned int i = 0; i < obj.size(); ++i) {
		obj[i].bind();
		stack.push();
		// each object had its own transformation that was read in the gltf file
		stack.mult(obj[i].transform);

		if (with_textures) {
			if (obj[i].mater.base_color_texture != -1) {
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, obj[i].mater.base_color_texture);
			}
			glUniform1i(s["texture_diffuse"], 0);
		}
		glUniformMatrix4fv(s["uModel"], 1, GL_FALSE, &stack.m()[0][0]);
		glDrawElementsInstanced(obj[i]().mode, obj[i]().count, obj[i]().itype, 0, instances);
		stack.pop();
	}
}




/*
	This function is used to render the scene according to
	a projection and view matrix. 
	
	When replacement shader is
	not null, all uniform calls except for uProj, uView and uModel
	are bypassed and the skybox is not rendered.
	This is used when trying to render the scene from a light's point
	of view.
*/
void render_scene(glm::mat4 proj, glm::mat4 view, shader* replacement_shader = NULL, shader* replacemet_shader_models = NULL) {

	if (replacement_shader == NULL) {
	}
	else {
		glUniformMatrix4fv( (*replacement_shader)["uProj"], 1, GL_FALSE, &proj[0][0]);
		glUniformMatrix4fv( (*replacement_shader)["uView"], 1, GL_FALSE, &view[0][0]);
	}


	matrix_stack stack;
	stack.load_identity();
	stack.push();

	/*glUseProgram(basic_shader.program);
		glUniformMatrix4fv(basic_shader["uProj"], 1, GL_FALSE, &proj[0][0]);
		glUniformMatrix4fv(basic_shader["uView"], 1, GL_FALSE, &view[0][0]);*/

	//stack.mult(tb[0].matrix());

	/*glUniformMatrix4fv(basic_shader["uModel"], 1, GL_FALSE, &stack.m()[0][0]);
	glUniform3f(basic_shader["uColor"], -1.f, 0.6f, 0.f);
	fram.bind();
	glDrawArrays(GL_LINES, 0, 6);*/

	/*glColor3f(0, 0, 1);
	glBegin(GL_LINES);
	glVertex3f(0, 0, 0);
	glVertex3f(r.sunlight_direction().x, r.sunlight_direction().y, r.sunlight_direction().z);
	glEnd();*/


	float s = 1.f / r.bbox().diagonal();
	glm::vec3 c = r.bbox().center();

	glm::vec3 ldir = r.sunlight_direction();

	stack.mult(glm::scale(glm::mat4(1.f), glm::vec3(s)));
	stack.mult(glm::translate(glm::mat4(1.f), -c));


	/*
		This moves the terrain slightly downwards to prevent the track
		from clipping with the ground beneath.
	*/
	stack.push();
	stack.mult(glm::translate(glm::mat4(1.), glm::vec3(0, 1, 0) * -0.07));

	glm::vec3 skycolor;

	if (replacement_shader == NULL) {
		
		glUseProgram(terrain_shader.program);
		glUniformMatrix4fv(terrain_shader["uProj"], 1, GL_FALSE, &proj[0][0]);
		glUniformMatrix4fv(terrain_shader["uView"], 1, GL_FALSE, &view[0][0]);
		glUniformMatrix4fv(terrain_shader["uModel"], 1, GL_FALSE, &stack.m()[0][0]);
		glUniform3f(terrain_shader["uColor"], -1, 0, 0);
		skycolor = skyColor(r.getDayTime());
		glUniform3f(terrain_shader["uSkyColor"], skycolor.x, skycolor.y, skycolor.z);

		loadLights(terrain_shader);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, terrain_diffuse.id);
		glUniform1i(terrain_shader["texture_diffuse"], 0);
	}
	else {
		glUniformMatrix4fv((*replacement_shader)["uModel"], 1, GL_FALSE, &stack.m()[0][0]);
	}

	glDisable(GL_CULL_FACE);

	r_terrain.bind();
	glDrawElements(GL_TRIANGLES, r_terrain().count * 2, GL_UNSIGNED_INT, 0);

	glEnable(GL_CULL_FACE);

	stack.pop();

	/*glUseProgram(basic_shader.program);
		glUniformMatrix4fv(basic_shader["uProj"], 1, GL_FALSE, &proj[0][0]);
		glUniformMatrix4fv(basic_shader["uView"], 1, GL_FALSE, &view[0][0]);*/

	/*for (unsigned int ic = 0; ic < r.cars().size(); ++ic) {
		stack.push();
		stack.mult(r.cars()[ic].frame);
		stack.mult(glm::translate(glm::mat4(1.f), glm::vec3(0, 0.1, 0.0)));
		glUniformMatrix4fv(basic_shader["uModel"], 1, GL_FALSE, &stack.m()[0][0]);
		glUniform3f(basic_shader["uColor"], -1.f, 0.6f, 0.f);
		fram.bind();
		glDrawArrays(GL_LINES, 0, 6);
		stack.pop();
	}*/

	/*fram.bind();
	for (unsigned int ic = 0; ic < r.cameramen().size(); ++ic) {
		stack.push();
		stack.mult(r.cameramen()[ic].frame);
		stack.mult(glm::scale(glm::mat4(1.f), glm::vec3(4, 4, 4)));
		glUniformMatrix4fv(basic_shader["uModel"], 1, GL_FALSE, &stack.m()[0][0]);
		glUniform3f(basic_shader["uColor"], -1.f, 0.6f, 0.f);
		glDrawArrays(GL_LINES, 0, 6);
		stack.pop();
	}
	glUniformMatrix4fv(basic_shader["uModel"], 1, GL_FALSE, &stack.m()[0][0]);*/

	/*r_track.bind();
	glPointSize(3.0);
	glUniform3f(basic_shader["uColor"], 0.2f, 0.3f, 0.2f);
	glDrawArrays(GL_LINE_STRIP, 0, r_track.vn);
	glPointSize(1.0);*/


	/*
		Render the track.
	*/
	if (replacement_shader == NULL) {
		glUseProgram(track_shader.program);
		glUniformMatrix4fv(track_shader["uProj"], 1, GL_FALSE, &proj[0][0]);
		glUniformMatrix4fv(track_shader["uView"], 1, GL_FALSE, &view[0][0]);
		glUniformMatrix4fv(track_shader["uModel"], 1, GL_FALSE, &stack.m()[0][0]);
		glUniform3f(track_shader["uColor"], -1, 0, 0);
		glUniform3f(track_shader["uSkyColor"], skycolor.x, skycolor.y, skycolor.z);

		loadLights(track_shader);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, track_texture.id);
		glUniform1i(track_shader["texture_diffuse"], 0);

	}
	else {
		glUniformMatrix4fv((*replacement_shader)["uModel"], 1, GL_FALSE, &stack.m()[0][0]);
	}

	r_track.bind();
	glCullFace(GL_FRONT);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, r_track.vn);
	glCullFace(GL_BACK);

	if (replacement_shader == NULL) {
		
	}
	else {
		glUniformMatrix4fv((*replacement_shader)["uModel"], 1, GL_FALSE, &stack.m()[0][0]);
	}

	/*glUseProgram(basic_shader.program);
		glUniformMatrix4fv(basic_shader["uProj"], 1, GL_FALSE, &proj[0][0]);
		glUniformMatrix4fv(basic_shader["uView"], 1, GL_FALSE, &view[0][0]);
		glUniformMatrix4fv(basic_shader["uModel"], 1, GL_FALSE, &stack.m()[0][0]);*/

	/*r_lamps.bind();
	glUniform3f(basic_shader["uColor"], 1.f, 1.0f, 0.f);
	glDrawArrays(GL_LINES, 0, r_lamps.vn);*/

	/*r_trees.bind();
	glUniform3f(basic_shader["uColor"], 0.f, 1.0f, 0.f);
	glDrawArrays(GL_LINES, 0, r_trees.vn);*/



	/*
		Draw trees.
	*/

	if (replacement_shader == NULL) {
		glUseProgram(model_shader.program);
		glUniformMatrix4fv(model_shader["uProj"], 1, GL_FALSE, &proj[0][0]);
		glUniformMatrix4fv(model_shader["uView"], 1, GL_FALSE, &view[0][0]);
		glUniformMatrix4fv(model_shader["uModel"], 1, GL_FALSE, &stack.m()[0][0]);
		glUniform3f(model_shader["uColor"], -0.5f, 0.5f, 0.5f);
		glUniform3f(model_shader["uSkyColor"], skycolor.x, skycolor.y, skycolor.z);
		loadLights(model_shader);
	}
	else {
		glUseProgram((*replacemet_shader_models).program);
		glUniformMatrix4fv((*replacemet_shader_models)["uProj"], 1, GL_FALSE, &proj[0][0]);
		glUniformMatrix4fv((*replacemet_shader_models)["uView"], 1, GL_FALSE, &view[0][0]);
		glUniformMatrix4fv((*replacemet_shader_models)["uModel"], 1, GL_FALSE, &stack.m()[0][0]);
	}

	GLuint mblockIndex;
	GLuint mbindingPoint = 1;
	if (replacement_shader == NULL) {
		mblockIndex = glGetUniformBlockIndex(model_shader.program, "Models");
		glUniformBlockBinding(model_shader.program, mblockIndex, mbindingPoint);
	}
	else {
		mblockIndex = glGetUniformBlockIndex((*replacemet_shader_models).program, "Models");
		glUniformBlockBinding((*replacemet_shader_models).program, mblockIndex, mbindingPoint);
	}
	
	glBindBufferBase(GL_UNIFORM_BUFFER, mbindingPoint, tree_modelmatrices_UBO);

	if (replacement_shader == NULL){
		drawLoadedModelInstanced(tree_renderable, tree_box, model_shader, r.trees().size());
	}
	else {
		drawLoadedModelInstanced(tree_renderable, tree_box, (*replacemet_shader_models), r.trees().size(), false);
	}


	/*
		Draw cars
	*/

	glBindBufferBase(GL_UNIFORM_BUFFER, mbindingPoint, car_modelmatrices_UBO);

	if (replacement_shader == NULL) {
		drawLoadedModelInstanced(car_renderable, car_box, model_shader, r.cars().size());
	}
	else {
		drawLoadedModelInstanced(car_renderable, car_box, (*replacemet_shader_models), r.cars().size(), false);
	}


	/*
		Draw cameramen.
	*/
	glBindBufferBase(GL_UNIFORM_BUFFER, mbindingPoint, camera_modelmatrices_UBO);

	if (replacement_shader == NULL) {
		drawLoadedModelInstanced(camera_renderable, camera_box, model_shader, r.cameramen().size());
	}
	else {
		drawLoadedModelInstanced(camera_renderable, camera_box, (*replacemet_shader_models), r.cameramen().size(), false);
	}


	/*
		Draw lamps.
	*/
	if (replacement_shader == NULL) {
		glUniform3f(model_shader["uColor"], 0.5f, 0.5f, 0.5f);
	}
	glBindBufferBase(GL_UNIFORM_BUFFER, mbindingPoint, lamp_modelmatrices_UBO);

	if (replacement_shader == NULL) {
		drawLoadedModelInstanced(lamp_renderable, lamp_box, model_shader, r.lamps().size());
	}
	else {
		drawLoadedModelInstanced(lamp_renderable, lamp_box, (*replacemet_shader_models), r.lamps().size(), false);
	}

	stack.pop();

	/*
		Render the skybox.
	*/
	if (replacement_shader == NULL) {
		glUseProgram(skybox_shader.program);
		glm::mat4 inv_proj = glm::inverse(proj);
		glm::mat4 inv_view = glm::inverse(view);
		glUniformMatrix4fv(skybox_shader["uInvProj"], 1, GL_FALSE, &inv_proj[0][0]);
		glUniformMatrix4fv(skybox_shader["uInvView"], 1, GL_FALSE, &inv_view[0][0]);
		glUniform3f(skybox_shader["uLightDir"], ldir.x, ldir.y, ldir.z);
		glm::vec3 skycolor = skyColor(r.getDayTime());
		glUniform3f(skybox_shader["uSkyColor"], skycolor.x, skycolor.y, skycolor.z);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_cubemap.id);
		glUniform1i(skybox_shader["cubemap"], 0);
		quad.bind();
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	}
}


int main(int argc, char** argv)
{

	carousel_loader::load("../assets/small_test.svg", "../assets/terrain_256.png", r);

	//add 10 cars
	for (int i = 0; i < 10; ++i)
		r.add_car();

	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
		return -1;

	glfwWindowHint(GLFW_SAMPLES, 4);

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(800, 800, "CarOusel", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}
	/* declare the callback functions on mouse events */
	if (glfwRawMouseMotionSupported())
		glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);


	oldTime = deltaTime = glfwGetTime();

	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetKeyCallback(window, key_callback);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	glewInit();
	glEnable(GL_MULTISAMPLE);
	printout_opengl_glsl_info();


	/*
		Init user interface
	*/
	userInterface_init(window);


	/*
		########################################
		Initialize shaders, textures and models.
		########################################
	*/
	{
		model_shader.create_program("./shaders/model.vert", "./shaders/terrain-track-model.frag");
		skybox_shader.create_program("./shaders/skybox.vert", "./shaders/skybox.frag");
		track_shader.create_program("./shaders/track.vert", "./shaders/terrain-track-model.frag");
		basic_shader.create_program("./shaders/basic.vert", "./shaders/basic.frag");
		terrain_shader.create_program("./shaders/terrain.vert", "./shaders/terrain-track-model.frag");

		terrain_diffuse.load("../assets/grass.png", 0);
		track_texture.load("../assets/road.jpg", 0);
		skybox_cubemap.load_cubemap(
			"../assets/skybox/pos_x.png",
			"../assets/skybox/neg_x.png",
			"../assets/skybox/pos_y.png",
			"../assets/skybox/neg_y.png",
			"../assets/skybox/pos_z.png",
			"../assets/skybox/neg_z.png",
			0
		);

		r_track.create();
		game_to_renderable::to_track(r, r_track);
		r_terrain.create();
		game_to_renderable::to_heightfield(r, r_terrain);
		r_trees.create();
		game_to_renderable::to_tree(r, r_trees);
		r_lamps.create();
		game_to_renderable::to_lamps(r, r_lamps);
		quad = shape_maker::quad();
		fram = shape_maker::frame();
		r_cube = shape_maker::cube();

		{
			gltf_loader l;
			l.load_to_renderable("../assets/low_poly_tree.glb", tree_renderable, tree_box);
		}
		{
			gltf_loader l;
			l.load_to_renderable("../assets/car_low-poly.glb", car_renderable, car_box);
		}
		{
			gltf_loader l;
			l.load_to_renderable("../assets/camera.glb", camera_renderable, camera_box);
		}
		{
			gltf_loader l;
			l.load_to_renderable("../assets/low_poly_street_light.glb", lamp_renderable, lamp_box);
		}
	}

	
	cube = shape_maker::cube();
	see_lighted.create_program("./shaders/visualize_depth.vert", "./shaders/visualize_depth.frag");
	show_frame.create_program("./shaders/show_frame.vert", "./shaders/show_frame.frag");
	
	
	
	/*	
		######################
		Initialize all lights.
		######################
	*/
	{
		// Initialize sun light
		{
			float zoom = 0.35;
			glm::mat4 sun_proj = glm::ortho(-zoom, zoom, -zoom, zoom, .5f, 1.5f);
			glm::mat4 sun_view = glm::lookAt(r.sunlight_direction(), glm::vec3(0), glm::vec3(0, 1, 0));
			sun.init(sun_view, sun_proj, 3000, 3000);
		}

		// Calculate lamp closest points to the track.
		for (int i = 0; i < r.lamps().size(); i++) {
			glm::vec4 curr_pos = glm::vec4(r.lamps()[i].pos, 1.);
			glm::vec3 closest = getClosestPoint(glm::vec3(curr_pos), r.t());
			closest_points.push_back(closest);
		}

		// Initialize lamp lights.
		{
			matrix_stack stack;
			float s = 1.f / r.bbox().diagonal();
			glm::vec3 c = r.bbox().center();
			stack.mult(glm::scale(glm::mat4(1.f), glm::vec3(s)));
			stack.mult(glm::translate(glm::mat4(1.f), -c));
			stack.push();

			terrain ter = r.ter();

			std::cout << "num lights : " << r.lamps().size() << std::endl;
			for (int i = 0; i < std::min(r.lamps().size(), (size_t)19); i++) {
				stack.push();
				glm::vec4 curr_pos = glm::vec4(r.lamps()[i].pos, 1.);
				glm::vec4 closest = glm::vec4(closest_points[i].x, curr_pos.y, closest_points[i].z, 1.);

				// transform curr_pos and closest in world space
				curr_pos = stack.m() * curr_pos;
				closest = stack.m() * closest;

				glm::vec4 closest_dir = closest - curr_pos;
				closest_dir.w = 0;
				closest_dir = glm::normalize(closest_dir);

				glm::vec3 light_pos = glm::vec3(curr_pos + closest_dir * 0.007f);
				light_pos.y += 0.015;

				glm::vec3 rot_axis = glm::cross(glm::vec3(closest_dir), glm::vec3(0, 1, 0));
				float angle_multiplier = glm::dot(ter.normal(closest_points[i]), glm::vec3(0, 1, 0));
				glm::mat4 rotation = glm::rotate(glm::mat4(1.), glm::radians(25.f) * angle_multiplier, rot_axis);
				glm::vec3 look_dir = glm::mat3(rotation) * glm::vec3(0, -1, 0);

				glm::mat4 light_view = glm::lookAt(light_pos, light_pos + look_dir, glm::vec3(1, 0, 0)); // lamp lights look straight down.
				glm::mat4 light_proj = glm::perspective(glm::radians(100.f), 1.f, 0.005f, 0.05f);

				Light l;
				l.init(light_view, light_proj, 300, 300);
				lampLights.push_back(l);

				stack.pop();
			}
		}

		//	Initialize car lights;
		{
			float s = 1.f / r.bbox().diagonal();
			glm::vec3 c = r.bbox().center();
			for (int i = 0; i < r.cars().size(); i++) {
				glm::mat4 frame = r.cars()[i].frame;

				frame = glm::translate(glm::mat4(1), -c) * frame;
				frame[3] = glm::scale(glm::mat4(1), glm::vec3(s)) * frame[3] + glm::vec4(0, 0.05, 0, 0);
				frame = glm::inverse(frame);

				glm::mat4 car_view = frame;
				glm::mat4 car_proj = glm::perspective(glm::radians(50.f), 1.f, 0.01f, 0.05f);

				Light l;
				l.init(car_view, car_proj, 200, 200);
				carLights.push_back(l);
			}
		}

		// initialize lights ubo.
		{
			light_struct ls;
			// load sun light
			ls.lightSpaceMatrix[0] = sun.lightSpaceTransform();
			ls.lightDir[0] = glm::vec4(sun.getLightDir(), 0.);
			ls.lightColor[0] = glm::vec4(sun.getColor(), 0.);
			for (int i = 0; i < 19; i++) {
				ls.lightSpaceMatrix[i + 1] = lampLights[i].lightSpaceTransform();
				ls.lightDir[i + 1] = glm::vec4(lampLights[i].getLightDir(), 0.);
				ls.lightColor[i + 1] = glm::vec4(lampLights[i].getColor(), 0.);
			}
			for (int i = 0; i < 10; i++) {
				ls.lightSpaceMatrix[i + 1 + 19] = carLights[i].lightSpaceTransform();
				ls.lightDir[i + 1 + 19] = glm::vec4(carLights[i].getLightDir(), 0.);
				ls.lightColor[i + 1 + 19] = glm::vec4(carLights[i].getColor(), 0.);
			}
			glGenBuffers(1, &lightsUBO);
			glBindBuffer(GL_UNIFORM_BUFFER, lightsUBO);
			glBufferData(GL_UNIFORM_BUFFER, sizeof(light_struct), (void*)&ls, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);
		}
	}



	/*
		##########################
		Initialize all model ubos.
		##########################
	*/
	{
		matrix_stack stack;
		stack.load_identity();
		stack.push();

		float s = 1.f / r.bbox().diagonal();
		glm::vec3 c = r.bbox().center();
		stack.mult(glm::scale(glm::mat4(1.f), glm::vec3(s)));
		stack.mult(glm::translate(glm::mat4(1.f), -c));

		// initialize tree models ubo;
		{
			Modelmatrices m;
			for (int i = 0; i < std::min((size_t)20, r.trees().size()); i++) {
				stack.push();
				glm::vec3 curr_pos = r.trees()[i].pos;
				stack.mult(glm::translate(glm::mat4(1.), curr_pos));
				stack.mult(glm::translate(glm::mat4(1.), glm::vec3(0, 1, 0) * 2.53));
				stack.mult(glm::scale(glm::mat4(1.), glm::vec3(7.3)));
				m.Model[i] = stack.m();
				stack.pop();
			}
			glGenBuffers(1, &tree_modelmatrices_UBO);
			glBindBuffer(GL_UNIFORM_BUFFER, tree_modelmatrices_UBO);
			glBufferData(GL_UNIFORM_BUFFER, sizeof(Modelmatrices), (void*)&m, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);
		}

		// initialize lamp models ubo;
		{
			Modelmatrices m;
			for (int i = 0; i < std::min(size_t(20),r.lamps().size()); i++) {
				stack.push();
				glm::vec4 curr_pos = glm::vec4(r.lamps()[i].pos, 1.);
				glm::vec2 xz_pos = glm::vec2(curr_pos.x, curr_pos.z);
				glm::vec2 closest = glm::vec2(closest_points[i].x, closest_points[i].z);
				glm::vec2 closest_dir = glm::normalize(closest - xz_pos);
				float cos_theta = closest_dir.x;
				float theta = closest_dir.y < 0 ? glm::acos(cos_theta) : -glm::acos(cos_theta);
				stack.mult(glm::translate(glm::mat4(1.), glm::vec3(curr_pos)));
				stack.mult(glm::rotate(glm::mat4(1), theta, glm::vec3(0, 1, 0)));
				stack.mult(glm::translate(glm::mat4(1.), glm::vec3(0.46, 0.93, 0)));
				stack.mult(glm::scale(glm::mat4(1.), glm::vec3(2.3)));
				m.Model[i] = stack.m();
				stack.pop();
			}
			glGenBuffers(1, &lamp_modelmatrices_UBO);
			glBindBuffer(GL_UNIFORM_BUFFER, lamp_modelmatrices_UBO);
			glBufferData(GL_UNIFORM_BUFFER, sizeof(Modelmatrices), (void*)&m, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);
		}

		// initialize car models ubo
		{
			Modelmatrices m;
			for (unsigned int i = 0; i < std::min(size_t(20),r.cars().size()); i++) {
				stack.push();
				stack.mult(r.cars()[i].frame);
				stack.mult(glm::translate(glm::mat4(1.f), glm::vec3(0, 0.1, 0.0)));
				stack.mult(glm::rotate(glm::mat4(1.f), glm::pi<float>(), glm::vec3(0, 1, 0)));
				stack.mult(glm::translate(glm::mat4(1.), glm::vec3(0, 1, 0) * 0.15));
				stack.mult(glm::scale(glm::mat4(1.), glm::vec3(1.9)));
				m.Model[i] = stack.m();
				stack.pop();
			}
			glGenBuffers(1, &car_modelmatrices_UBO);
			glBindBuffer(GL_UNIFORM_BUFFER, car_modelmatrices_UBO);
			glBufferData(GL_UNIFORM_BUFFER, sizeof(Modelmatrices), (void*)&m, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);
		}

		// initialize camera models ubo
		{
			Modelmatrices m;
			for (unsigned int i = 0; i < std::min(size_t(20), r.cameramen().size()); i++) {
				stack.push();
				stack.mult(r.cameramen()[i].frame);
				stack.mult(glm::rotate(glm::mat4(1.f), glm::pi<float>(), glm::vec3(0, 1, 0)));
				stack.mult(glm::scale(glm::mat4(1.f), glm::vec3(1)));
				stack.mult(glm::translate(glm::mat4(1.f), glm::vec3(0, 1, 0) * 0));
				m.Model[i] = stack.m();
				stack.pop();
			}
			glGenBuffers(1, &camera_modelmatrices_UBO);
			glBindBuffer(GL_UNIFORM_BUFFER, camera_modelmatrices_UBO);
			glBufferData(GL_UNIFORM_BUFFER, sizeof(Modelmatrices), (void*)&m, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);
		}
	}


	/* use the program shader "program_shader" */
	glUseProgram(basic_shader.program);

	/* define the viewport  */
	glViewport(0, 0, 800, 800);

	tb[0].reset();
	tb[0].set_center_radius(glm::vec3(0, 0, 0), 1.f);
	curr_tb = 0;

	
	proj = glm::perspective(glm::radians(45.f), 1.f, .001f, 2.f);

	view_saved = view;

	GLint maxVarying;
	glGetIntegerv(GL_MAX_VARYING_COMPONENTS, &maxVarying);
	std::cout << "Max varying components: " << maxVarying << std::endl;

	GLint maxTextures;
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextures);
	std::cout << "Max texture units: " << maxTextures << std::endl;

	r.start(5, 0, 0, 500);
	r.update();

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	
	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		check_gl_errors(__LINE__, __FILE__);

		processInput(window);

		r.update();

		view = interface_view;

		/*
			##############################
			Update car and cameramen ubos.
			##############################
		*/
		{
			matrix_stack stack;
			stack.load_identity();
			stack.push();

			float s = 1.f / r.bbox().diagonal();
			glm::vec3 c = r.bbox().center();
			stack.mult(glm::scale(glm::mat4(1.f), glm::vec3(s)));
			stack.mult(glm::translate(glm::mat4(1.f), -c));

			Modelmatrices m;
			for (unsigned int i = 0; i < r.cars().size(); i++) {
				stack.push();
				stack.mult(r.cars()[i].frame);
				stack.mult(glm::translate(glm::mat4(1.f), glm::vec3(0, 0.1, 0.0)));
				stack.mult(glm::rotate(glm::mat4(1.f), glm::pi<float>(), glm::vec3(0, 1, 0)));
				stack.mult(glm::translate(glm::mat4(1.), glm::vec3(0, 1, 0) * 0.15));
				stack.mult(glm::scale(glm::mat4(1.), glm::vec3(1.9)));
				m.Model[i] = stack.m();
				stack.pop();
			}
			glBindBuffer(GL_UNIFORM_BUFFER, car_modelmatrices_UBO);
			glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4) * r.cars().size(), (void*)m.Model);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			for (unsigned int i = 0; i < r.cameramen().size(); i++) {
				stack.push();
				stack.mult(r.cameramen()[i].frame);
				stack.mult(glm::rotate(glm::mat4(1.f), glm::pi<float>(), glm::vec3(0, 1, 0)));
				stack.mult(glm::scale(glm::mat4(1.f), glm::vec3(1)));
				stack.mult(glm::translate(glm::mat4(1.f), glm::vec3(0, 1, 0) * 0));
				m.Model[i] = stack.m();
				stack.pop();
			}
			glBindBuffer(GL_UNIFORM_BUFFER, camera_modelmatrices_UBO);
			glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4)* r.cameramen().size(), (void*)m.Model);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);
		}

		
		/*
			###########################################
			Update the sun. (view, proj, ubo, depthmap)
			###########################################
		*/
		{
			
			// update the sun's view and projection matrices.
			{
				glm::mat4 sun_view = glm::lookAt(r.sunlight_direction(), glm::vec3(0), glm::vec3(0, 1, 0));
				sun.setView(sun_view);

				glm::mat4 scene_view = glm::lookAt(glm::vec3(0, 0, -0.5), glm::vec3(0., 0, 1), glm::vec3(0, 1, 0));
				glm::mat4 scene_proj = glm::ortho(-0.35f, 0.35f, -0.01f, 0.05f, 0.13f, 0.87f);
				glm::mat4 scene_frustum = scene_proj * scene_view;
				glm::mat4 inv_scene_frustum = glm::inverse(scene_frustum);

				glm::mat4 cam_view = view_saved;
				glm::mat4 cam_proj = proj;

				// find optimal near and far values for cam_proj
				{
					float f = std::numeric_limits<float>::lowest();

					for (float x = -1; x <= 1; x += 2)
						for (float y = -1; y <= 1; y += 2)
							for (float z = -1; z <= 1; z += 2) {
								glm::vec4 p = glm::vec4(x, y, z, 1.0f);
								glm::vec4 scene_v = inv_scene_frustum * p;
								scene_v /= scene_v.w;
								glm::vec3 cam_v = cam_view * scene_v;
								cam_v.z = -cam_v.z;
								if (cam_v.z > f) {
									f = cam_v.z;
								}
							}

					f = glm::max(0.0001f, f);

					GLint vp[4];
					glGetIntegerv(GL_VIEWPORT, vp);
					cam_proj = glm::perspective(glm::radians(45.f), float(vp[2]) / float(vp[3]), 0.001f, f);
				}

				proj_saved = cam_proj;

				glm::mat4 cam_frustum = cam_proj * cam_view;
				glm::mat4 inv_cam_frustum = glm::inverse(cam_frustum);

				// calculate sun projection matrix

				glm::vec3 scene_min(std::numeric_limits<float>::max());
				glm::vec3 scene_max(std::numeric_limits<float>::lowest());
				glm::vec3 cam_min(std::numeric_limits<float>::max());
				glm::vec3 cam_max(std::numeric_limits<float>::lowest());

				for (float x = -1; x <= 1; x += 2)
					for (float y = -1; y <= 1; y += 2)
						for (float z = -1; z <= 1; z += 2) {
							glm::vec4 p = glm::vec4(x, y, z, 1.0f);

							glm::vec4 scene_v = inv_scene_frustum * p;
							scene_v /= scene_v.w;
							scene_v = sun_view * scene_v;
							scene_v.z = -scene_v.z;
							scene_min = glm::min(scene_min, glm::vec3(scene_v));
							scene_max = glm::max(scene_max, glm::vec3(scene_v));

							glm::vec4 cam_v = inv_cam_frustum * p;
							cam_v /= cam_v.w;
							cam_v = sun_view * cam_v;
							cam_v.z = -cam_v.z;
							cam_min = glm::min(cam_min, glm::vec3(cam_v));
							cam_max = glm::max(cam_max, glm::vec3(cam_v));
						}

				glm::vec3 intersect_min = glm::max(scene_min, cam_min);
				glm::vec3 intersect_max = glm::min(scene_max, cam_max);

				glm::mat4 sun_proj = glm::ortho(intersect_min.x, intersect_max.x,
					intersect_min.y, intersect_max.y,
					intersect_min.z, intersect_max.z);
				sun.setProj(sun_proj);
			}


			// update lights ubo with sun light.
			glBindBuffer(GL_UNIFORM_BUFFER, lightsUBO);
			glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(sun.lightSpaceTransform()));
			glBufferSubData(GL_UNIFORM_BUFFER, offsetof(light_struct, lightDir), sizeof(glm::vec3), glm::value_ptr(sun.getLightDir()));
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			// update sun depth map.
			sun.updateDepthMap(*render_scene);
		}
		

		/*
			#######################
			Update lamp depth maps.
			#######################
		*/
		for (Light& l : lampLights) {
			l.updateDepthMap(*render_scene);
		}
		

		/*
			##############################################
			Update car lights. (ubo, depthmap, view, proj)
			##############################################
		*/
		{
			light_struct ls;

			for (int i = 0; i < r.cars().size(); i++) {
				float s = 1.f / r.bbox().diagonal();
				glm::vec3 c = r.bbox().center();
				glm::mat4 frame = r.cars()[i].frame;
				frame = glm::translate(glm::mat4(1), -c) * frame;
				frame[3] = glm::scale(glm::mat4(1), glm::vec3(s)) * frame[3];

				// use origin and dir to calculate view

				glm::vec3 origin = frame[3]; 
				glm::vec3 dir = -frame[2];
				glm::vec3 up = frame[1];
				origin += up * 0.009;
				origin += -dir * 0.003;

				glm::vec3 rot_axis = glm::cross(up, dir);
				dir = glm::rotate(glm::mat4(1.), glm::radians(20.f), rot_axis) * glm::vec4(dir,0.);

				frame = glm::lookAt(origin, origin + dir, up);

				glm::mat4 car_view = frame;
				glm::mat4 car_proj = glm::perspective(0.7, 1.2, 0.011, 0.14);
				carLights[i].setView(car_view);
				carLights[i].setProj(car_proj);
				carLights[i].updateDepthMap(*render_scene);

				ls.lightDir[i] = glm::vec4(dir,1.);
				ls.lightSpaceMatrix[i] = carLights[i].lightSpaceTransform();
			}


			size_t offsetMatrix = offsetof(light_struct, lightSpaceMatrix) + sizeof(glm::mat4) * 20;
			size_t offsetDir = offsetof(light_struct, lightDir) + sizeof(glm::vec4) * 20;

			glBindBuffer(GL_UNIFORM_BUFFER, lightsUBO);
			glBufferSubData(GL_UNIFORM_BUFFER, offsetMatrix, sizeof(glm::mat4) * 10, (void*)ls.lightSpaceMatrix);
			glBufferSubData(GL_UNIFORM_BUFFER, offsetDir, sizeof(glm::vec4) * 10, (void *)ls.lightDir);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);
		}

		/*
			#################
			Render the scene.
			#################
		*/
		{
			glClearColor(0.3f, 0.3f, 0.3f, 1.f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
			render_scene(proj, view);
		}


		/*
			########################
			Show frustums if needed.
			########################
		*/
		{
			glUseProgram(show_frame.program);
			glUniformMatrix4fv(show_frame["uProj"], 1, GL_FALSE, glm::value_ptr(proj));
			glUniformMatrix4fv(show_frame["uView"], 1, GL_FALSE, glm::value_ptr(view));
			glUniform3f(show_frame["uColor"], 1, 0, 0);

			cube.bind();

			glDisable(GL_CULL_FACE);

			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

			if (show_lamps_frustum)
				for (int i = 0; i < lampLights.size(); i++) {
					Light& l = lampLights[i];
					glm::mat4 m = l.lightSpaceTransform();
					glUniformMatrix4fv(show_frame["uWorldToFrame"], 1, GL_FALSE, glm::value_ptr(m));
					glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
				}

			if (show_sun_frustum)
			{
				glm::mat4 m = sun.lightSpaceTransform();
				glUniformMatrix4fv(show_frame["uWorldToFrame"], 1, GL_FALSE, glm::value_ptr(m));
				glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
			}

			glUniform3f(show_frame["uColor"], 1, 1, 0);

			if (show_scene_frustum)
			{
				glm::mat4 view = glm::lookAt(glm::vec3(0, 0, -0.5), glm::vec3(0., 0, 1), glm::vec3(0, 1, 0));
				glm::mat4 proj = glm::ortho(-0.35, 0.35, -0.01, 0.05, 0.13, 0.87);
				glm::mat4 m = proj * view;
				glUniformMatrix4fv(show_frame["uWorldToFrame"], 1, GL_FALSE, glm::value_ptr(m));
				glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
			}

			if (show_carlight_frustum)
				for (int i = 0; i < carLights.size(); i++) {
					Light& l = carLights[i];
					glm::mat4 m = l.lightSpaceTransform();
					glUniformMatrix4fv(show_frame["uWorldToFrame"], 1, GL_FALSE, glm::value_ptr(m));
					glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
				}

			if (show_saved_frustum)
			{
				glUniform3f(show_frame["uColor"], 0, 1, 0);
				glm::mat4 proj = glm::perspective(glm::radians(45.), 1., 0.001, 0.05);
				glm::mat4 m = proj_saved * view_saved;
				glUniformMatrix4fv(show_frame["uWorldToFrame"], 1, GL_FALSE, glm::value_ptr(m));
				glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
			}

			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

			glEnable(GL_CULL_FACE);

		}


		/*
			Render user interface
		*/
		userInterface_render();

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();
	}
	glUseProgram(0);

	userInterface_destroy();

	glfwTerminate();
	return 0;
}


