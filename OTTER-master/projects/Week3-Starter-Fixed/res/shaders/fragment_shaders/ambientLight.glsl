#version 430

#include "../fragments/fs_common_inputs.glsl"

// We output a single color to the color buffer
layout(location = 0) out vec4 frag_color;

uniform vec3 objectColor;

struct Material {
	sampler2D Diffuse;
	float     Shininess;
    float     ambientStrength;
};

// Create a uniform for the material
uniform Material u_Material;

#include "../fragments/multiple_point_lights.glsl"
#include "../fragments/frame_uniforms.glsl"
#include "../fragments/color_correction.glsl"

void main()
{
    vec3 normal = normalize(inNormal);

    vec3 lightColor = vec3(0.2f, 0.5f, 0.9f);
    float ambientStrength = 0.7f;

    vec4 textureColor = texture(u_Material.Diffuse, inUV);

    vec3 ambient = ambientStrength * lightColor * inColor * textureColor.rgb;

    frag_color = vec4(ambient, 1.0f);
}  