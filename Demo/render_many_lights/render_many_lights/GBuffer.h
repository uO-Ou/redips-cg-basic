#pragma once
#include <vec.h>
#include <ogl/glTexture.h>

namespace redips{
	class GBuffer{
	public:
		enum _GL_GBUFFER_TEXTURE_TYPE_ { _position_, _normal_, _albedo_spec_,_texture_cnt_ };
	public:
		void bind4Writing(){
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, gBuffer);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}
		void bind4Reading(){
			glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
		}
		void SetReadBuffer(_GL_GBUFFER_TEXTURE_TYPE_ TextureType){
			glReadBuffer(GL_COLOR_ATTACHMENT0 + TextureType);
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
			glReadBuffer(GL_COLOR_ATTACHMENT0 + textureType);
			glBlitFramebuffer(0, 0, dim2.x, dim2.y, 0, 0, dim2.x, dim2.y, GL_COLOR_BUFFER_BIT, GL_LINEAR);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		}
		void unbind(){ 
			glBindFramebuffer(GL_FRAMEBUFFER, 0); 
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}
		const glTexture& getTexture(_GL_GBUFFER_TEXTURE_TYPE_ type){
			if (type < 0 || type >= _texture_cnt_){ 
				glTexture nul;
				puts("[gBuffer] : invalidate parameter"); 
				return nul; 
			}
			return textures[type];
		}
		GBuffer(int2 dim){
			if (initialized = initialize(dim)){
				puts("[gBuffer] : initialized !");
			};
		};
		~GBuffer(){
			for (int i = 0; i < _texture_cnt_; i++) textures[i].destroy();
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
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + _position_,GL_TEXTURE_2D, 
				                                  textures[_position_].create2d(dim2, GL_RGBA32F/*GL_RGB16F*/, GL_RGBA/*GL_RGB*/, GL_FLOAT, NULL), 0);
			//normal texture
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + _normal_, GL_TEXTURE_2D,
				                                  textures[_normal_].create2d(dim2, GL_RGBA32F/*GL_RGB16F*/, GL_RGBA/*GL_RGB*/, GL_FLOAT, NULL), 0);
			//color texture
			glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0+_albedo_spec_,GL_TEXTURE_2D,
				                                  textures[_albedo_spec_].create2d(dim2,GL_RGBA,GL_RGBA,GL_UNSIGNED_BYTE,NULL),0);

			//tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
			GLuint attachments[_texture_cnt_]; 
			for (int i = 0; i < _texture_cnt_; i++) attachments[i] = GL_COLOR_ATTACHMENT0 + i;
			glDrawBuffers(_texture_cnt_, attachments);

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
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			return true;
		}
		bool initialized = false;
		int2 dim2;
		
		glTexture textures[_texture_cnt_];
		GLuint gBuffer = 0;
		GLuint rboDepth = 0;
	};
}


