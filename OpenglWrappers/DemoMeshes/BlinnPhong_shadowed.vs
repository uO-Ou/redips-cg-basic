#version 430 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texcoord;

uniform mat4 model;
uniform mat4 projection_view;
uniform mat4 light_space_ort_projection_view;

out Pipe{
	vec3 FragPos;
	vec3 LightSpaceProjPos;
	vec2 TexCoord;
	vec3 Normal;
}vsOutput;

void main(){
	  vec4 inworld = model * vec4(position,1.0f);
	  vsOutput.FragPos = inworld.xyz;

	  vsOutput.LightSpaceProjPos = (light_space_ort_projection_view * inworld).xyz * 0.5f + 0.5f;

	  vsOutput.TexCoord = texcoord;
	  
	  mat3 normalMatrix = transpose(inverse(mat3(model)));
	  vsOutput.Normal = normalMatrix * normal;

	  //calculate glPostion
	  gl_Position = projection_view * inworld;
}

