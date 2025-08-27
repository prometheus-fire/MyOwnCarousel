#version 460 core  

#define NUM_LIGHTS 30

in vec2 vTexCoords; 
in vec3 vNormal;
in vec2 vUV;
in vec3 vPosition;
in mat4 vModel;

uniform sampler2D depthmap[NUM_LIGHTS];

layout (std140) uniform Lights {
    mat4 lightSpaceMatrix[NUM_LIGHTS];
    vec3 lightDir[NUM_LIGHTS];
    vec3 lightColor[NUM_LIGHTS];
};

uniform sampler2D texture_diffuse;
uniform vec3 uColor;
uniform vec3 uSkyColor;
uniform mat4 uView;

out vec4 FragColor; 

float shadow(int i, vec3 projCoords, float dot_n_ldir) {

    vec3 uvCoords = projCoords * 0.5 + 0.5;

    float curr_z = uvCoords.z;
    float depth_z = texture(depthmap[i], uvCoords.xy).r ;

    float visibility = 0.0;
    vec2 texelSize = 1.0 / textureSize(depthmap[i], 0);
    for(int x = 0; x <= 0; ++x)
    {
        for(int y = 0; y <= 0; ++y)
        {
            float pcfDepth = texture(depthmap[i], uvCoords.xy + vec2(x, y) * texelSize).r; 
            float bias = 0.0005;
            if (i>=20) bias = max(0.05 * (1.0 - dot_n_ldir), 0.005);
            visibility += curr_z < pcfDepth + bias ? 1.0 : 0.;        
        }    
    }
    visibility /= 1.0;

    // only for lamps
        if (i>0 && i<20) {
            vec3 lspos = projCoords;
            float dot_light_dir = normalize(lspos).z;
            float t = min(1., dot_light_dir + 0.);
            float coeff = t - dot(lspos, lspos);
            coeff = max(coeff, 0.);
            visibility = visibility * coeff;
        }

        if (i>=20) {
            vec3 lspos = projCoords;
            float dot_light_dir = normalize(lspos).z;
            float t = min(1., dot_light_dir + 3.);
            float coeff = t - dot(lspos, lspos) * 0.45;
            coeff = max(coeff, 0.);
            visibility = visibility * coeff;
        }

    return visibility;
}  

void main(void) 
{ 

    vec3 result = vec3(0.);

    float ambient = 0.3;
    float kd = 0.88 - ambient;
    float ks = 1. - kd - ambient;
    float shininess = 32.;

    vec3 albedo;
    if (uColor.x >= 0) {
        albedo = uColor;
    }
    else  {
        vec4 albedo_4 = texture(texture_diffuse, vTexCoords);
        if (albedo_4.w < .1) discard;
        albedo = albedo_4.xyz;
    }

    vec3 normal = normalize(vNormal);
    vec3 viewdir = normalize( (uView * vModel * vec4(vPosition, 1.)).xyz);

    vec4 world_space_pos = vModel * vec4(vPosition,1.0);
    mat3 rotView = mat3(uView);

    for (int i=0; i<NUM_LIGHTS; i++) {
         vec4 lightSpacePos = lightSpaceMatrix[i] * world_space_pos;
         vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;

         if (
            abs(projCoords.x) > 1. ||
            abs(projCoords.y) > 1. ||
            abs(projCoords.z) > 1.
        ) continue;
        
        vec3 lightdir = normalize(rotView * lightDir[i]);
        float dot_n_ldir = dot(normal, lightdir);
        if (dot_n_ldir >= 0.) continue;

        vec3 lightcolor = i==0 ? uSkyColor : lightColor[i];

        float curr_shadow = shadow(i, projCoords, dot_n_ldir);
       
        float diffuse = max(0., dot(normal, -lightdir));
        vec3 H = normalize(lightdir + viewdir);
        float specular = pow( max(0., dot(H, normal)) , shininess);
        result += (kd * diffuse * albedo + specular * ks) * curr_shadow * lightcolor;
    }
    
    result += ambient * albedo * uSkyColor;

    FragColor = vec4(result, 1.); 
} 

