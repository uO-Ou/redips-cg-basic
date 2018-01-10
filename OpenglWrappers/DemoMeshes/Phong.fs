#version 430 core
out vec4 color;

uniform vec3 lightPos;
uniform vec3 cameraPos;
uniform vec3 lightColor;

in vec3 Normal;
in vec3 FragPos;

const float M = 16.0f;
const float SpecularStrength = 0.5f;
const float SC = 1.0f;
const float SL = 0.0004;
const float SQ = 0.0001;

void main(){
    // Attenuation
    float distance = length(lightPos - FragPos);
    float attenuation = 1.0 / (SC + SL * distance + SQ * distance * distance);

    // Diffuse
    vec3 N = normalize(Normal);
    vec3 L = normalize(lightPos - FragPos);
	  vec3 diffuse = max(dot(N, L), 0.0) * attenuation * lightColor;

	  //Specular
	  vec3 V = normalize(cameraPos - FragPos);
    vec3 R = reflect(-L, N);
    vec3 specular = SpecularStrength * pow(max(dot(V, R), 0.0), M) * attenuation * lightColor;

	  color = vec4(diffuse+specular,1.0f);
}
