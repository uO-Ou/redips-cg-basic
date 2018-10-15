#version 430 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texcoord;

uniform mat4 model;
uniform mat4 projection_view;
uniform mat4 lightSpaceMatrix;

out Pipe{
	vec3 Normal;
	vec3 FragPos;
	vec2 TexCoord;
	vec4 FragPosInLightSpace;
}vsOutput;

void main(){
	  //write to pipe
	  vec4 inworld = model * vec4(position,1.0f);
	  vsOutput.FragPos = inworld.xyz;
	  
	  mat3 normalMatrix = transpose(inverse(mat3(model)));
	  vsOutput.Normal = normalMatrix * normal;
	  
	  vsOutput.TexCoord = texcoord;

	  //calculate glPostion
	  gl_Position = projection_view * inworld;

	  //for shadow mapping
	  vsOutput.FragPosInLightSpace = lightSpaceMatrix * vec4(vsOutput.FragPos,1);
}