/*
* Author : redips redips.xin@gmail.com
* Date : 2017.12.20
* Description : particles
*/
#pragma once
#include "model.h"
namespace redips{
	class Particles : public Model{
	public:
		Material material;
		Particles(float radius = 0.1f):radius(radius){
			material = Material("sphere");
		};
		Particles(const char* file, float radius = 0.1f) :radius(radius){
			load(file);
			material = Material("sphere");
		}
		~Particles(){};
		void addSphere(const float3& pos){
			spheres.push_back(pos);
			aabb_raw += pos;
		}
		const float* ptr()const { return &(spheres[0].x); }
		
		void getRawAABB(){
			aabb_raw.reset();
			for (int i = 0; i < spheres.size(); i++) aabb_raw += spheres[i];
		}
		
		const Material& getMaterial(int index) const{
			return material;
		}

		float3 texcoord(int particleId, const float3& pos) const{
			redips::float3 ray = (pos - (transform*float4(spheres[particleId], 1.0f)).vec3());
			float coord_y = acos(ray.unit().dot(redips::float3(0, -1, 0))) * PI_INV;

			ray.y = 0;
			if (ray.length2() < 1e-6) return redips::float3(0,coord_y,1);
			float coord_x = acos(ray.unit().dot(redips::float3(-1, 0, 0))) * PI_INV * 0.5f;
			if (ray.z < 0) coord_x = 1.0f - coord_x;
			return redips::float3(coord_x,coord_y,1);
		}

		void setMaterial(const Material& mtl) { material = mtl; };
		
		void buildTree(){

		}
		bool intersect(const Ray& ray, HitRecord& record){
			bool hitted = false;
			float tdist = FLT_MAX;
			for (int i = 0; i < spheres.size(); i++){
				if (ray.intersect((transform * float4(spheres[i], 1.0f)).vec3(), radius, tdist)){
					if (tdist < record.distance && tdist>record.offset){
						record.distance = tdist;
						record.hitIndex = i;
						hitted = true;
					}
				}
			}
			if (hitted) record.normal = (ray.ori + ray.dir * record.distance - (transform*float4(spheres[record.hitIndex], 1.0f)).vec3()).unit();
			return hitted;
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
			getRawAABB();
		}
		
	};
};


