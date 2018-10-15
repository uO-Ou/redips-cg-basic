#version 430 core
out vec4 Color;

in vec3 light;
in vec2 TexCoord;

//material
uniform struct Material{
	uint flags; //0:no texture,1:ambientTexture,2:diffuseTexture,3:both
	vec3 diffuse,ambient;
	sampler2D diffuseTexture;
	sampler2D ambientTexture;
}material;

void main(){    

	vec3 Ambient = vec3(0,0,0), Diffuse = vec3(0,0,0);
	if((material.flags&1u)==0) Ambient = material.ambient;
	else Ambient = (texture(material.ambientTexture,TexCoord).bgr * material.ambient);

	if((material.flags&2u)==0) Diffuse = material.diffuse;
	else Diffuse = (texture(material.diffuseTexture,TexCoord).bgr * material.diffuse);

    Color = vec4(Diffuse*light.bgr,1.0);
}