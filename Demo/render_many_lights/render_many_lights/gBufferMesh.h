#pragma once
#include <ogl/glMeshWrapper.h>

class gBufferMesh : public redips::glMeshWrapper{
public:
	gBufferMesh(const redips::Triangles* model,unsigned int setup_type = 1u) : glMeshWrapper(model,setup_type&2u,setup_type&1u){
		bindVaoAttribData(0, 1, 2);
	};
	gBufferMesh(const redips::glMeshWrapper& another) :glMeshWrapper(another){
		bindVaoAttribData(0, 1, 2);
	}
	void draw(redips::Shader& shader){
		for (int i = 0; i < meshCnt; i++){
			int stype = 0;
			if (meshMtls[i]->texture_kd != NULL && meshFaceTypes[i] == _withtex_){
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, mtlTextureHandle[meshMtls[i]->texture_kd]);
				glUniform1i(glGetUniformLocation(shader.Program, "diffuseTexture"), 0);
				stype = 1;
			}
			{
				float3 color = meshMtls[i]->diffuse;
				glUniform3f(glGetUniformLocation(shader.Program, "diffuseColor"), color.x,color.y,color.z);
			}
			glUniform1i(glGetUniformLocation(shader.Program, "surfaceType"), stype);

			glBindVertexArray(vaos[i]);
			glDrawArrays(GL_TRIANGLES, 0, 3 * meshFaceCnt[i]);
		}
	}
	~gBufferMesh(){};
};

