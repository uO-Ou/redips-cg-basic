#version 430 core

in vec3 FragPos;

uniform uint res2;

layout(std430,binding=0) buffer counter{
      uint cnts[];
};

void main(){
      vec3 pos = clamp(FragPos * (1u<<res2),0,((1u<<res2)-1u));
      atomicOr(cnts[(uint(pos.x)<<(res2+res2-5))+(uint(pos.y)<<(res2-5))+(uint(pos.z)>>5)],1u<<(uint(pos.z)&31u));
} 