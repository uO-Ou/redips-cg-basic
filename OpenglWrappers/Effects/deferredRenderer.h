#pragma once
#include "GBuffer.h"

class DeferredRenderer : public redips::GBuffer{
public:
	DeferredRenderer(int width,int height) : GBuffer(width,height){
		char strbuf[512];
		sprintf_s(strbuf, "%s/OpenglWrappers/Effects/DeferredRenderer", _REDIPS_ROOT_PATH_);
		//sprintf_s(strbuf, "%s/OpenglWrappers/Effects/ssao", _REDIPS_ROOT_PATH_);
		useShader(redips::ShaderSource(strbuf));
	}
	~DeferredRenderer(){};
	void drawQuard(){
		deferredShader.Use();
		bindTexture(0, _GL_GBUFFER_TEXTURE_TYPE_::_normal_,"normalTexture");
		bindTexture(1, _GL_GBUFFER_TEXTURE_TYPE_::_position_,"positionTexture");
		bindTexture(2, _GL_GBUFFER_TEXTURE_TYPE_::_albedo_spec_,"materialTexture");
		redips::GBuffer::drawQuard();
	}
};