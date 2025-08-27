#version 460 core

in vec4 lightSpacePosition;
uniform sampler2D depthmap;
out vec4 FragColor;

void main() {
    vec3 projCoords = lightSpacePosition.xyz / lightSpacePosition.w;
    projCoords = projCoords * 0.5 + 0.5;

    float curr_z = projCoords.z;
    float depth_z = texture(depthmap, projCoords.xy).r;

    float bias = 0.0005;
    FragColor = vec4(1., 0., 0., 1.); // rosso = in ombra
    if (curr_z < depth_z + bias) // verde = visibile alla luce
        FragColor = vec4(0., 1., 0., 1.);
}
