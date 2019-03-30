/*
* Author : redips redips.xin@gmail.com
* Date : 2017.12.25
* Description : blinn-phong with texture
*/
#pragma once
#include "../glMeshWrapper.h"

namespace redips{
	class BlinnPhongMesh : public redips::glMeshWrapper{
	public:
		BlinnPhongMesh(const redips::Triangles* model, GLuint shatex = 0, redips::ShaderSource shaderSource = redips::ShaderSource(), unsigned int option = WrapOption::_default_ | WrapOption::_genMipmap_)
			: glMeshWrapper(model, shaderSource, option){
			bindVaoAttribData(0, 1, 2);
			shadow_texture = shatex;
			if (shaderSource.sourceType == redips::ShaderSource::SourceType::_null_){
				char strbuf[512];
				sprintf_s(strbuf, "%s/OpenglWrappers/DemoMeshes/BlinnPhong%s", _REDIPS_ROOT_PATH_, shadow_texture ? "_shadowed" : "");
				useShader(strbuf);
			}
		};

		BlinnPhongMesh(const glMeshWrapper& another, GLuint shatex = 0, redips::ShaderSource shaderSource = redips::ShaderSource()) : glMeshWrapper(another, shaderSource){
			bindVaoAttribData(0, 1, 2);
			shadow_texture = shatex;
			if (shaderSource.sourceType == redips::ShaderSource::SourceType::_null_){
				char strbuf[512];
				sprintf_s(strbuf, "%s/OpenglWrappers/DemoMeshes/BlinnPhong%s", _REDIPS_ROOT_PATH_, shadow_texture ? "_shadowed" : "");
				useShader(strbuf);
			}
		}

		BlinnPhongMesh(const BlinnPhongMesh&) = delete;

		void draw(bool shadowed = false){
			if (!m_shader) { std::cerr << "shader error" << std::endl; return; };
			m_shader->Use();
			for (int i = 0; i < meshCnt; i++){
				if (meshFaceCnt[i] < 1) continue;
				unsigned int flags = 0;

				if (meshMtls[i]->texture_ka != NULL && meshFaceTypes[i] == redips::GROUP_FACE_TYPE::_withtex_){
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, mtlTextureHandle[meshMtls[i]->texture_ka]);
					glUniform1i(glGetUniformLocation(m_shader->Program, shaderAmbientTextureUniformStr), shaderAmbientTextureLocation);
					flags |= 1u;
				}
				{
					redips::float3 color = meshMtls[i]->ambient;
					glUniform3f(glGetUniformLocation(m_shader->Program, shaderAmbientColorUniformStr), color.x, color.y, color.z);
				}

				if (meshMtls[i]->texture_kd != NULL && meshFaceTypes[i] == redips::GROUP_FACE_TYPE::_withtex_){
					glActiveTexture(GL_TEXTURE1);
					glBindTexture(GL_TEXTURE_2D, mtlTextureHandle[meshMtls[i]->texture_kd]);
					glUniform1i(glGetUniformLocation(m_shader->Program, shaderDiffuseTextureUniformStr), shaderDiffuseTextureLocation);
					flags |= 2u;
				}
				{
					redips::float3 color = meshMtls[i]->diffuse;
					glUniform3f(glGetUniformLocation(m_shader->Program, shaderDiffuseColorUniformStr), color.x, color.y, color.z);
				}
				
				if (shadowed && shadow_texture != 0) {
					glActiveTexture(GL_TEXTURE3);
					glBindTexture(GL_TEXTURE_2D, shadow_texture);
					glUniform1i(glGetUniformLocation(m_shader->Program, shaderDepthTextureUniformStr), shaderDepthTextureLocation);
					flags |= 8u;
				}

				glUniform1ui(glGetUniformLocation(m_shader->Program, shaderSurfaceTypeUniformStr), flags);
				
				glBindVertexArray(vaos[i]);
				glDrawArrays(GL_TRIANGLES, 0, 3 * meshFaceCnt[i]);
			}
		}
		~BlinnPhongMesh(){};

		GLuint shadow_texture = 0;

		const GLchar* shaderSurfaceTypeUniformStr = "material.flags";
		const GLchar* shaderAmbientColorUniformStr = "material.ambient";
		const GLchar* shaderDiffuseColorUniformStr = "material.diffuse";
		const GLchar* shaderAmbientTextureUniformStr = "material.ambientTexture";
		const GLchar* shaderDiffuseTextureUniformStr = "material.diffuseTexture";
		const GLchar* shaderDepthTextureUniformStr = "material.shadowTexture";
		const GLuint  shaderAmbientTextureLocation = 0;
		const GLuint  shaderDiffuseTextureLocation = 1;
		const GLuint  shaderDepthTextureLocation = 3;
	};

	class BlinnPhongMeshTBN : public redips::glMeshWrapper {
	public:
		BlinnPhongMeshTBN(const redips::Triangles* model, GLuint shatex = 0, redips::ShaderSource shaderSource = redips::ShaderSource(), unsigned int option = WrapOption::_default_ | WrapOption::_genMipmap_ | WrapOption::_genTangent_)
			: glMeshWrapper(model, shaderSource, option) {
			bindVaoAttribData(0, 1, 2, 3);
			shadow_texture = shatex;
			if (shaderSource.sourceType == redips::ShaderSource::SourceType::_null_) {
				char strbuf[512];
				sprintf_s(strbuf, "%s/OpenglWrappers/DemoMeshes/BlinnPhong_tbn%s", _REDIPS_ROOT_PATH_, shadow_texture ? "_shadowed" : "");
				useShader(strbuf);
			}
		};
		
		BlinnPhongMeshTBN(const glMeshWrapper& another, GLuint shatex = 0, redips::ShaderSource shaderSource = redips::ShaderSource()) : glMeshWrapper(another, shaderSource) {
			bindVaoAttribData(0, 1, 2, 3);
			shadow_texture = shatex;
			if (shaderSource.sourceType == redips::ShaderSource::SourceType::_null_) {
				char strbuf[512];
				sprintf_s(strbuf, "%s/OpenglWrappers/DemoMeshes/BlinnPhong_tbn%s", _REDIPS_ROOT_PATH_, shadow_texture ? "_shadowed" : "");
				useShader(strbuf);
			}
		}

		BlinnPhongMeshTBN(const BlinnPhongMeshTBN&) = delete;

		~BlinnPhongMeshTBN() {};

		void draw(bool shadowed = false) {
			if (!m_shader) { std::cerr << "shader error" << std::endl; return; };
			m_shader->Use();
			for (int i = 0; i < meshCnt; i++) {
				if (meshFaceCnt[i] < 1) continue;
				unsigned int flags = 0;
				if (meshMtls[i]->texture_ka != NULL && meshFaceTypes[i] == redips::GROUP_FACE_TYPE::_withtex_) {
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, mtlTextureHandle[meshMtls[i]->texture_ka]);
					glUniform1i(glGetUniformLocation(m_shader->Program, shaderAmbientTextureUniformStr), shaderAmbientTextureLocation);
					flags |= 1u;
				}
				{
					redips::float3 color = meshMtls[i]->ambient;
					glUniform3f(glGetUniformLocation(m_shader->Program, shaderAmbientColorUniformStr), color.x, color.y, color.z);
				}
				if (meshMtls[i]->texture_kd != NULL && meshFaceTypes[i] == redips::GROUP_FACE_TYPE::_withtex_) {
					glActiveTexture(GL_TEXTURE1);
					glBindTexture(GL_TEXTURE_2D, mtlTextureHandle[meshMtls[i]->texture_kd]);
					glUniform1i(glGetUniformLocation(m_shader->Program, shaderDiffuseTextureUniformStr), shaderDiffuseTextureLocation);
					flags |= 2u;
				}
				{
					redips::float3 color = meshMtls[i]->diffuse;
					glUniform3f(glGetUniformLocation(m_shader->Program, shaderDiffuseColorUniformStr), color.x, color.y, color.z);
				}
				if (meshMtls[i]->texture_bump != NULL && meshFaceTypes[i] == redips::GROUP_FACE_TYPE::_withtex_) {
					glActiveTexture(GL_TEXTURE2);
					glBindTexture(GL_TEXTURE_2D, mtlTextureHandle[meshMtls[i]->texture_bump]);
					glUniform1i(glGetUniformLocation(m_shader->Program, shaderNormalTextureUniformStr), shaderNormalTextureLocation);
					flags |= 4u;
				}
				if (shadowed && shadow_texture != 0) {
					glActiveTexture(GL_TEXTURE3);
					glBindTexture(GL_TEXTURE_2D, shadow_texture);
					glUniform1i(glGetUniformLocation(m_shader->Program, shaderDepthTextureUniformStr), shaderDepthTextureLocation);
					flags |= 8u;
				}

				glUniform1ui(glGetUniformLocation(m_shader->Program, shaderSurfaceTypeUniformStr), flags);

				glBindVertexArray(vaos[i]);
				glDrawArrays(GL_TRIANGLES, 0, 3 * meshFaceCnt[i]);
			}
		}
	private:

		GLuint shadow_texture = 0;

		const GLchar* shaderSurfaceTypeUniformStr = "material.flags";
		const GLchar* shaderAmbientColorUniformStr = "material.ambient";
		const GLchar* shaderDiffuseColorUniformStr = "material.diffuse";
		const GLchar* shaderAmbientTextureUniformStr = "material.ambientTexture";
		const GLchar* shaderDiffuseTextureUniformStr = "material.diffuseTexture";
		const GLchar* shaderNormalTextureUniformStr = "material.normalTexture";
		const GLchar* shaderDepthTextureUniformStr = "material.shadowTexture";
		const GLuint  shaderAmbientTextureLocation = 0;
		const GLuint  shaderDiffuseTextureLocation = 1;
		const GLuint  shaderNormalTextureLocation = 2;
		const GLuint  shaderDepthTextureLocation = 3;
	};
};

