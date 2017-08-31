#pragma once
#include "kdtree.h"
#include "material.h"
#include "geometry.h"
#include <ctime>
using namespace redips;
//notice : transform-operation should update aabb-box;
class Model{
public:
	  MODEL_TYPE type;
	  Model(){
		   transform = Mat44f::eye();
	  };
	  const Mat44f& Transform() const{ return transform; }
	  BOX aabb() const{
		  BOX ret;
		  for (int i = 0; i < 8; i++){  
			  ret += (transform*float4(aabb_raw.lbb + float3::bits(i)*aabb_raw.dim(), 1.0f)).vec3();	
			  float3 tmp = aabb_raw.lbb + float3::bits(i)*aabb_raw.dim();
		  }
		  return ret;
	  }
public:
	  virtual const Material* getMaterial(int index) const = 0;
	  virtual float3 diffuseColor(int index,float3 pos) = 0;
	  virtual bool intersect(const Ray& ray, HitRecord& record) = 0;
	  virtual void buildTree() = 0;
	  virtual void updateAABB() = 0;
	  bool useTree;
	  KDTree mtree;
protected:
	  Mat44f transform;
	  BOX aabb_raw;
};
