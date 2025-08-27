
#version 460 core

in vec3 viewDir;

uniform samplerCube cubemap;
uniform vec3 uLightDir;
uniform vec3 uSkyColor;

out vec4 FragColor;

void main() {
	float intensity = max(0., dot(normalize(viewDir), uLightDir));
	intensity = pow(intensity, 400.);
    vec3 sky_albedo = texture(cubemap, viewDir).xyz;
	FragColor = vec4(uSkyColor*sky_albedo+vec3(intensity), 1.);
}