#version 430 core
layout (location = 0) out vec4 gPosition;
layout (location = 1) out vec4 gNormal;
layout (location = 2) out vec4 gColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCord;

uniform int surfaceType;  //0:no texture,1:with texture
uniform vec3 diffuseColor;
uniform sampler2D diffuseTexture;

void main(){    
    // Store the fragment position vector in the first gbuffer texture
    gPosition = vec4(FragPos,1.0f);
    // Also store the per-fragment normals into the gbuffer
    gNormal = vec4(normalize(Normal),1.0f);
    //store color buffer
    if(surfaceType>0){
          gColor = vec4(texture(diffuseTexture,TexCord).rgb,1.0f);
    }
    else{
         gColor = vec4(diffuseColor,1.0f);
    }
}