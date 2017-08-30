#pragma once
#include "kdtree.h"
#include "material.h"
#include "geometry.h"
#include <ctime>

class Model{
public:
	  Mat44f transform;
	  MODEL_TYPE type;
	  Model(){
		   transform = Mat44f::eye();
	  };
public:
	  virtual const Material* getMaterial(int index) const = 0;
	  virtual float3 diffuseColor(int index,float3 pos) = 0;
	  virtual bool intersect(const Ray& ray, HitRecord& record) = 0;
	  virtual void buildTree() = 0;
	  bool useTree;
	  KDTree mtree;
};
