#pragma once
#include "../../Common/vec.h"
#include "../glTextureWrapper.h"

namespace redips{
	class GBuffer{
	public:
		enum class _GL_GBUFFER_TEXTURE_TYPE_ { _position_, _normal_, _albedo_spec_,_texture_cnt_ };
	public:
		void bind4Writing(){
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, gBuffer);
			//glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}
		void bind4Reading(){
			glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
		}
		void SetReadBuffer(_GL_GBUFFER_TEXTURE_TYPE_ TextureType){
			glReadBuffer(GL_COLOR_ATTACHMENT0 + int(TextureType));
		}
		void copyZBuffer(){
			glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER,0);
			glBlitFramebuffer(0,0,dim2.x,dim2.y,0,0,dim2.x,dim2.y,GL_DEPTH_BUFFER_BIT,GL_NEAREST);
			glBindFramebuffer(GL_FRAMEBUFFER,0);
		}
		void render(_GL_GBUFFER_TEXTURE_TYPE_  textureType){
			glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glReadBuffer(GL_COLOR_ATTACHMENT0 + int(textureType));
			glBlitFramebuffer(0, 0, dim2.x, dim2.y, 0, 0, dim2.x, dim2.y, GL_COLOR_BUFFER_BIT, GL_LINEAR);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		}
		void unbind(){ 
			glBindFramebuffer(GL_FRAMEBUFFER, 0); 
			//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}
		const glTexture& getTexture(_GL_GBUFFER_TEXTURE_TYPE_ type){
			if (int(type) < 0 || type >= (_GL_GBUFFER_TEXTURE_TYPE_::_texture_cnt_)){
				glTexture nul;
				puts("[gBuffer] : invalidate parameter"); 
				return nul; 
			}
			return textures[int(type)];
		}
		GBuffer(int width,int height){
			if (initialized = initialize(int2(width,height))){
				puts("[gBuffer] : initialized !");
			};
		};
		~GBuffer(){
			for (int i = 0; i < int(_GL_GBUFFER_TEXTURE_TYPE_::_texture_cnt_); i++) textures[i].destroy();
			if (gBuffer) glDeleteFramebuffers(1,&gBuffer);
			if (rboDepth) glDeleteRenderbuffers(1,&rboDepth);
		};
	private:
		bool initialize(int2 dim){
			dim2 = dim;

			//gbuffer
			glGenFramebuffers(1, &gBuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

			//position texture
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + int(_GL_GBUFFER_TEXTURE_TYPE_::_position_), GL_TEXTURE_2D,
				textures[int(_GL_GBUFFER_TEXTURE_TYPE_::_position_)].create2d(dim2, GL_RGBA32F/*GL_RGB16F*/, GL_RGBA/*GL_RGB*/, GL_FLOAT, NULL), 0);
			//normal texture
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + int(_GL_GBUFFER_TEXTURE_TYPE_::_normal_), GL_TEXTURE_2D,
				textures[int(_GL_GBUFFER_TEXTURE_TYPE_::_normal_)].create2d(dim2, GL_RGBA32F/*GL_RGB16F*/, GL_RGBA/*GL_RGB*/, GL_FLOAT, NULL), 0);
			//color texture
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + int(_GL_GBUFFER_TEXTURE_TYPE_::_albedo_spec_), GL_TEXTURE_2D,
				textures[int(_GL_GBUFFER_TEXTURE_TYPE_::_albedo_spec_)].create2d(dim2, GL_RGBA32F, GL_RGBA, GL_FLOAT, NULL), 0);

			//tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
			GLuint attachments[int(_GL_GBUFFER_TEXTURE_TYPE_::_texture_cnt_)];
			for (int i = 0; i < int(_GL_GBUFFER_TEXTURE_TYPE_::_texture_cnt_); i++) attachments[i] = GL_COLOR_ATTACHMENT0 + i;
			glDrawBuffers(int(_GL_GBUFFER_TEXTURE_TYPE_::_texture_cnt_), attachments);

			//create and attach depth buffer (renderbuffer)
			glGenRenderbuffers(1, &rboDepth);
			glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, dim2.x, dim2.y);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

			//finally check if framebuffer is complete
			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
				std::cout << "[GBuffer] : Framebuffer not complete!" << std::endl;
				return false;
			}
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			return true;
		}
		bool initialized = false;
		int2 dim2;
		
		glTexture textures[int(_GL_GBUFFER_TEXTURE_TYPE_::_texture_cnt_)];
		GLuint gBuffer = 0;
		GLuint rboDepth = 0;
	};
}


