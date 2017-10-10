#version 430 core
out vec4 color;

in vec2 texcoord;

//uniform sampler2D position_tex;
//uniform sampler2D normal_tex;
uniform sampler2D color_tex;

uniform uint width,height,lightCnt;

layout(std430,binding=0) buffer shadow_factor{
       uint factor[];
};

void main(){
       uint tx = uint(clamp(texcoord.x*width,0,width-1));
       uint ty = uint(clamp(texcoord.y*height,0,height-1));
       float ratio = factor[ty*width+tx]*1.0f/lightCnt; 
       color = vec4(texture(color_tex,vec2(texcoord)).rgb*(ratio),1.0);
} 