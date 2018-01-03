/*
* Author : redips redips.xin@gmail.com
* Date : 2017.12.19
* Description : a ray tracer
*/
#pragma once
#include "../Cameras/phc.h"
#include "../cameras/mpc.h"
#include "../Geometry/particles.h"
#include "../Geometry/triangles.h"
#include "fImage.h"

namespace redips{
	class RayTracer {
	public:
		RayTracer(){
			bgColor = float3(0.88f, 0.99f, 0.99f);
			MAX_RAYTRACE_DEPTH = 1;
		};
		~RayTracer(){
			delete imgbuf;
		};

		void addObject(Model* obj){ objects.push_back(obj); }
		void addLight(const Light& light){	lights.push_back(light);	}
		void updateLight(int pos, const Light& light){
			if (pos < 0 || pos >= lights.size()) return;
			lights[pos] = light;
		}
		void updateSceneBox(){
			sceneBox.reset();
			for (int i = 0; i < objects.size(); i++) sceneBox += objects[i]->aabb_T();
		}

		void render(const Camera& camera,const char* picname = ""){
			if (imgwidth != camera.resolution.x || imgheight != camera.resolution.y){
				delete imgbuf; imgbuf = nullptr;
				imgwidth = camera.resolution.x, imgheight = camera.resolution.y;
			}
			if (imgbuf == nullptr){
				imgbuf = new BYTE[imgwidth*imgheight*imgbpp / 8];
			}
			int bpp = (imgbpp >> 3);
			clock_t start = clock();
			HitRecord records;
			if (camera.type == CAMERA_TYPE::_phc_){
				for (int x = 0; x < imgwidth; x++) for (int y = 0; y < imgheight; y++){
					records.reset();
					trace(1, camera.getRay(x, y), records);
					if (records.color.x>1) records.color.x = 1;
					if (records.color.y>1) records.color.y = 1;
					if (records.color.z>1) records.color.z = 1;
					imgbuf[(y*imgwidth + x) * bpp + 0] = records.color.z * 255;
					imgbuf[(y*imgwidth + x) * bpp + 1] = records.color.y * 255;
					imgbuf[(y*imgwidth + x) * bpp + 2] = records.color.x * 255;
				}
			}
			else if (camera.type == CAMERA_TYPE::_mpc_){
				const MPC& mpc = dynamic_cast<const MPC&>(camera);
				for (int x = 0; x < imgwidth; x++) for (int y = 0; y < imgheight; y++){
					records.reset();
					//in transition region
					if (abs(x-imgwidth/2)<mpc.tpanel.x/2&&abs(y-imgheight/2)<mpc.tpanel.y/2){
						float3 spoint((x*1.0/mpc.resolution.x-0.5f)*mpc.canvaSize.x,(y*1.0/mpc.resolution.y-0.5f)*mpc.canvaSize.y,mpc.nearp);
						float3 epoint = mpc.layers[0][0].getEndsPoint(spoint);
						{//first layer
							auto tsp = mpc.c2w3()*spoint+mpc.pos();
							auto tep = mpc.c2w3()*epoint+mpc.pos();
							records.distance = (tep - tsp).length();
							trace(1, Ray(tsp,tep-tsp), records);
						}
						
						//middle layers
						if(records.hitIndex<0){
							spoint = epoint;
							int mpc_region_id = -1;
							float min_dist = FLT_MAX;
							for (int i = 0; i < 5; i++){
								//if (mpc.layers[1][i].contains(spoint)){
								//	mpc_region_id = i; break;
								//}
								auto tmp = mpc.layers[1][i].contains(spoint);
								if (tmp<min_dist){
									mpc_region_id = i;
									min_dist = tmp;
								}
							}
							_RUNTIME_ASSERT_(mpc_region_id>-1,"raytracing mpc-camera transition region ray shooter failed");
							for (int l = 1; l < 4; l++){
								epoint = mpc.layers[l][mpc_region_id].getEndsPoint(spoint);
								auto tsp = mpc.c2w3()*spoint+mpc.pos();
								auto tep = mpc.c2w3()*epoint+mpc.pos();
								records.distance = (tep - tsp).length();
								trace(1, Ray(tsp, tep - tsp), records);
								if (records.hitIndex >= 0) break;
								spoint = epoint;
							}
						}
						//final layer
						if (records.hitIndex<0){
							epoint = mpc.layers[4][0].getEndsPoint(spoint);
							auto tsp = mpc.c2w3()*spoint+mpc.pos();
							auto tep = mpc.c2w3()*epoint+mpc.pos();
							records.distance = (tep - tsp).length();
							trace(1, Ray(tsp, tep - tsp), records);
						}
					}
					else{
						trace(1, mpc.getRay(x, y), records);
					}
					if (records.color.x>1) records.color.x = 1;
					if (records.color.y>1) records.color.y = 1;
					if (records.color.z>1) records.color.z = 1;
					imgbuf[(y*imgwidth + x) * bpp + 0] = records.color.z * 255;
					imgbuf[(y*imgwidth + x) * bpp + 1] = records.color.y * 255;
					imgbuf[(y*imgwidth + x) * bpp + 2] = records.color.x * 255;
				}
			}
			clock_t finish = clock();
			printf("[ray-tracer] : cost %lf ms\n", (double)(finish - start) / CLOCKS_PER_SEC * 1000);

			if (strlen(picname)) 
				redips::FImage::saveImage(imgbuf, imgwidth, imgheight, bpp, picname);
		}

	public:
		float3 bgColor;
		BOX sceneBox;
		std::vector<Light> lights;
		std::vector<Model*> objects;
		int MAX_RAYTRACE_DEPTH;
		unsigned char* imgbuf = nullptr;
		int imgwidth=0, imgheight=0, imgbpp = 24;
	private:
		void trace(int depth, const Ray &ray, HitRecord& records){
			//a.check if intersect with scene
			int hitId = -1;
			for (int i = 0; i < objects.size(); i++){
				if (objects[i]->intersect(ray, records)){
					hitId = i;
				}
			}
			//if not,return background-color
			if (hitId < 0) {
				records.color = bgColor;
				return;
			}

			//get hitted material
			const Material &mtl = objects[hitId]->getMaterial(records.hitIndex);

			float3 hitPoint = ray.ori + ray.dir * records.distance;
			//b. check if ray inside object
			bool inside = false;
			if (ray.dir.dot(records.normal)>0){
				records.normal = records.normal * (-1);
				inside = true;
			}
			
			float3 surfaceColor(0.0f);
			float3 ambientColor = mtl.ambient;
			if ((depth < MAX_RAYTRACE_DEPTH) && (mtl.specular.length()>0)){
				//sendout a new ray
			}
			else{
				float3 diffuseColor = mtl.diffuse;
				float3 texcoord = objects[hitId]->texcoord(records.hitIndex, hitPoint);
				if (texcoord.z > 0){ 
					if (mtl.texture_kd) diffuseColor = mtl.tex_diffuse(texcoord.x, texcoord.y);
					if (mtl.texture_ka) ambientColor = mtl.tex_ambient(texcoord.x, texcoord.y);
				}
				
				for (int i = 0; i < lights.size(); i++){
					float3 lightDir = lights[i].position - hitPoint;
					Ray shadowRay(hitPoint + records.normal*1e-2, lightDir);
					float len2Light = lightDir.length();
					bool litted = true;
					{// check if there's something block the light
						HitRecord shadowRecord;
						for (int j = 0; j < objects.size(); j++){
							if (j == hitId) continue;
							if (objects[j]->intersect(shadowRay, shadowRecord)){
								if (shadowRecord.distance < len2Light){
									//printf("shadowed.len2light %f,record %f\n",len2Light,shadowRecord.distance);
									litted = false;
									break;
								}
							}
						}
					}
					if (litted){
						float factor = (shadowRay.dir.dot(records.normal));
						surfaceColor += diffuseColor*(MAX(0.0f, factor))*lights[i].intensity;
					}
				}
			}
			records.color = surfaceColor*0.9 + ambientColor.bgr()*0.2;
		}
	};
};


