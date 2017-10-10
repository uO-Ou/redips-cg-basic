#version 430 core
out vec4 color;

uniform vec3 lightPos;
uniform vec3 lightColor;

in vec3 Normal;
in vec3 FragPos;

void main(){
   // Diffuse 
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
   
	float distance = length(lightPos - FragPos);
	float attenuation = 1.0 / (1.0 + 0.004 * distance + 0.0001 * distance * distance);
    diffuse *= attenuation;

	color = vec4(diffuse,1.0f);
} 