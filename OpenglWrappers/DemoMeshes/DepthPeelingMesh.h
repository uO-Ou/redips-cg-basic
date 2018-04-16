/*
* Author : redips redips.xin@gmail.com
* Date : 2018.04.13 19:09
* Description : depth peeling @ Qingdao
*/
#pragma once
#include "../glMeshWrapper.h"
namespace redips{
	class DepthPeelingBlinnPhongMesh : public glMeshWrapper{
		bool isTexturesSetted = false;
	public:
		DepthPeelingBlinnPhongMesh(const redips::Triangles* model, redips::ShaderSource shaderSource = redips::ShaderSource()) : glMeshWrapper(model, shaderSource){
			bindVaoAttribData(0, 1, 2);
			if (shaderSource.sourceType == redips::ShaderSource::SourceType::_null_){
				char strbuf[512];
				sprintf_s(strbuf, "%s/OpenglWrappers/DemoMeshes/DepthPeeling", _REDIPS_ROOT_PATH_);
				useShader(strbuf);
			}
		};
		DepthPeelingBlinnPhongMesh(const glMeshWrapper& another, redips::ShaderSource shaderSource = redips::ShaderSource()) : glMeshWrapper(another, shaderSource){
			bindVaoAttribData(0, 1, 2);
			if (shaderSource.sourceType == redips::ShaderSource::SourceType::_null_){
				char strbuf[512];
				sprintf_s(strbuf, "%s/OpenglWrappers/DemoMeshes/DepthPeeling", _REDIPS_ROOT_PATH_);
				useShader(strbuf);
			}
		}
		void setTextures(redips::glTexture& deptex,redips::glTexture& coltex){
			m_shader->Use();
			
			//bind depth texture, for depth peeling
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, deptex);
			glUniform1i(glGetUniformLocation(m_shader->Program, shaderDepthMapUniformStr), shaderDepthMapLocation);
			//bind color texture, for color blending
			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_2D, coltex);
			glUniform1i(glGetUniformLocation(m_shader->Program, shaderColorMapUniformStr), shaderColorMapLocation);

			isTexturesSetted = true;
			glUniform1f(glGetUniformLocation(m_shader->Program, "texWid"), float(deptex.dim.x));
			glUniform1f(glGetUniformLocation(m_shader->Program, "texHet"), float(deptex.dim.y));
		}
		void draw(){
			if (!m_shader) { std::cerr << "[error] shader error" << std::endl; return; };
			if (!isTexturesSetted){ std::cerr << "[error] doesn't have a depth map" << std::endl; return; }
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

		const GLchar* shaderSurfaceTypeUniformStr = "surfaceType";
		const GLchar* shaderAmbientColorUniformStr = "ambientColor";
		const GLchar* shaderDiffuseColorUniformStr = "diffuseColor";
		const GLchar* shaderAmbientTextureUniformStr = "ambientTexture";
		const GLchar* shaderDiffuseTextureUniformStr = "diffuseTexture";
		const GLchar* shaderDepthMapUniformStr = "depthTexture";
		const GLchar* shaderColorMapUniformStr = "colorTexture";
		const GLuint shaderAmbientTextureLocation = 0;
		const GLuint shaderDiffuseTextureLocation = 1;
		const GLuint shaderDepthMapLocation = 2;
		const GLuint shaderColorMapLocation = 3;
	};
};
