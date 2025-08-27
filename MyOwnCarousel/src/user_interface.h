#pragma once

#include <vector>
#include <string>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <GLFW/glfw3.h>


#include "globals.h"


/*
	This file includes some functions used
	for managing the ui of the carousel.
	These functions are used inside of the main function.
*/


void userInterface_init(GLFWwindow* window) {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	ImGui::StyleColorsDark(); // or ImGui::StyleColorsClassic();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");
}




void userInterface_render() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	

	// Variabili globali o statiche
	static int current_item = 0;

	//const char* items[] = { "FPS_Camera", "Opzione 2", "Opzione 3" };


	// Your vector of options
	std::vector<std::string> items = { "FPS camera" };

	for (int i = 0; i < r.cameramen().size(); i++) {
		items.push_back("Cameraman " + std::to_string(i));
	}

	// Convert vector to const char* array
	std::vector<const char*> itemPtrs;
	for (const auto& item : items) {
		itemPtrs.push_back(item.c_str());
	}


	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("POV")) {
			ImGui::Text("Select an option:");
			ImGui::Combo("Select Option", &current_item, itemPtrs.data(), static_cast<int>(itemPtrs.size()));
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Debug")) {
			ImGui::Checkbox("Show Sun frustum.", &show_sun_frustum);
			ImGui::Checkbox("Show lamps frustum.", &show_lamps_frustum);
			ImGui::Checkbox("Show scene frustum.", &show_scene_frustum);
			ImGui::Checkbox("Show car lights frustum.", &show_carlight_frustum);
			ImGui::Checkbox("Show saved view.", &show_saved_frustum);
			ImGui::Checkbox("Save current view.", &update_view_saved);
			if (ImGui::Button("save_view")) {
				view_saved = interface_view;
			}
			ImGui::EndMenu();
		}


		ImGui::EndMainMenuBar();
	}

	if (current_item == 0) {
		interface_view = global_camera.getViewMatrix();
	}
	else {
		float s = 1.f / r.bbox().diagonal();
		glm::vec3 c = r.bbox().center();

		glm::mat4 frame = r.cameramen()[current_item-1].frame;

		frame = glm::translate(glm::mat4(1), -c) * frame;
		frame[3] = glm::scale(glm::mat4(1), glm::vec3(s)) * frame[3] + glm::vec4(0,0.005,0,0);
		frame = glm::inverse(frame);

		interface_view = frame;
	}

	if (update_view_saved) {
		view_saved = interface_view;
	}


	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}


void userInterface_destroy() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}


