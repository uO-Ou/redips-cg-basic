#pragma once
#include <ogl/glMeshWrapper.h>
#include <ogl/shader.h>

class VoxelMesh : public redips::glMeshWrapper{
public:
	VoxelMesh(const redips::Triangles* model,bool setup_type = true):glMeshWrapper(model,setup_type){
		bindVaoAttribData(0, -1, -1);
	};
	VoxelMesh(const redips::glMeshWrapper& another) :glMeshWrapper(another){
		bindVaoAttribData(0, -1, -1);
	}
	void draw(redips::Shader& shader){
		drawAllMeshes();
	}
	~VoxelMesh(){};
};

