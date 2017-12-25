#version 430 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texcoord;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main(){
    vec4 worldPos = model * vec4(position, 1.0f);
    FragPos = worldPos.xyz; 
    gl_Position = projection * view * worldPos;
    
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    Normal = normalMatrix * normal;
	
	TexCord = texcoord;
}