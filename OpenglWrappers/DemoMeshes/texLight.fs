#version 430 core
out vec4 color;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 cameraPos;

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoord;

uniform int surfaceType;  //0:no texture,1:ambientTexture,2:diffuseTexture,3:two textures
uniform vec3 diffuseColor;
uniform vec3 ambientColor;
uniform sampler2D diffuseTexture;
uniform sampler2D ambientTexture;

void main(){
    // Diffuse 
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    // Specular
    float specularStrength = 0.5f;
    vec3 viewDir = normalize(cameraPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16);
    vec3 specular = specularStrength * spec * lightColor;  

	float distance = length(lightPos - FragPos);
	float attenuation = 1.0 / (1.0 + 0.0004 * distance + 0.0001 * distance * distance);
    //diffuse *= attenuation;
    //specular *= attenuation; 

    vec3 ambient;
    if((surfaceType&1u)>0) ambient = (texture(ambientTexture,TexCoord).bgr * ambientColor);
    else ambient = ambientColor;

    if((surfaceType&2u)>0) diffuse *=  (texture(diffuseTexture,TexCoord).bgr * diffuseColor);
    else diffuse *= diffuseColor;

	color = vec4(ambient+diffuse+specular,1.0f);
} 