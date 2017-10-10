#version 430 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 3) in vec3 translate; 

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

out vec3 Normal;
out vec3 FragPos;

void main(){
    vec4 inworld = model * vec4(position,1.0f) + vec4(translate,0.0f);
	gl_Position = projection * view * inworld;
	
	FragPos = inworld.xyz;

	mat3 normalMatrix = transpose(inverse(mat3(model)));
	Normal = normalMatrix * normal;
}