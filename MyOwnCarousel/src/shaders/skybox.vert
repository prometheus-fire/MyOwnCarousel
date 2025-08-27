
#version 460 core

layout (location=0) in vec3 aPosition;

uniform mat4 uInvProj;
uniform mat4 uInvView;

out vec3 viewDir;

void main() {
	/*
		Calculate fragment look direction in
		world space.
	*/
	vec4 near = vec4(aPosition.xy, -1., 1.);
	vec4 far = vec4(aPosition.xy, 1., 1.);

	// transform near and far in view space
	near = uInvProj * near;
	near = near / near.w;
	far = uInvProj * far;
	far = far / far.w;

	vec4 dir = far - near;
	dir.w = 0.;
	
	// transform dir in view space
	dir = uInvView * dir;

	viewDir = normalize( dir.xyz );

	gl_Position = vec4(aPosition.xy, 0.999999, 1.);
}