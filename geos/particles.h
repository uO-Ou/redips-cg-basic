#pragma once
#include "model.h"
class Particles : public Model{
public:
	Particles(){
		radius = 0.1f;
	};
	~Particles(){};
	bool intersect(const Ray& ray, HitRecord& record){
		bool hitted = false;
		float tdist = FLT_MAX;
		for (int i = 0; i < spheres.size(); i++){
			if (ray.intersect((transform * float4(spheres[i],1.0f)).vec3(), radius, tdist)){
				if (tdist < record.distance){
					record.distance = tdist;
					record.hitIndex = i;
					hitted = true;
				}
			}
		}
		if(hitted) record.normal = (ray.ori + ray.dir * record.distance - (transform*float4(spheres[record.hitIndex],1.0f)).vec3()).unit();
		return hitted;
	}
	Material* getMaterial(int index){
		return new Material("sphere");
	}
	float3 diffuseColor(int index, float3 pos){
		return float3(0.0f,0.8f,0.0f);
	}
	void addSphere(float3 pos){
		spheres.push_back(pos);
	}
public:
	float radius;
	std::vector<float3> spheres;
};

