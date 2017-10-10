#version 430 core
out vec4 color;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 cameraPos;
uniform int useLight;

in vec3 Normal;
in vec3 FragPos;

void main(){
    if(useLight==0) color = vec4(0.0f,1.0f,0.0f,1.0f);
    else{
           // Diffuse 
          vec3 norm = normalize(Normal);
          vec3 lightDir = normalize(lightPos - FragPos);
          float diff = max(dot(norm, lightDir), 0.0);
          vec3 diffuse = diff * lightColor;
          // Specular
          float specularStrength = 0.5f;
          vec3 viewDir = normalize(cameraPos - FragPos);
          vec3 reflectDir = reflect(-lightDir, norm);  
          float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
          vec3 specular = specularStrength * spec * lightColor;  

	      float distance = length(lightPos - FragPos);
	      float attenuation = 1.0 / (1.0 + 0.004 * distance + 0.0001 * distance * distance);
          diffuse *= attenuation;
          specular *= attenuation; 

	      color = vec4(diffuse+specular,1.0f);
    }
} 