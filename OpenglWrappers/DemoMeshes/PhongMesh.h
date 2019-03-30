/*
* Author : redips redips.xin@gmail.com
* Date : 2017.12.25
* Description : phong without texture
*/
#pragma once
#include "../glMeshWrapper.h"

namespace redips{
	class PhongMesh : public redips::glMeshWrapper{
	public:
		PhongMesh(const redips::Triangles* model, redips::ShaderSource shaderSource = redips::ShaderSource()) : glMeshWrapper(model, shaderSource){
			bindVaoAttribData(0, 1, -1);
			if (shaderSource.sourceType == redips::ShaderSource::SourceType::_null_){
				char strbuf[512];
				sprintf_s(strbuf, "%s/OpenglWrappers/DemoMeshes/Phong", _REDIPS_ROOT_PATH_);
				useShader(strbuf);
			}
		};
		PhongMesh(const glMeshWrapper& another, redips::ShaderSource shaderSource = redips::ShaderSource()) : glMeshWrapper(another, shaderSource){
			bindVaoAttribData(0, 1, -1);
			if (shaderSource.sourceType == redips::ShaderSource::SourceType::_null_){
				char strbuf[512];
				sprintf_s(strbuf, "%s/OpenglWrappers/DemoMeshes/Phong", _REDIPS_ROOT_PATH_);
				useShader(strbuf);
			}
		}
		PhongMesh(const PhongMesh&) = delete;
		~PhongMesh(){};
	};
};

