/*
* Author : redips redips.xin@gmail.com
* Date : 2017.12.20
* Description : model base class
		a. currently support two kinds of Model : Triangles and Particles
		b. each Model-derived class should implement several methods:
			intersect(...) buildTree() getRawAABB() getMaterial(...) texcoord(...)
		c. transform-operation should update aabb-box;
*/
#pragma once
#include "kdtree.h"
#include "material.h"
#include "geometry.h"
#include <ctime>

namespace redips{
	class Model{
	public:
		Model(){ transform = Mat44f::eye(); };
		virtual ~Model(){};
	public:
		const Mat44f& Transform() const{ return transform; }
		void Transform(const Mat44f& transform){
			this->transform = transform;
		}
		//return bounding box after transformed
		BOX aabb_T() const{
			BOX ret;
			for (int i = 0; i < 8; i++){
				ret += (transform*float4(aabb_raw.lbb + float3::bits(i)*aabb_raw.dim(), 1.0f)).vec3();
			}
			return ret;
		}
		//return bounding box of raw obj-file
		const BOX& aabb_R() const{
			return aabb_raw;
		}
	public:
		//return a reference to geometry[index]'s material
		virtual const Material& getMaterial(int index) const = 0;
		//return texcoord of pos. incase a obj file doesnt contain texcoord, result.z will be 0
		virtual float3 texcoord(int geoId, const float3& pos) const = 0;
		//check if ray intersects with model
		virtual bool intersect(const Ray& ray, HitRecord& record) = 0;
		//build a kdtree to accelerate ray-model intersect operation
		virtual void buildTree() = 0;
		//calculate aabb of input file
		virtual void getRawAABB() = 0;
	protected:
		bool useTree;
		KDTree mtree;
		BOX aabb_raw;
		Mat44f transform;
	public:
		MODEL_TYPE type;
	};
};

