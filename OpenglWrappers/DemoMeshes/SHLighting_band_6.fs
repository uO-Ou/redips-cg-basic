#version 430 core
out vec4 Color;

in Pipe{
	vec3 Normal;
	vec3 FragPos;
	vec2 TexCoord;
	vec3 SHLight;
}fsInput;

//material
uniform struct Material{
	uint flags; //0:no texture, 2:diffuseTexture,3:both
	vec3 diffuse;
	sampler2D diffuseTexture;
}material;

//lights
#define MAX_POINT_LIGHT_NUMBER 6
#define MAX_DIRECTIONAL_LIGHT_NUMBER 2
uniform struct PointLight{
    vec3 position;
    vec3 intensity;
    vec3 attenuation;
}pointLights [MAX_POINT_LIGHT_NUMBER];

uniform struct DirectionalLight{
	vec3 direction;
	vec3 intensity;
}directionaLights [MAX_DIRECTIONAL_LIGHT_NUMBER];

uniform int pointLightNumber, directionaLightNumber;

//camera
uniform vec3 cameraPosition;

const float M = 64.0f;
const float SpecularStrength = 0.0;

void main(){

	vec3 N = normalize(fsInput.Normal);

	vec3 Diffuse_M = vec3(0,0,0), Specular_L = vec3(0,0,0), Diffuse_L = vec3(0,0,0);

	if((material.flags&2u)==0) Diffuse_M = material.diffuse;
	else Diffuse_M = (texture(material.diffuseTexture,fsInput.TexCoord).bgr * material.diffuse);

	//loop directional lights
    for(int i = 0;i < directionaLightNumber; ++i){

	    // Diffuse_L += ...

		// Specular_L
		vec3 H = normalize(cameraPosition - fsInput.FragPos - directionaLights[i].direction);
        Specular_L += SpecularStrength * pow(max(dot(H, N), 0.0), M) * directionaLights[i].intensity;
	}
	
    //loop point lights
    for(int i = 0;i < pointLightNumber; ++i){

		 //direction to light
		 vec3 v2l = pointLights[i].position - fsInput.FragPos;

		 //attenuation
         float distance = length(v2l);
         float attenuation = 1.0 / dot(pointLights[i].attenuation,vec3(1,distance,distance*distance));
		 v2l = v2l * (1.0/distance);

		 // Diffuse_L
		 Diffuse_L += pointLights[i].intensity * attenuation * max(dot(v2l , N),0);

         // Specular_L
         vec3 H = normalize(cameraPosition - fsInput.FragPos + pointLights[i].position-fsInput.FragPos);
         Specular_L += SpecularStrength * pow(max(dot(H, N),0.0),M) * pointLights[i].intensity*attenuation;
    }

	Color = vec4(Diffuse_M*(fsInput.SHLight.bgr+Diffuse_L)+Specular_L, 1.0);
	
}