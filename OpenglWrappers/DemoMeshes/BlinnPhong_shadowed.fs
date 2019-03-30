#version 430 core
out vec4 color;

in Pipe{
	vec3 FragPos;
	vec3 LightSpaceProjPos;
	vec2 TexCoord;
	vec3 Normal;
}fsInput;

//material
uniform struct Material{
	uint flags; //0:no texture,1:ambientTexture,2:diffuseTexture,3:both
	vec3 diffuse,ambient;
	sampler2D shadowTexture;
	sampler2D diffuseTexture;
	sampler2D ambientTexture;
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

const float ShadowBias = 1e-2;
const float M = 2.0f;
const float SpecularStrength = 0.2f;
const float gamma = 2.2f;

const int PCFD = 5;

float shadow_factor(){
	if((material.flags&8u)>0){

		ivec2 tdim = textureSize(material.shadowTexture, 0);
		float stepx = 1.0 / tdim.x;
		float stepy = 1.0 / tdim.y;

		float ticker = 0;
		for(int x=-PCFD/2;x<=PCFD/2;++x) for(int y=-PCFD/2;y<=PCFD/2;++y){
			float newx = fsInput.LightSpaceProjPos.x + stepx * x;
			float newy = fsInput.LightSpaceProjPos.y + stepy * y;
			float newd = texture(material.shadowTexture, vec2(newx,newy)).x;

			if(fsInput.LightSpaceProjPos.z<newd+ShadowBias){
				++ticker;
			}
		}
		return ticker / (PCFD*PCFD);

		//float nearz = texture(material.shadowTexture, vec2(fsInput.LightSpaceProjPos.x, fsInput.LightSpaceProjPos.y)).x;
		//if(fsInput.LightSpaceProjPos.z > nearz + ShadowBias){
		//	return 0;
		//} else return 1;
	} else return 1;
}

void main(){
	vec3 N = normalize(fsInput.Normal);
	
	// Ambient
	vec3 Ambient = vec3(0,0,0);
	if((material.flags&1u)==0) Ambient = material.ambient;
	else Ambient = (pow(texture(material.ambientTexture,fsInput.TexCoord).bgr,vec3(gamma)) * material.ambient);

	vec3 Diffuse = vec3(0,0,0),Specular = vec3(0,0,0);

	//deal directional light0, for shadow
	float shafact = shadow_factor();
	//float shafact = 1.0;
	if(shafact > 0){
		// Diffuse
		Diffuse += (max(dot(N, -directionaLights[0].direction), 0.0) * directionaLights[0].intensity * shafact);
		
		// Specular
		vec3 H = normalize(cameraPosition - fsInput.FragPos - directionaLights[0].direction);
        Specular += SpecularStrength * pow(max(dot(H, N), 0.0), M) * directionaLights[0].intensity * shafact;
	}

	//loop directional lights
    for(int i = 1;i < directionaLightNumber; ++i){
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
