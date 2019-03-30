#version 430 core
out vec4 color;

in Pipe{
	vec3 FragPos;
	vec2 TexCoord;
	mat3 TBN;
}fsInput;

//material
uniform struct Material{
	uint flags; //0:no texture,1:ambientTexture,2:diffuseTexture,3:both
	vec3 diffuse,ambient;
	sampler2D diffuseTexture;
	sampler2D ambientTexture;
	sampler2D normalTexture;
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

const float M = 2.0f;
const float SpecularStrength = 0.2f;
const float gamma = 2.2f;

void main(){
	vec3 N;
	if((material.flags&4u)==0){
		N = normalize(fsInput.TBN[2]);
	}
	else{
		N = texture(material.normalTexture, fsInput.TexCoord).bgr;
		N = /*normalize*/(N * 2.0 - 1.0);
		N = normalize(fsInput.TBN * N);
	}

	// Ambient
	vec3 Ambient = vec3(0,0,0);
	if((material.flags&1u)==0) Ambient = material.ambient;
	else Ambient = (pow(texture(material.ambientTexture,fsInput.TexCoord).bgr,vec3(gamma)) * material.ambient);

	vec3 Diffuse = vec3(0,0,0),Specular = vec3(0,0,0);
	//loop directional lights
    for(int i = 0;i < directionaLightNumber; ++i){
		// Diffuse
		Diffuse += (max(dot(N, -directionaLights[i].direction), 0.0) * directionaLights[i].intensity);
		
		// Specular
		vec3 H = normalize(cameraPosition - fsInput.FragPos - directionaLights[i].direction);
        Specular += SpecularStrength * pow(max(dot(H, N), 0.0), M) * directionaLights[i].intensity;
	}
    //loop point lights
    for(int i = 0;i < pointLightNumber; ++i){
         vec3 L = pointLights[i].position - fsInput.FragPos;
         float distance = length(L);    L = L * (1.0/distance);
         float attenuation = 1.0 / dot(pointLights[i].attenuation,vec3(1,distance,distance*distance));

         //Diffuse
         Diffuse += max(dot(L,N),0.0)*pointLights[i].intensity*attenuation;

         // Specular
         vec3 H = normalize(cameraPosition - fsInput.FragPos + pointLights[i].position-fsInput.FragPos);
         Specular += SpecularStrength * pow(max(dot(H,N),0.0),M) * pointLights[i].intensity*attenuation;
    }

	if((material.flags&2u)==0) Diffuse *= material.diffuse;
	else Diffuse *= pow(texture(material.diffuseTexture,fsInput.TexCoord).bgr,vec3(gamma)) * material.diffuse;
	
	color = vec4(pow(Ambient+Diffuse,vec3(1.0/gamma))+Specular, 1.0f);
}
