#version 430 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texcoord;
layout (location = 3) in vec3 tangent;

uniform mat4 model;
uniform mat4 projection_view;

out Pipe{
	vec3 FragPos;
	vec2 TexCoord;
	mat3 TBN;
}vsOutput;

void main(){
	  vec4 inworld = model * vec4(position,1.0f);
	  vsOutput.FragPos = inworld.xyz;

	  vsOutput.TexCoord = texcoord;
	  
	  mat3 normalMatrix = transpose(inverse(mat3(model)));
	  vec3 N = normalMatrix * normal;

	  vec3 T = mat3(model) * tangent;
	  
	  vec3 B = cross(T, N);
	  vsOutput.TBN = mat3(T, B, N);

	  //calculate glPostion
	  gl_Position = projection_view * inworld;
}

