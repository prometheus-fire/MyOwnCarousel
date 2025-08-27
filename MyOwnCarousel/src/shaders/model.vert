#version 460 core 
layout (location = 0) in vec3 aPosition;  
layout (location = 2) in vec3 aNormal;
layout (location = 4) in vec2 aTexCoords;

uniform mat4 uProj;
uniform mat4 uView;
uniform mat4 uModel;

layout (std140) uniform Models {
	mat4 Model[20]; // maximum number of instanced models does not exceed 20
};


out vec2 vTexCoords;
out vec3 vNormal;
out vec2 vUV; // x and y coordinates of the fragment in ndc space
out vec3 vPosition;
out mat4 vModel;

void main(void) 
{ 
	vModel =  Model[gl_InstanceID] * uModel ;
	mat3 NormalMatrix = mat3(transpose(inverse(uView * vModel)));
	vec3 view_normal = NormalMatrix * aNormal;
	vTexCoords = aTexCoords;
	vNormal = normalize(view_normal);
    gl_Position = uProj*uView*vModel*vec4(aPosition, 1.0);
	vUV = gl_Position.xy / gl_Position.w;
	vPosition = aPosition;
}