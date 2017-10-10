#include <optix.h>
#include <optixu/optixu_math_namespace.h>
using namespace optix;

struct PerRayData_shadow{
	float distance;
};

//2张纹理
rtTextureSampler<float4, 2>  position_texture;
rtTextureSampler<float4, 2>  normal_texture;

//3个buffer
rtBuffer<float, 1> lights_buffer;
rtBuffer<unsigned int, 1> shadowMap_buffer;
rtBuffer<unsigned int, 1> visibilities_buffer;

rtDeclareVariable(uint, light_cnt, , );                                  //声明ray的type
rtDeclareVariable(uint, scr_width, , );                                 //width
rtDeclareVariable(uint, shadow_ray_type, , );                     //声明ray的type
rtDeclareVariable(float, scene_epsilon, , );                         //epsilon
rtDeclareVariable(uint2, launch_index, rtLaunchIndex, );    //launch_index  

rtDeclareVariable(rtObject, shadow_casters, , );    

rtDeclareVariable(PerRayData_shadow, prd_shadow, rtPayload, );   //声明ray的携带变量
rtDeclareVariable(float, t_hit, rtIntersectionDistance, );                     //声明距离变量为t_hit


RT_PROGRAM void any_hit_shadow(){
	prd_shadow.distance = t_hit;
	rtTerminateRay();
}

//发射光线，计算可见性
RT_PROGRAM void calculate_visibilities(){
	float3 ray_origin = make_float3(tex2D(position_texture, launch_index.x, launch_index.y));
	float3 pixel_normal = make_float3(tex2D(normal_texture, launch_index.x, launch_index.y));
	unsigned int offsets = launch_index.y*scr_width + launch_index.x;
	unsigned int uint_cpp = (light_cnt - 1) / 32 + 1;
	unsigned int littedCnt = 0;
	//if (launch_index.x != 222 || launch_index.y != 222) return;;
	if (!isnan(ray_origin.x)) {
		for (unsigned int lid = 0u; lid < light_cnt; lid++){
			//float3 light_pos = make_float3(tex1Dfetch(lights_texture, lid));
			float3 light_pos = *((float3 *)(&lights_buffer[lid*3]));
			float3 L = light_pos - ray_origin;
			if (dot(L, pixel_normal)>0.0f){
				PerRayData_shadow prd;
				prd.distance = -1;

				float dist = sqrtf(dot(L, L));
				float3 ray_direction = L / dist;
				optix::Ray ray = optix::make_Ray(ray_origin, ray_direction, shadow_ray_type, scene_epsilon, dist);
				rtTrace(shadow_casters, ray, prd);
				if (prd.distance==-1){
					littedCnt++;
					visibilities_buffer[offsets*uint_cpp + lid / 32u] |= 1u << (lid & 31u);
				}
			}
		}
	}
	shadowMap_buffer[offsets] = littedCnt;
}

RT_PROGRAM void exception(){

}
