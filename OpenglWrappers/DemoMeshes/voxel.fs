#version 430 core

in vec3 FragPos;
in vec4 Inworld;

uniform uint voxel_resolution;
uniform uint pass;

layout(std430,binding=0) buffer counter{
      uint cnts[];
};

//layout(std430,binding=1) buffer fragments{
//      vec4 frags[];
//};

//pass0  count
//pass1  save frags
void main(){
      vec3 pos = clamp(FragPos * (1u<<voxel_resolution),0,((1u<<voxel_resolution)-1u));
      uint offset = (uint(pos.x)<<(voxel_resolution+voxel_resolution))+(uint(pos.y)<<(voxel_resolution))+(uint(pos.z));
      if(pass==0) atomicAdd(cnts[offset],1u);
      else{
            //uint idx = atomicAdd(cnts[offset],1u);
            //frags[idx] = Inworld;
      }
} 