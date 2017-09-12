#version 430 core

in vec3 FragPos;

uniform uint pres;

layout(std430,binding=0) buffer counter{
      uint cnts[];
};

void main(){
      vec3 pos = clamp(FragPos * (1u<<pres),0,((1u<<pres)-1u));
      atomicOr(cnts[(uint(pos.x)<<(pres+pres-5))+(uint(pos.y)<<(pres-5))+(uint(pos.z)>>5)],1u<<(uint(pos.z)&31u));
} 