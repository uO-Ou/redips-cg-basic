#version 430 core
out vec4 color;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 cameraPos;

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoord;

uniform float texWid,texHet;
uniform float blendCoef;

//0:no texture,1:ambientTexture,2:diffuseTexture,3:two textures
uniform int surfaceType;
uniform vec3 diffuseColor;
uniform vec3 ambientColor;

uniform sampler2D diffuseTexture;
uniform sampler2D ambientTexture;
uniform sampler2D depthTexture;
uniform sampler2D colorTexture;

const float M = 16.0f;
const float SpecularStrength = 0.5f;
const float SC = 1.0f;
const float SL = 0.000000004;
const float SQ = 0.0000000001;

const float Bias = 1e-4;

void main(){
	//Depth peeling
	vec2 deptc = vec2(gl_FragCoord.x/texWid,gl_FragCoord.y/texHet);
	if(gl_FragCoord.z<=texture(depthTexture,deptc).r+Bias)
		discard;

    // Attenuation
    float distance = length(lightPos - FragPos);
    float attenuation = 1.0 / (SC + SL * distance + SQ * distance * distance);

    // Diffuse
    vec3 N = normalize(Normal);
    vec3 L = normalize(lightPos - FragPos);
	  vec3 diffuse = max(dot(N, L), 0.0) * attenuation * lightColor;

    // Specular
    vec3 H = normalize(cameraPos - FragPos + lightPos - FragPos);
    vec3 specular = SpecularStrength * pow(max(dot(H, N), 0.0), M) * attenuation * lightColor;

    // Sum up
    vec3 ambient;
    if((surfaceType&1u)>0)
        ambient = (texture(ambientTexture,TexCoord).bgr * ambientColor);
    else ambient = ambientColor;

    if((surfaceType&2u)>0)
        diffuse *= (texture(diffuseTexture,TexCoord).bgr * diffuseColor);
    else diffuse *= diffuseColor;

	color = vec4((ambient+diffuse+specular)*(1.0f-blendCoef)+texture(colorTexture,deptc).rgb*blendCoef,1.0f);
}
