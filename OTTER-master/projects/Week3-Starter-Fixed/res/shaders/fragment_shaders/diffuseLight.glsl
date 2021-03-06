#version 430

#include "../fragments/fs_common_inputs.glsl"


struct DiffuseLight
{
	vec3 color;
	vec3 direction;
	float factor;
	bool isOn;
};

vec3 getDiffuseLightColor(DiffuseLight diffuseLight, vec3 normal);


vec3 getDiffuseLightColor(DiffuseLight diffuseLight, vec3 normal)
{
	if(!diffuseLight.isOn) {
		return vec3(0.0, 0.0, 0.0);
	}
	
	float finalIntensity = max(0.0, dot(normal, -diffuseLight.direction));
	finalIntensity = clamp(finalIntensity*diffuseLight.factor, 0.0, 1.0);
	return vec3(diffuseLight.color*finalIntensity);
}