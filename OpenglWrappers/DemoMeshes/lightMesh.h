/*
* Author : redips redips.xin@gmail.com
* Date : 2017.12.9
* Description : glMeshWrapper with light
*/
#pragma once
#include <openglWrappers/glMeshWrapper.h>
class LightMesh : public redips::glMeshWrapper{
public:
	LightMesh(const redips::Triangles* model, redips::ShaderSource shadersource = redips::ShaderSource(), unsigned int option = 1u) : glMeshWrapper(model, shadersource,option){
		bindVaoAttribData(0, 1, -1);
	};
	LightMesh(const glMeshWrapper& another, redips::ShaderSource shadersource = redips::ShaderSource()) : glMeshWrapper(another,shadersource){
		bindVaoAttribData(0, 1, -1);
	}
	~LightMesh(){};
};
