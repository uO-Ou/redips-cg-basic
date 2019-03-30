#pragma once
#include "../glTextureWrapper.h"
namespace redips{
	class FrameBufferObject{
		GLuint fboId = 0;
		GLuint m_width, m_height;
		redips::int4 m_ori_viewport;
		bool initialized = false;
	public:
		redips::glTexture depthMap, colorMap;
		FrameBufferObject(GLuint width, GLuint height) : m_width(width), m_height(height){
			glGenFramebuffers(1, &fboId);
		}
		~FrameBufferObject(){ glDeleteFramebuffers(1, &fboId); }

		void setup4ShadowMap(){
			assert(!initialized);
			depthMap.create2d(int2(m_width, m_height), GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
			glBindFramebuffer(GL_FRAMEBUFFER, fboId);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
			glDrawBuffer(GL_NONE);
			glReadBuffer(GL_NONE);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			initialized = true;
		}

		void setupColorAndDepthAttachment() {
			assert(!initialized);
			colorMap.create2d(int2(m_width, m_height), GL_RGBA8, GL_RGBA, GL_FLOAT, NULL);
			depthMap.create2d(int2(m_width, m_height), GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

			glBindFramebuffer(GL_FRAMEBUFFER, fboId);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorMap, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
				std::cout << "[FrameBufferObject] : Framebuffer not complete!" << std::endl;
				return ;
			}
			initialized = true;
		}

		void copyZBuffer() {
			glBindFramebuffer(GL_READ_FRAMEBUFFER, fboId);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glBlitFramebuffer(0, 0, m_width, m_height, 0, 0, m_width, m_height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		void bind4Writing() {
			if (!initialized) {
				std::cerr << "[FrameBufferObject] : error, didn't initialized" << std::endl;
				return;
			}
			glGetIntegerv(GL_VIEWPORT, &m_ori_viewport[0]);
			glViewport(0, 0, m_width, m_height);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboId);
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}
		void bind4Reading() {
			if (!initialized) {
				std::cerr << "[FrameBufferObject] : error, didn't initialized" << std::endl;
				return;
			}
			glBindFramebuffer(GL_READ_FRAMEBUFFER, fboId);
		}

		void bind(){
			if (!initialized){
				std::cerr << "[FrameBufferObject] : error, didn't initialized" << std::endl;
				return;
			}

			glGetIntegerv(GL_VIEWPORT, &m_ori_viewport[0]);

			glBindFramebuffer(GL_FRAMEBUFFER, fboId);
			glViewport(0, 0, m_width, m_height);
			glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		}

		void unbind(){
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(m_ori_viewport.x, m_ori_viewport.y, m_ori_viewport.z, m_ori_viewport.w);
		}
	};
};