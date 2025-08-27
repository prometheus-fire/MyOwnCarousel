#version 460 core 
layout (location = 0) in vec3 aPosition; 

uniform mat4 uProj;
uniform mat4 uView;
uniform mat4 uModel;

layout (std140) uniform Models {
	mat4 Model[20]; // maximum number of instanced models does not exceed 20
};

void main(void) 
{ 
    gl_Position = uProj*uView*Model[gl_InstanceID]*uModel*vec4(aPosition, 1.0); 
}