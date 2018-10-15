#version 430 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texcoord;

layout (location = 3) in vec3 SHC012;
layout (location = 4) in vec3 SHC345;
layout (location = 5) in vec3 SHC678;

out vec3 light;
out vec2 TexCoord;

uniform mat4 model;
uniform mat4 projection_view;

uniform vec3 light_r_012;
uniform vec3 light_r_345;
uniform vec3 light_r_678;
uniform vec3 light_g_012;
uniform vec3 light_g_345;
uniform vec3 light_g_678;
uniform vec3 light_b_012;
uniform vec3 light_b_345;
uniform vec3 light_b_678;

void main(){
    gl_Position = projection_view * model * vec4(position, 1.0f);

	light.r = clamp(dot(light_r_012, SHC012) + dot(light_r_345, SHC345) + dot(light_r_678, SHC678),0,1);
	light.g = clamp(dot(light_g_012, SHC012) + dot(light_g_345, SHC345) + dot(light_g_678, SHC678),0,1);
	light.b = clamp(dot(light_b_012, SHC012) + dot(light_b_345, SHC345) + dot(light_b_678, SHC678),0,1);

	TexCoord = texcoord;
}