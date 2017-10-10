#pragma once
#include <ogl/glMeshWrapper.h>
class TexLightMesh : public glMeshWrapper{
public:
	TexLightMesh(const Triangles* model, unsigned int setup_type = 1u) : glMeshWrapper(model, setup_type & 2u, setup_type & 1u){
		bindVaoAttribData(0,1,2);
	};
	TexLightMesh(const glMeshWrapper& another) : glMeshWrapper(another){
		bindVaoAttribData(0, 1, 2);
	}
	void draw(Shader& shader){
		for (int i = 0; i < meshCnt; i++){
			int stype = 0;
			if (meshMtls[i]->texture_ka != NULL && meshFaceTypes[i] == _withtex_){
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, mtlTextureHandle[meshMtls[i]->texture_ka]);
				glUniform1i(glGetUniformLocation(shader.Program, shaderAmbientTextureUniformStr), shaderAmbientTextureLocation);
				stype |= 1u;
			}
			{
				float3 color = meshMtls[i]->ambient;
				glUniform3f(glGetUniformLocation(shader.Program, shaderAmbientColorUniformStr), color.x, color.y, color.z);
			}
			if (meshMtls[i]->texture_kd != NULL && meshFaceTypes[i] == _withtex_){
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, mtlTextureHandle[meshMtls[i]->texture_kd]);
				glUniform1i(glGetUniformLocation(shader.Program, shaderDiffuseTextureUniformStr), shaderDiffuseTextureLocation);
				stype |= 2u;
			}
			{
				float3 color = meshMtls[i]->diffuse;
				glUniform3f(glGetUniformLocation(shader.Program, shaderDiffuseColorUniformStr), color.x, color.y, color.z);
			}
			glUniform1i(glGetUniformLocation(shader.Program, shaderSurfaceTypeUniformStr), stype);

			glBindVertexArray(vaos[i]);
			glDrawArrays(GL_TRIANGLES, 0, 3 * meshFaceCnt[i]);
		}
	}
	~TexLightMesh(){};

	const GLchar* shaderSurfaceTypeUniformStr = "surfaceType";
	const GLchar* shaderAmbientColorUniformStr = "ambientColor";
	const GLchar* shaderDiffuseColorUniformStr = "diffuseColor";
	const GLchar* shaderAmbientTextureUniformStr = "ambientTexture";
	const GLchar* shaderDiffuseTextureUniformStr = "diffuseTexture";
	const GLuint shaderAmbientTextureLocation = 0;
	const GLuint shaderDiffuseTextureLocation = 1;
};
