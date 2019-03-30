#pragma once
#include "../glMeshWrapper.h"

namespace redips{
	class GBufferMesh : public redips::glMeshWrapper{
	public:
		GBufferMesh(const redips::Triangles* model, redips::ShaderSource shaderSource = redips::ShaderSource())
			: glMeshWrapper(model, shaderSource){
			bindVaoAttribData(0, 1, 2);
			if (shaderSource.sourceType == redips::ShaderSource::SourceType::_null_){
				char strbuf[512];
				sprintf_s(strbuf, "%s/OpenglWrappers/DemoMeshes/gbuffers", _REDIPS_ROOT_PATH_);
				useShader(strbuf);
			}
		};
		
		GBufferMesh(const glMeshWrapper& another, redips::ShaderSource shaderSource = redips::ShaderSource()) : glMeshWrapper(another, shaderSource){
			bindVaoAttribData(0, 1, 2);
			if (shaderSource.sourceType == redips::ShaderSource::SourceType::_null_){
				char strbuf[512];
				sprintf_s(strbuf, "%s/OpenglWrappers/DemoMeshes/gbuffers", _REDIPS_ROOT_PATH_);
				useShader(strbuf);
			}
		}
		
		GBufferMesh(const GBufferMesh&) = delete;
		
		void draw(){
			if (!m_shader) { std::cerr << "shader error" << std::endl; return; };
			m_shader->Use();
			for (int i = 0; i < meshCnt; i++){
				int stype = 0;
				if (meshMtls[i]->texture_kd != NULL && meshFaceTypes[i] == GROUP_FACE_TYPE::_withtex_){
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, mtlTextureHandle[meshMtls[i]->texture_kd]);
					glUniform1i(glGetUniformLocation(m_shader->Program, "diffuseTexture"), 0);
					stype = 1;
				}
				{
					float3 color = meshMtls[i]->diffuse;
					glUniform3f(glGetUniformLocation(m_shader->Program, "diffuseColor"), color.x, color.y, color.z);
				}
				glUniform1i(glGetUniformLocation(m_shader->Program, "surfaceType"), stype);

				glBindVertexArray(vaos[i]);
				glDrawArrays(GL_TRIANGLES, 0, 3 * meshFaceCnt[i]);
			}
		}
		~GBufferMesh(){};
	};
};


