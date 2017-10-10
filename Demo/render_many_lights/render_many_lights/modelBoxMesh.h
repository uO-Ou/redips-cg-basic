#pragma once
#include <ogl/glMeshWrapper.h>

class ModelBoxMesh : public redips::glMeshWrapper{
public:
	ModelBoxMesh(redips::Triangles* model, unsigned int setup_type = 1u) :glMeshWrapper(model, setup_type&2u,setup_type&1u){
		bindVaoAttribData(0, 1, -1);
	};
	ModelBoxMesh(const redips::glMeshWrapper& another) :glMeshWrapper(another){
		bindVaoAttribData(0, 1, -1);
	}
	void draw(redips::Shader& shader){
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glLineWidth(2);
		drawAllMeshes();
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	~ModelBoxMesh(){
	
	};
};

