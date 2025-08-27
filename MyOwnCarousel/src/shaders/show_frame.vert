#version 420 core

layout (location = 0) in vec3 aPosition;

uniform mat4 uProj;
uniform mat4 uView;
uniform mat4 uWorldToFrame;

void main(void) {
	mat4 inv_frame = inverse(uWorldToFrame);
	vec4 worldPos = inv_frame * vec4(aPosition, 1.);
	worldPos /= worldPos.w;
	gl_Position = uProj * uView * worldPos;
}