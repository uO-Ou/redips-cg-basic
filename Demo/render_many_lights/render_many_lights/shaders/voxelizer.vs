#version 430 core
layout (location = 0) in vec3 position;

uniform mat4 projection;
uniform mat4 model;
uniform mat3 swizzler;

out vec3 FragPos;
out vec4 Inworld;

void main(){
    Inworld = ((model * vec4(position,1.0f)));
    gl_Position = projection * vec4((swizzler * Inworld.xyz),1.0f);
	FragPos = (transpose(swizzler))*((gl_Position.xyz)*0.5f+0.5f);
}