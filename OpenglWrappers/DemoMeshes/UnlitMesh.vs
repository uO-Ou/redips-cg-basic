#version 430 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texcoord;

out Pipe{
	vec3 FragPos;
	vec3 Normal;
	vec2 TexCoord;
}vsOutput;

uniform mat4 model;
uniform mat4 projection_view;

void main(){
   vec4 inworld = model * vec4(position,1.0f);
   vsOutput.FragPos = inworld.xyz;

   vsOutput.TexCoord = texcoord;

   mat3 normalMatrix = transpose(inverse(mat3(model)));
   vsOutput.Normal = normalMatrix * normal;

   gl_Position = projection_view * inworld;
}