#pragma once
#include <ogl/glMeshWrapper.h>
class LightMesh : public glMeshWrapper{
public:
	LightMesh(const Triangles* model, unsigned int setup_type = 1u) : glMeshWrapper(model, setup_type & 2u, setup_type & 1u){
		bindVaoAttribData(0, 1, -1);
	};
	LightMesh(const glMeshWrapper& another) : glMeshWrapper(another){
		bindVaoAttribData(0, 1, -1);
	}
	void draw(Shader& shader){
		for (int i = 0; i < meshCnt; i++){
			glBindVertexArray(vaos[i]);
			glDrawArrays(GL_TRIANGLES, 0, 3 * meshFaceCnt[i]);
		}
	}
	~LightMesh(){};
};
