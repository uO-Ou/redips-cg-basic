/*
* Author : redips redips.xin@gmail.com
* Date : 2017.12.19
* Description : a ray tracer
*/
#pragma once
#include "../Cameras/phc.h"
#include "../Geometry/particles.h"
#include "../Geometry/triangles.h"
#include "fImage.h"

namespace redips{
	class RayTracer {
	public:
		RayTracer(int width, int height) :imgwidth(width), imgheight(height){
			imgbuf = new BYTE[width*height*imgbpp/8];
			bgColor = float3(0.2f, 0.2f, 0.2f);
			MAX_RAYTRACE_DEPTH = 1;
		};
		~RayTracer(){
			if (imgbuf) delete imgbuf;
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
			int bpp = (imgbpp >> 3);
			clock_t start = clock();
			for (int x = 0; x < imgwidth; x++) for (int y = 0; y < imgheight; y++){
				float3 color = trace(camera.getRay(x, y), 1);
				if (color.x>1) color.x = 1;
				if (color.y>1) color.y = 1;
				if (color.z>1) color.z = 1;
				imgbuf[(y*imgwidth + x) * bpp + 0] = color.z * 255;
				imgbuf[(y*imgwidth + x) * bpp + 1] = color.y * 255;
				imgbuf[(y*imgwidth + x) * bpp + 2] = color.x * 255;
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
		int imgwidth, imgheight, imgbpp = 24;
	private:
		float3 trace(const Ray &ray, int depth){
			//a.check if intersect with scene
			HitRecord records;
			int hitId = -1;
			for (int i = 0; i < objects.size(); i++){
				if (objects[i]->intersect(ray, records)){
					hitId = i;
				}
			}
			//if not,return background-color
			if (hitId < 0) return bgColor;

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
					float3 lightDir = lights[i].pos - hitPoint;
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
						surfaceColor += diffuseColor*(MAX(0.0f, factor))*lights[i].color;
					}
				}
			}
			return surfaceColor*0.9 + ambientColor.bgr()*0.2;
		}
	};
};


