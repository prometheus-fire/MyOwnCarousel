#pragma once

#include <GL/glew.h>
#include <stdio.h>
#include <string>
#include <sstream>
#include <iostream>
#include <map>
#include <fstream>
#include <regex>
#include <unordered_map>
#include "../common/debugging.h"

template <typename... Args>
std::vector<std::string> join(Args... args) {
	// Initialize a vector of strings
	std::vector<std::string> vec = { args... };
	return vec;
}

struct shader {
	GLuint   vertex_shader, geometry_shader, compute_shader, fragment_shader, program;

	std::map<std::string, int> uni;
	std::vector<std::pair<unsigned int,std::string>> att;
	
	std::string name = "boh";

	std::unordered_map<std::string, GLuint> uniforms;

	void bind_uniform(std::string name) {
		uni[name] = glGetUniformLocation(program, name.c_str());
	}

	const bool has_uniform(std::string name) const {
		return (uni.find(name) != uni.end());
	}

	void bind_attribute(std::string name, unsigned int id) {
		att.push_back(std::pair<unsigned int, std::string>(id,name));
	}



	int operator[](std::string name) {
		/*if (uni.find(name) == uni.end()) {
			std::cout << "No location for uniform variable " << name << std::endl;
			exit(0);
		}
		return uni[name];*/

		if (uniforms.find(name) == uniforms.end()) {
			GLuint loc = glGetUniformLocation(this->program, name.data());
			if (loc == -1) {
				std::cout << "Error: could not find location for " << name << ". Program id: " << this->name << std::endl;
				exit(0);
			}
			uniforms[name] = loc;
		}
		return uniforms[name];
	}
#if defined(GL_VERSION_4_3)
	void  create_program(std::vector<std::string>comp_shader) {
		std::string compute_shader_src_code;
		for (unsigned int i = 0; i < comp_shader.size();++i)
			compute_shader_src_code += textFileRead(comp_shader[i].c_str()) + "\n";
		create_program(compute_shader_src_code);
	}
	void  create_program(const char* nameC) {
		std::string compute_shader_src_code = textFileRead(nameC);
		create_program(compute_shader_src_code);
	}

	void  create_program(std::string compute_shader_src_code) {
		
		create_shader(compute_shader_src_code.c_str(), GL_COMPUTE_SHADER);
		program = glCreateProgram();
		glAttachShader(program, compute_shader);

		glLinkProgram(program);

		bind_uniform_variables(compute_shader_src_code);
		check_shader(compute_shader);
		validate_shader_program(program);
	}
#endif
	/* create a program shader */
	void  create_program(std::vector<std::string> vert_shader, const char * frag_name) {
		std::vector<std::string> frag_shader;
		frag_shader.push_back(frag_name);
		create_program(vert_shader, frag_shader);
	}
	void  create_program( const char* vert_name,  std::vector<std::string> frag_shader) {
		std::vector<std::string> vert_shader;
		frag_shader.push_back(vert_name);
		create_program(vert_shader, frag_shader);
	}

	void  create_program(std::vector<std::string> vert_shader, std::vector<std::string> frag_shader) {
		std::cout << "creating program\n vertex shader " <<  std::endl;
		for (unsigned int ii = 0; ii < vert_shader.size(); ++ii)
			std::cout << vert_shader[ii];

		std::cout << "\n fragment shader " << std::endl;

		for (unsigned int ii = 0; ii < frag_shader.size(); ++ii)
			std::cout << frag_shader[ii];

		std::string vertex_shader_src_code;
		for(unsigned int i=0; i < vert_shader.size();++i)
			vertex_shader_src_code +=  textFileRead(vert_shader[i].c_str())+ "\n";

		std::string fragment_shader_src_code;
		for (unsigned int i = 0; i < frag_shader.size();++i)
			fragment_shader_src_code +=  textFileRead(frag_shader[i].c_str()) + "\n";
		create_program(vertex_shader_src_code, fragment_shader_src_code);
	}
	void  create_program(const GLchar* nameV, const char* nameF) {
		this->name = nameV;
		std::string vertex_shader_src_code = textFileRead(nameV);
		std::string fragment_shader_src_code = textFileRead(nameF);
		create_program(vertex_shader_src_code, fragment_shader_src_code);
	}
	void  create_program(std::string vertex_shader_src_code, std::string fragment_shader_src_code) {
		
		create_shader(vertex_shader_src_code.c_str(), GL_VERTEX_SHADER);
		create_shader(fragment_shader_src_code.c_str(), GL_FRAGMENT_SHADER);

		program = glCreateProgram();
		glAttachShader(program, vertex_shader);
		glAttachShader(program, fragment_shader);

		for (unsigned int ia = 0; ia < att.size(); ++ia) 
			glBindAttribLocation(program, att[ia].first, att[ia].second.c_str());
		
		glLinkProgram(program);

		bind_uniform_variables(vertex_shader_src_code);
		bind_uniform_variables(fragment_shader_src_code);

		check_shader(vertex_shader);
		check_shader(fragment_shader);
		validate_shader_program(program);
	}

private:
	static  std::string textFileRead(const char* fn) {
		std::ifstream ifragment_shader(fn);
		std::string content((std::istreambuf_iterator<char>(ifragment_shader)),
			(std::istreambuf_iterator<char>()));
		if (content.empty()) {
			std::cout << "No content for " << fn << std::endl;
			exit(0);
		}
		return content;
	}

	bool create_shader(const GLchar* src, unsigned int SHADER_TYPE) {
		GLuint s = 0;
		switch (SHADER_TYPE) {
		case GL_VERTEX_SHADER:   s = vertex_shader = glCreateShader(GL_VERTEX_SHADER);break;
		case GL_FRAGMENT_SHADER: s = fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);break;
#if defined(GL_VERSION_4_3)
		case GL_COMPUTE_SHADER:  s = compute_shader = glCreateShader(GL_COMPUTE_SHADER);break;
#endif
		}

		glShaderSource(s, 1, &src, NULL);
		glCompileShader(s);
		int status;
		glGetShaderiv(s, GL_COMPILE_STATUS, &status);
		if (status != GL_TRUE) {
			check_shader(s);
			return false;
		}
		return true;
	}

	/* this function looks for uniform variables and associates their locations
	 to their name in the map "uni" so that you can call
	 glUniform*(my_shader["uMyUniformName"],...)
	 DISCLAIMER : this does NOT do a PROPER parsing of the code.
	 If your declare a list of uniforms with a single instance of the qualifier:

	 uniform float a,b[10],c;

	 it will only see c.
	 If you declare uniforms by themselves:

	 uniform float a;
	 uniform float b[10];
	 uniform float c;

	 it'll be fine
	 */
	void bind_uniform_variables(std::string code) {
		std::replace(code.begin(), code.end(), '\n', ';');
		code.erase(std::remove(code.begin(), code.end(), '\r'), code.end());
		code.erase(std::remove(code.begin(), code.end(), '\n'), code.end());
		code.erase(std::remove(code.begin(), code.end(), '\t'), code.end());
		code.erase(std::remove(code.begin(), code.end(), '\b'), code.end());

		size_t pos;
		std::istringstream check1(code);

		std::string intermediate;
		std::vector <std::string> tokens;
		// Tokenizing w.r.t. space ' '
		while (getline(check1, intermediate, ';'))
		{
			intermediate = std::regex_replace(intermediate, std::regex("  "), " ");

			if (intermediate.find(" ") == 0)
				intermediate.erase(0, 1);

			if (intermediate.find("uniform") == 0) {
				pos = intermediate.find_last_of(" ");
				size_t pos_end = intermediate.find_first_of("[") - 1;
				if (pos_end > intermediate.length())
					pos_end = intermediate.length();
				std::string uniform_name = intermediate.substr(pos + 1, pos_end - pos);
				this->bind_uniform(uniform_name);
				tokens.push_back(intermediate.substr(pos + 1, intermediate.length() - pos));
			}
		}
	}

};


