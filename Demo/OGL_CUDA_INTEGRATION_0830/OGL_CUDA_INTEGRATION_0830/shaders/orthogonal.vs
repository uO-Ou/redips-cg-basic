#version 430 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

uniform mat4 projection;
uniform mat4 model;

out vec3 Normal;
out vec3 FragPos;

void main()
{
	gl_Position = projection * model * vec4(position,1.0f);
	
	FragPos = position;

	mat3 normalMatrix = transpose(inverse(mat3(model)));
	Normal = normalMatrix * normal;
}