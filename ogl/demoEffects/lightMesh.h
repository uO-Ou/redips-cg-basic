#pragma once
#include <ogl/glMeshWrapper.h>
class LightMesh : public glMeshWrapper{
public:
	LightMesh(const Triangles* model, bool setup_type = true) : glMeshWrapper(model, setup_type){
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
