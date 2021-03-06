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
			MAX_RAYTRACE_DEPTH = 3;
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
			
			if (camera.type == CAMERA_TYPE::_phc_){
				#pragma omp parallel for
				for (int x = 0; x < imgwidth; x++) {
					#pragma omp parallel for
					for (int y = 0; y < imgheight; y++){
						HitRecord records;
						trace(1, camera.getRay(x, y), records);
						if (records.color.x>1) records.color.x = 1;
						if (records.color.y>1) records.color.y = 1;
						if (records.color.z > 1) records.color.z = 1;
						imgbuf[(y*imgwidth + x) * bpp + 0] = records.color.z * 255;
						imgbuf[(y*imgwidth + x) * bpp + 1] = records.color.y * 255;
						imgbuf[(y*imgwidth + x) * bpp + 2] = records.color.x * 255;
					}
				}
			}
			else if (camera.type == CAMERA_TYPE::_mpc_){

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
			for (int i = 0; i < objects.size(); ++i){
				if (objects[i]->intersect(ray, records)){
					hitId = i;
				}
			}
			//if not,return background-color
			if (hitId < 0) {
				records.color = bgColor;
				return;
			}

			float3 hitPoint = ray.ori + ray.dir * records.distance;
			
			//b. check if ray inside object
			bool inside = false;
			if (ray.dir.dot(records.normal)>0){
				records.normal = records.normal * (-1);
				inside = true;
			}

			//get hitted material
			const Material &mtl = objects[hitId]->getMaterial(records.hitIndex);

			float3 surfaceColor(0.0f);
			float3 ambientColor = mtl.ambient;
			float3 diffuseColor = mtl.diffuse;
			float3 texcoord = objects[hitId]->texcoord(records.hitIndex, hitPoint);
			if (texcoord.z > 0){
				if (mtl.texture_kd) diffuseColor = mtl.tex_diffuse(texcoord.x, texcoord.y);
				if (mtl.texture_ka) ambientColor = mtl.tex_ambient(texcoord.x, texcoord.y);
			}

			if ((depth < MAX_RAYTRACE_DEPTH) && (mtl.specular.length()>0)){
				//sendout a new ray
				float facingRatio = -ray.dir.dot(records.normal);
				float fresnelEffect = MIX(pow(1-facingRatio,4),1,0.2);
				float3 reflectDir = ray.dir + records.normal * (-2 * ray.dir.dot(records.normal));
				HitRecord reflect; 
				trace(depth + 1, Ray(hitPoint, reflectDir), reflect);
				surfaceColor = reflect.color * fresnelEffect * diffuseColor;
			}
			else{
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


