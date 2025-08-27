#pragma once

/*
	This file holds a bunch of global variables used
	in multiple files.
	(There are more includes than needed, but I'm using pragma once so ...)
*/


#include ".\common\debugging.h"
#include ".\common\renderable.h"
#include ".\common\shaders.h"
#include ".\common\simple_shapes.h"
#include ".\common\carousel\carousel.h"
#include ".\common\carousel\carousel_to_renderable.h"
#include ".\common\carousel\carousel_loader.h"

#include <iostream>
#include <algorithm>
#include <conio.h>
#include <direct.h>
#include ".\common\matrix_stack.h"
#include ".\common\intersection.h"
#include ".\common\trackball.h"

/*
	Global FPS camera.
	Use WASD and mouse to control,
	space for going up and left shift for going down.
	Press ctrl to toggle camera movement.
*/

#include "Camera.h"
Camera global_camera(glm::vec3(0, 0.2, 0.5), glm::vec3(0.f, 0.2f, 0.f), glm::vec3(0.0, 1.f, 0.f));

/*
	Global race object.
*/

race r;

/*
	View frame according to user interface.
	See user_interface.h
*/
glm::mat4 interface_view;

glm::mat4 view_saved;
glm::mat4 proj_saved;

bool show_sun_frustum = false, 
show_lamps_frustum = false, 
show_scene_frustum = false, 
show_carlight_frustum = false,
show_saved_frustum = false;

bool update_view_saved = true;