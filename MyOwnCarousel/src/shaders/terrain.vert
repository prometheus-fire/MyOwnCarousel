#version 460 core 

layout (location = 0) in vec3 aPosition;  
layout (location = 1) in vec3 aNormal;

uniform mat4 uProj;
uniform mat4 uView;
uniform mat4 uModel;


out vec2 vTexCoords;
out vec3 vNormal;
out vec2 vUV; // x and y coordinates of the fragment in ndc space
out vec3 vPosition;
out mat4 vModel;


void main(void) 
{ 
	mat3 NormalMatrix = mat3(transpose(inverse(uModel * uView)));
	vec3 view_normal = NormalMatrix * aNormal;
	vTexCoords = aPosition.xz * .5;
	vNormal = normalize(view_normal);
    gl_Position = uProj*uView*uModel*vec4(aPosition, 1.0); 
	vUV = gl_Position.xy / gl_Position.w;
	vPosition = aPosition;
	vModel = uModel;
}