#pragma once
/*
* Author : redips redips.xin@gmail.com
* Date : 2019.03.25
* Description : unlit mesh
*/
#include "../glMeshWrapper.h"

namespace redips{
	class UnlitMesh : public glMeshWrapper{
	public:
		UnlitMesh(const redips::Triangles* model, redips::ShaderSource shaderSource = redips::ShaderSource(), unsigned int option = WrapOption::_default_)
			: glMeshWrapper(model, shaderSource, option) {
			bindVaoAttribData(0, 1, 2, 3);
			if (shaderSource.sourceType == redips::ShaderSource::SourceType::_null_) {
				char strbuf[512];
				sprintf_s(strbuf, "%s/OpenglWrappers/DemoMeshes/UnlitMesh", _REDIPS_ROOT_PATH_);
				useShader(strbuf);
			}
		};
		UnlitMesh(const glMeshWrapper& another, redips::ShaderSource shaderSource = redips::ShaderSource()) : glMeshWrapper(another, shaderSource) {
			bindVaoAttribData(0, 1, 2);
			if (shaderSource.sourceType == redips::ShaderSource::SourceType::_null_) {
				char strbuf[512];
				sprintf_s(strbuf, "%s/OpenglWrappers/DemoMeshes/UnlitMesh", _REDIPS_ROOT_PATH_);
				useShader(strbuf);
			}
		}
		UnlitMesh(const UnlitMesh&) = delete;

		void draw() {
			if (!m_shader) { std::cerr << "shader error" << std::endl; return; };
			m_shader->Use();
			for (int i = 0; i < meshCnt; i++) {
				if (meshFaceCnt[i] < 1) continue;
				glBindVertexArray(vaos[i]);
				glDrawArrays(GL_TRIANGLES, 0, 3 * meshFaceCnt[i]);
			}
		}
	};
}