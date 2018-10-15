#version 430 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texcoord;

layout (location = 3) in vec4 SH_Part_00;
layout (location = 4) in vec4 SH_Part_01;
layout (location = 5) in vec4 SH_Part_02;
layout (location = 6) in vec4 SH_Part_03;

out Pipe{
	vec3 Normal;
	vec3 FragPos;
	vec2 TexCoord;
	vec3 SHLight;
}vsOutput;

uniform mat4 model;
uniform mat4 projection_view;

uniform vec4 light_0;
uniform vec4 light_1;
uniform vec4 light_2;
uniform vec4 light_3;

float _scale_() {
	float ret = dot(light_0, SH_Part_00)+dot(light_1, SH_Part_01)+dot(light_2, SH_Part_02)+dot(light_3, SH_Part_03);
	return clamp(ret,0,1);
}

void main(){
	vec4 inworld = model * vec4(position,1.0f);
	vsOutput.FragPos = inworld.xyz;
	  
	mat3 normalMatrix = transpose(inverse(mat3(model)));
	vsOutput.Normal = normalMatrix * normal;

    vsOutput.TexCoord = texcoord;

	float _s_ = _scale_();
	vsOutput.SHLight = vec3(_s_,_s_,_s_);

	gl_Position = projection_view * inworld;
}