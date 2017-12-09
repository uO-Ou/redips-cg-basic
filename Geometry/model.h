/*
* Author : redips redips.xin@gmail.com
* Date : 2017.12.9
* Description : model base class
		a. currently support two kinds of Model : Triangles and Particles
		b. each Model-derived class should implement several methods:
			intersect(...) buildTree() updateAABB() getMaterial(...) diffuseColor(...)
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
		BOX aabb_T() const{
			BOX ret;
			for (int i = 0; i < 8; i++){
				ret += (transform*float4(aabb_raw.lbb + float3::bits(i)*aabb_raw.dim(), 1.0f)).vec3();
			}
			return ret;
		}
		const BOX& aabb_R() const{
			return aabb_raw;
		}
	public:
		virtual const Material& getMaterial(int index) const = 0;
		virtual float3 diffuseColor(int index, float3 pos) = 0;
		virtual bool intersect(const Ray& ray, HitRecord& record) = 0;
		virtual void buildTree() = 0;
		virtual void updateAABB() = 0;
		
	protected:
		Mat44f transform;
		BOX aabb_raw;
	public:
		MODEL_TYPE type;
		bool useTree;
		KDTree mtree;
	};
};

