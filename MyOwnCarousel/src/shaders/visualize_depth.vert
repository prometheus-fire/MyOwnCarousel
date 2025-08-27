#version 460 core 
layout (location = 0) in vec3 aPosition; 

uniform mat4 uProj;
uniform mat4 uView;
uniform mat4 uModel;

uniform mat4 lightSpaceMatrix;

out vec4 lightSpacePosition;

void main(void) 
{ 
    vec4 world_space_pos = uModel * vec4(aPosition, 1.0);
    vec4 light_space_pos = lightSpaceMatrix * world_space_pos;
    lightSpacePosition = light_space_pos;
    gl_Position = uProj*uView*world_space_pos; 
}