/*
* Author : redips redips.xin@gmail.com
* Date : 2017.12.9
* Description : particles
*/
#pragma once
#include "model.h"
namespace redips{
	class Particles : public Model{
	public:
		Particles(){
			radius = 0.1f;
			material = Material("sphere");
		};
		Particles(const char* file){
			radius = 0.1f;
			load(file);
		}
		~Particles(){};
		void addSphere(float3 pos){
			spheres.push_back(pos);
			updateAABB();
		}
		const float* ptr()const { return &(spheres[0].x); }
		
		void updateAABB(){
			aabb_raw.reset();
			for (int i = 0; i < spheres.size(); i++) aabb_raw += spheres[i];
		}
		
		const Material& getMaterial(int index) const{
			return material;
		}
		void setMaterial(const Material& mtl) { material = mtl; };
		
		void buildTree(){

		}
		bool intersect(const Ray& ray, HitRecord& record){
			bool hitted = false;
			float tdist = FLT_MAX;
			for (int i = 0; i < spheres.size(); i++){
				if (ray.intersect((transform * float4(spheres[i], 1.0f)).vec3(), radius, tdist)){
					if (tdist < record.distance){
						record.distance = tdist;
						record.hitIndex = i;
						hitted = true;
					}
				}
			}
			if (hitted) record.normal = (ray.ori + ray.dir * record.distance - (transform*float4(spheres[record.hitIndex], 1.0f)).vec3()).unit();
			return hitted;
		}
		float3 diffuseColor(int index, float3 pos){
			return material.diffuse;
		}

	public:
		float radius;
		std::vector<float3> spheres;
	private:
		void load(const char* file){
			int scnt = 0;
			std::ifstream fin(file);
			fin >> scnt;
			spheres.resize(scnt);
			for (int i = 0; i < scnt; i++){ fin >> spheres[i].x >> spheres[i].y >> spheres[i].z; }
			fin.close();
			updateAABB();
		}
		Material material;
	};
};


