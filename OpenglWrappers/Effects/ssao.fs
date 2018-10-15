#version 430 core
out vec4 color;

in vec2 texcoord;

uniform sampler2D normalTexture;
uniform sampler2D positionTexture;
uniform sampler2D materialTexture;

//for ssao calculation
#define SSAO_SAMPLE_CNT 5
uniform mat4 projection_view;

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

const float M = 20.0f;
const float SpecularStrength = 0.3f;

float rand(vec2 seed){
    return fract(sin(dot(seed.xy, vec2(12.9898,78.233))) * 43758.5453);
}

void main(){
	vec3 N = texture(normalTexture,texcoord).xyz;
    if(length(N)>0.9){
        vec3 diffuse = texture(materialTexture,texcoord).bgr;
        vec3 FragPos = texture(positionTexture,texcoord).xyz;
        float ssao_factor = 1; {   //calculate ssao factor
            int viscnt = 0;
            vec2 seed = vec2(FragPos.x,FragPos.y);
            for(int i=0;i<SSAO_SAMPLE_CNT;++i){
                float A = rand(seed);
                float E = rand(vec2(A,seed.x));
                seed = vec2(A,E);

                vec3 dir = vec3(cos((E))*sin((A)), sin((E)), cos((E))*cos((A)));
                vec3 sampos = FragPos + dir;
                vec4 proj = projection_view*vec4(sampos,1);
                vec2 texc = vec2(proj.x/proj.w,proj.y/proj.w)*0.5+0.5;
                vec3 tpos = texture(positionTexture,texc).xyz;
                vec4 afoj = projection_view*vec4(tpos,1);
                if(proj.z/proj.w <= afoj.z/afoj.w) viscnt++;
            }
            ssao_factor = viscnt * 1.0 / SSAO_SAMPLE_CNT;
	    }

	    vec3 Diffuse = vec3(0,0,0),Specular = vec3(0,0,0);
	    //loop directional lights
        for(int i = 0;i < directionaLightNumber; ++i){
			// Diffuse
			Diffuse += (max(dot(N, -directionaLights[i].direction), 0.0) * directionaLights[i].intensity);

			// Specular
			vec3 H = normalize(cameraPosition - FragPos - directionaLights[i].direction);
			Specular += SpecularStrength * pow(max(dot(H, N), 0.0), M) * directionaLights[i].intensity;
		}

		//loop point lights
		for(int i = 0;i < pointLightNumber; ++i){
			vec3 L = pointLights[i].position -  FragPos;
			float distance = length(L);    L = L * (1.0/distance);
			float attenuation = 1.0 / dot(pointLights[i].attenuation,vec3(1,distance,distance*distance));

			//Diffuse
			Diffuse += max(dot(L,N),0.0)*pointLights[i].intensity*attenuation;

			// Specular
			vec3 H = normalize(cameraPosition - FragPos + pointLights[i].position-FragPos);
			Specular += SpecularStrength * pow(max(dot(H,N),0.0),M) * pointLights[i].intensity*attenuation;
		}

        color = vec4((Diffuse*diffuse + Specular)*ssao_factor,1.0);
		color = vec4((Diffuse*diffuse + Specular),1.0);
		//color = vec4(ssao_factor,ssao_factor,ssao_factor,1.0);
	}
	else color = vec4(0,0,0,1);
	//color = vec4(N,1.0);
}
