/*
* Author : redips redips.xin@gmail.com
* Date : 2017.12.9
* Description : glMeshWrapper with texture and light
*/
#pragma once
#include <openglWrappers/glMeshWrapper.h>
class TexLightMesh : public redips::glMeshWrapper{
public:
	TexLightMesh(const redips::Triangles* model, redips::ShaderSource shaderSource = redips::ShaderSource(), unsigned int option = 1u)
	    : glMeshWrapper(model, shaderSource, option){
		bindVaoAttribData(0, 1, 2);
	};
	TexLightMesh(const glMeshWrapper& another, redips::ShaderSource shaderSource = redips::ShaderSource()) : glMeshWrapper(another, shaderSource){
		bindVaoAttribData(0, 1, 2);
	}
	void draw(){
		m_shader->Use();
		for (int i = 0; i < meshCnt; i++){
			int stype = 0;
			if (meshMtls[i]->texture_ka != NULL && meshFaceTypes[i] == redips::GROUP_FACE_TYPE::_withtex_){
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, mtlTextureHandle[meshMtls[i]->texture_ka]);
				glUniform1i(glGetUniformLocation(m_shader->Program, shaderAmbientTextureUniformStr), shaderAmbientTextureLocation);
				stype |= 1u;
			}
			{
				redips::float3 color = meshMtls[i]->ambient;
				glUniform3f(glGetUniformLocation(m_shader->Program, shaderAmbientColorUniformStr), color.x, color.y, color.z);
			}
			if (meshMtls[i]->texture_kd != NULL && meshFaceTypes[i] == redips::GROUP_FACE_TYPE::_withtex_){
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, mtlTextureHandle[meshMtls[i]->texture_kd]);
				glUniform1i(glGetUniformLocation(m_shader->Program, shaderDiffuseTextureUniformStr), shaderDiffuseTextureLocation);
				stype |= 2u;
			}
			{
				redips::float3 color = meshMtls[i]->diffuse;
				glUniform3f(glGetUniformLocation(m_shader->Program, shaderDiffuseColorUniformStr), color.x, color.y, color.z);
			}
			glUniform1i(glGetUniformLocation(m_shader->Program, shaderSurfaceTypeUniformStr), stype);

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
