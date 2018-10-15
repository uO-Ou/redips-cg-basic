#version 430 core
out vec4 color;

in Pipe{
   vec3 Normal;
   vec3 FragPos;
   vec2 TexCoord;
   vec4 FragPosInLightSpace;
}fsInput;

//shadow map
uniform sampler2D depthTexture;

//material
uniform struct Material{
	uint flags; //0:no texture,1:ambientTexture,2:diffuseTexture,3:both
	vec3 diffuse,ambient;
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

const float M = 16.0f;
const float SpecularStrength = 0.5;

const float bias = 0.0005;

void main(){
    //calculate shadow factor
    vec3 proj = (fsInput.FragPosInLightSpace.xyz / fsInput.FragPosInLightSpace.w) * 0.5 + 0.5;
    float shadow = 0.0; {
        vec2 texelSize = 1.0 / textureSize(depthTexture, 0);
        for(int x = -3; x <= 3; ++x) {
            for(int y = -3; y <= 3; ++y) {
                float pcfDepth = texture(depthTexture, proj.xy + vec2(x, y) * texelSize).r; 
                shadow += proj.z - bias > pcfDepth ? 1.0 : 0.0;        
            }    
        }
        shadow /= 49.0;
    }
	shadow = (1-shadow)*0.8+0.2;
    //begin
    vec3 N = normalize(fsInput.Normal);

	// Ambient
	vec3 Ambient = vec3(0,0,0);
	if((material.flags&1u)==0) Ambient = material.ambient;
	else Ambient = (texture(material.ambientTexture,fsInput.TexCoord).bgr * material.ambient);

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
	else Diffuse *= (texture(material.diffuseTexture,fsInput.TexCoord).bgr * material.diffuse);
	
	color = vec4((Ambient+Diffuse+Specular)*shadow,1.0f);

	color = vec4(1-shadow,1-shadow,1-shadow,1.0f);

}