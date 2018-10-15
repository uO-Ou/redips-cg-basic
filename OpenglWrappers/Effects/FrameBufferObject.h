#pragma once
#include "../glTextureWrapper.h"
namespace redips{
	class FrameBufferObject{
		GLuint fboId = 0;
		GLuint m_width, m_height;
		bool initialized = false;
	public:
		redips::glTexture depthMap,colorMap;
		FrameBufferObject(GLuint width,GLuint height):m_width(width),m_height(height){
			glGenFramebuffers(1,&fboId);
		}
		~FrameBufferObject(){ glDeleteFramebuffers(1, &fboId); }

		void setup4ShadowMap(){
			depthMap.create2d(int2(m_width, m_height), GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
			glBindFramebuffer(GL_FRAMEBUFFER, fboId);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
			glDrawBuffer(GL_NONE);
			glReadBuffer(GL_NONE);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			initialized = true;
		}
		void bind(){
			if (!initialized){
				std::cerr << "[FrameBufferObject] : error, didn't initialized" << std::endl;
				return;
			}
			glBindFramebuffer(GL_FRAMEBUFFER, fboId);
			glViewport(0, 0, m_width, m_height);
			glClear(GL_DEPTH_BUFFER_BIT);
		}
		void unbind(){
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
	};
};