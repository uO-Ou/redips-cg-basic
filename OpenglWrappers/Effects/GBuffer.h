#pragma once
#include "../../Common/vec.h"
#include "../glTextureWrapper.h"
#include "../glslShaderWrapper.h"

namespace redips{
	class GBuffer{
	public:
		enum class _GL_GBUFFER_TEXTURE_TYPE_ { _position_, _normal_, _albedo_spec_,_texture_cnt_ };
		GLuint quardVao = 0, quardVbo = 0;
		Shader deferredShader;
		void useShader(const redips::ShaderSource& source){
			if (source.sourceType == ShaderSource::SourceType::_exists_program_){
				deferredShader = Shader(source.value.program);
			}
			else if (source.sourceType == ShaderSource::SourceType::_from_file_){
				deferredShader = Shader((std::string(source.value.path) + _vertex_shader_file_suffix_).c_str(), (std::string(source.value.path) + _fragment_shader_file_suffix_).c_str());
			}
		}
		void bindTexture(int location, _GL_GBUFFER_TEXTURE_TYPE_ type, const char* samplerName){
			glActiveTexture(GL_TEXTURE0+location);
			glBindTexture(GL_TEXTURE_2D, getTexture(type));
			deferredShader.uniformInt1(samplerName,location);
		}
		virtual void drawQuard(){
			glBindVertexArray(quardVao);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			glBindVertexArray(0);

			//this->copyZBuffer();
		}
	public:
		GLuint Id() const { return gBuffer; }
		void bind4Writing(){
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, gBuffer);
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
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
			glClearColor(0.0, 0.0, 0.0, 1.0);
			//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}
		glTexture& getTexture(_GL_GBUFFER_TEXTURE_TYPE_ type){
			if (int(type) < 0 || type >= (_GL_GBUFFER_TEXTURE_TYPE_::_texture_cnt_)){
				glTexture nul;
				puts("[gBuffer] : invalidate parameter"); 
				return nul; 
			}
			return textures[int(type)];
		}
		GBuffer(int width,int height){
			if (initialized = initialize(int2(width,height))){
			    float data[] = { -1.0f, 1.0f, 0.0f, -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, -1.0f, 0.0f };
				glGenBuffers(1, &quardVbo);
				glBindBuffer(GL_ARRAY_BUFFER, quardVbo);
				glBufferData(GL_ARRAY_BUFFER, sizeof(data), &data, GL_STATIC_DRAW);
					
				glGenVertexArrays(1, &quardVao);
				glBindVertexArray(quardVao);
				glEnableVertexAttribArray(0);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
				glBindBuffer(GL_ARRAY_BUFFER, 0);
				puts("[gBuffer] : initialized !");
			};
		};
		virtual ~GBuffer(){
			for (int i = 0; i < int(_GL_GBUFFER_TEXTURE_TYPE_::_texture_cnt_); i++) textures[i].destroy();
			if (gBuffer) glDeleteFramebuffers(1,&gBuffer);
			if (rboDepth) glDeleteRenderbuffers(1,&rboDepth);
			glDeleteBuffers(1,&quardVbo);
			glDeleteVertexArrays(1,&quardVao);
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


