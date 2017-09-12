#pragma once
#include <ogl/glMeshWrapper.h>

class ModelBoxMesh : public redips::glMeshWrapper{
public:
	ModelBoxMesh(redips::Triangles* model, bool setup_type = true) :glMeshWrapper(model, setup_type){
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

