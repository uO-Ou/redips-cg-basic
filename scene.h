#pragma once
#include "camera/phc.h"
#include "camera/mpc.h"
#include "geos/particles.h"
#include "geos/triangles.h"
#include <opencv2/opencv.hpp>

class Scene {
public:
	Scene(){
		image = cv::Mat(576, 768, CV_8UC3);
		bgColor = float3(0.2f, 0.2f, 0.2f);
		MAX_RAYTRACE_DEPTH = 2;
	};
	~Scene(){};
	cv::Mat image;

	void addObject(Model* obj){
		objects.push_back(obj);
	}
	void addLight(Light light){
		lights.push_back(light);
	}
	void project(PHC phc){
		using namespace cv;
		image.create(phc.resolution.height(), phc.resolution.width(), CV_8UC3);

		float2 proja,projb,projc;
		for (int i = 0; i < objects.size(); i++){
			if (objects[i]->type == _triangle_){
				Triangles *triangles = (Triangles *)(objects[i]);
				int faceCnt = triangles->faceCnt;
				const float3* vertices = &((*(triangles->vertices))[0]);
				const int3* indexes = &((*(triangles->faces_v))[0]);
				for (int j = 0; j < faceCnt; j++){
					  int3 idx = indexes[j];
					  float4 pa = (triangles->Transform() * float4(vertices[idx[0]], 1.0f));
					  float4 pb = (triangles->Transform() * float4(vertices[idx[1]], 1.0f));
					  float4 pc = (triangles->Transform() * float4(vertices[idx[2]], 1.0f));
					  bool flaga = phc.project_old(pa, proja);
					  bool flagb = phc.project_old(pb, projb);
					  bool flagc = phc.project_old(pc, projc);
					  proja.y = phc.resolution.y - proja.y;
					  projb.y = phc.resolution.y - projb.y;
					  projc.y = phc.resolution.y - projc.y;

					  if ( flaga || flagb || flagc){
						  cv::line(image, Point(proja.x, proja.y), Point(projb.x, projb.y), Scalar(0, 255, 0));
						  cv::line(image, Point(projb.x, projb.y), Point(projc.x, projc.y), Scalar(0, 255, 0));
						  cv::line(image, Point(projc.x, projc.y), Point(proja.x, proja.y), Scalar(0, 255, 0));
					  }
				}
			}
		}
	}
	void raytracing(PHC phc){
		image.create(phc.resolution.height(), phc.resolution.width(), CV_8UC3);

		clock_t start = clock();
		for (int x = 0; x < image.cols; x++) for (int y = 0; y < image.rows; y++){
			float3 color = trace2(phc.getRay(x, image.rows-y-1), 1);
			if (color.x>1) color.x = 1;
			if (color.y>1) color.y = 1;
			if (color.z>1) color.z = 1;
			image.at<cv::Vec3b>(y,x) = cv::Vec3b((color.x * 255), color.y * 255, color.z * 255);
		}
		clock_t finish = clock();
		printf("[ray-tracer] : cost %lf ms\n",(double)(finish-start)/CLOCKS_PER_SEC*1000);
	}
	void raytracing(MPC mpc){
		image.create(mpc.mainphc->resolution.height(),mpc.mainphc->resolution.width(),CV_8UC3);

		clock_t start = clock();
		for (int x = 0; x < image.cols; x++) for (int y = 0; y < image.rows; y++){
			float3 color = trace2(mpc.getRay(x, y), 1);
			if (color.x>1) color.x = 1;
			if (color.y>1) color.y = 1;
			if (color.z>1) color.z = 1;
			image.at<cv::Vec3b>(y, x) = cv::Vec3b((color.x * 255), color.y * 255, color.z * 255);
		}
		clock_t finish = clock();
		printf("mpc raytracing cost %lf ms\n", (double)(finish - start) / CLOCKS_PER_SEC * 1000);
	}
	void updateSceneBox(){
		sceneBox.reset();
		for (int i = 0; i < objects.size(); i++){
			sceneBox += objects[i]->aabb();
		}
	}

public:
	float3 bgColor;
	BOX sceneBox;
	std::vector<Model*> objects;
	std::vector<Light> lights;
	int MAX_RAYTRACE_DEPTH;
	
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
		const Material *mtl = objects[hitId]->getMaterial(records.hitIndex);

		float3 hitPoint = ray.ori + ray.dir * records.distance;
		//b. check if ray inside object
		bool inside = false;
		if (ray.dir.dot(records.normal)>0){
			records.normal = records.normal * (-1);
			inside = true;
		}
		//
		float3 surfaceColor(0.0f);
		if ((depth < MAX_RAYTRACE_DEPTH) && (mtl->specular.length()>0)){

		}
		else{
			for (int i = 0; i < lights.size(); i++){
				float3 lightDir = lights[i].pos - hitPoint;
				Ray shadowRay(hitPoint + records.normal*1e-2, lightDir);
				float len2Light = lightDir.length();
				bool litted = true;
				{
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
					surfaceColor += mtl->diffuse*(MAX(0.0f, factor))*lights[i].color;
					//surfaceColor += objects[hitId]->diffuseColor(records.hitIndex,hitPoint)* (MAX(0.0f, factor))*lights[i].color;
				}
			}
		}
		return surfaceColor + mtl->ambient;
	}

	float3 trace2(const Ray &ray, int depth){
		HitRecord records;
		int hitId = -1;
		for (int i = 0; i < objects.size(); i++){
			if (objects[i]->intersect(ray, records)){
				hitId = i;
			}
		}
		if (hitId < 0) return bgColor;

		float3 hitPoint = ray.ori + ray.dir * records.distance;
		bool inside = false;
		if (ray.dir.dot(records.normal)>0){
			records.normal = records.normal * (-1);
			inside = true;
		}
		float3 surfaceColor(0.0f);
		const Material *mtl = objects[hitId]->getMaterial(records.hitIndex);
		{
			for (int i = 0; i < lights.size(); i++){
				float3 lightDir = lights[i].pos - hitPoint;
				Ray shadowRay(hitPoint + records.normal*1e-2, lightDir);
				float len2Light = lightDir.length();
				bool litted = true;
				{
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
					surfaceColor += lights[i].color * (MAX(0.0f, factor)) * mtl->diffuse * (10.0/records.distance);
				}
			}
		}
		return surfaceColor ;
	}
};

