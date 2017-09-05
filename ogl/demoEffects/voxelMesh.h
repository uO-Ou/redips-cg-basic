#pragma once
#include <ogl/glMeshWrapper.h>
class VoxelMesh : public glMeshWrapper{
public:
	VoxelMesh(const Triangles* model, bool setup_type = true) : glMeshWrapper(model, setup_type){
		bindVaoAttribData(0,-1,-1);
	};
	VoxelMesh(const glMeshWrapper& another) : glMeshWrapper(another){
		bindVaoAttribData(0, -1, -1);
	}
	void draw(Shader& shader){
		for (int i = 0; i < meshCnt; i++){
			glBindVertexArray(vaos[i]);
			glDrawArrays(GL_TRIANGLES, 0, 3 * meshFaceCnt[i]);
		}
	}
	void draw(){
		for (int i = 0; i < meshCnt; i++){
			glBindVertexArray(vaos[i]);
			glDrawArrays(GL_TRIANGLES, 0, 3 * meshFaceCnt[i]);
		}
	}
	~VoxelMesh(){};
};

