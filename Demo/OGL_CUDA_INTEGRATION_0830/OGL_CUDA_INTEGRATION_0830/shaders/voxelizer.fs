#version 430 core

in vec3 FragPos;
in vec4 Inworld;

uniform uint precisio;
uniform uint pass;

layout(std430,binding=0) buffer counter{
      uint cnts[];
};

layout(std430,binding=1) buffer fragments{
      vec4 frags[];
};

//pass0 count
//pass1  save frags
void main(){
      vec3 pos = clamp(FragPos * (1u<<precisio),0,((1u<<precisio)-1u));
      uint offset = (uint(pos.x)<<(precisio+precisio-5))+(uint(pos.y)<<(precisio-5))+(uint(pos.z)>>5);
      if(pass==0) atomicAdd(cnts[offset],1u);
      else{
            uint idx = atomicAdd(cnts[offset],1u);
            frags[idx] = Inworld;
      }
} 