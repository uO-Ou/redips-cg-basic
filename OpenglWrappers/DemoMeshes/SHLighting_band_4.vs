#version 430 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texcoord;

layout (location = 3) in vec4 SH_Part_0;
layout (location = 4) in vec4 SH_Part_1;
layout (location = 5) in vec4 SH_Part_2;
layout (location = 6) in vec4 SH_Part_3;
layout (location = 7) in vec3 SH_Part_4;
layout (location = 8) in vec3 SH_Part_5;
layout (location = 9) in vec3 SH_Part_6;

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
uniform vec3 light_4;
uniform vec3 light_5;
uniform vec3 light_6;

float _scale_() {
	float ret = dot(light_0, SH_Part_0)
	           +dot(light_1, SH_Part_1)
			   +dot(light_2, SH_Part_2)
			   +dot(light_3, SH_Part_3)
			   +dot(light_4, SH_Part_4)
			   +dot(light_5, SH_Part_5)
			   +dot(light_6, SH_Part_6);
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