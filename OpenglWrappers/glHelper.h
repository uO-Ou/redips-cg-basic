/*
* Author : redips redips.xin@gmail.com
* Date : 2017.12.20
* Description : opengl-related helpers
*/
#pragma once
#include "glTextureWrapper.h"
namespace redips{
	//use opengl to render a 2d-image
	class glImageRender{
	private:
		glTexture texture;
		GLuint readFBOId = 0;
		bool initialized = false;

		int2 texsize, winsize; int bpp;

	public:
		glImageRender(int2 texsize, int bits_per_pixel, int2 winsize = int2(0, 0)) :texsize(texsize), bpp(bits_per_pixel){
			if (this->winsize.x < 1) this->winsize = texsize; else this->winsize = winsize;
			switch (bpp){
				case  8: texture.create2d(texsize, GL_R8, GL_R, GL_UNSIGNED_BYTE, nullptr); break;
				case 24: texture.create2d(texsize, GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE, nullptr); break;
				case 32: texture.create2d(texsize, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, nullptr); break;
				default: {printf("[glImageRender] : error! doesn't support bpp is %d\n", bpp); return; };
			}
			glGenFramebuffers(1, &readFBOId);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, readFBOId);
			glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
			initialized = true;
		}
		~glImageRender(){ 
			if (readFBOId) glDeleteFramebuffers(1, &readFBOId);
		}
		void render(BYTE* imagedata){
			if (!initialized) return;
			texture.update(imagedata);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, readFBOId);
			glBlitFramebuffer(0, 0, texsize.x, texsize.y, 0, 0, winsize.x, winsize.y, GL_COLOR_BUFFER_BIT, GL_LINEAR);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		}
	};

	//save current frame to a picture
	class glScreenCapture{
	private:
		redips::int2 winsize;
		BYTE* imgbuf = nullptr;
		float* depbuf = nullptr;
		glScreenCapture(redips::int2 winsize):winsize(winsize){
			imgbuf = new BYTE[winsize.x*winsize.y*4];
			depbuf = new float[winsize.x*winsize.y];
		};
		~glScreenCapture(){
			if (imgbuf) delete imgbuf;
			if (depbuf) delete depbuf;
		};
		static glScreenCapture* instance;
		int frameId = 0;
		char strbuf[2048];
	public:
		void capture(const char* picname){
			GLint eReadType, eReadFormat;
			glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT, &eReadFormat);
			glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_TYPE, &eReadType);

			//glReadPixels(0, 0, winsize.x, winsize.y, GL_RGBA, GL_UNSIGNED_BYTE, imgbuf);
			glReadPixels(0, 0, winsize.x, winsize.y, eReadFormat, eReadType, imgbuf);
			CHECK_GL_ERROR("read pixels failed");
			sprintf_s(strbuf,"%s%d.bmp",picname,frameId++);
			if (redips::FImage::saveImage(imgbuf, winsize.x, winsize.y, 4, strbuf)){
				printf("[glScreenCapture] : save picture [%s] success !\n", strbuf);
			}
			else{
				printf("[glScreenCapture] : save picture [%s] failed !\n", strbuf);
			}
		}

		void capture_depth(const char* picname){
			glReadPixels(0, 0, winsize.x, winsize.y, GL_DEPTH_COMPONENT, GL_FLOAT, depbuf);
			CHECK_GL_ERROR("read depth failed");
			for (int i = 0; i < winsize.x*winsize.y; ++i) {
				imgbuf[i * 4 + 0] = depbuf[i] * 222;
				imgbuf[i * 4 + 2] = 222 - imgbuf[i * 4 + 0];
			}
			sprintf_s(strbuf, "%s_depth_%d.bmp", picname, frameId++);
			if (redips::FImage::saveImage(imgbuf, winsize.x, winsize.y, 4, strbuf)){
				printf("[glScreenCapture] : save picture [%s] success !\n", strbuf);
			}
			else{
				printf("[glScreenCapture] : save picture [%s] failed !\n", strbuf);
			}
		}

		static glScreenCapture* getInstance(int2 winsize){
			if (instance == nullptr) instance = new glScreenCapture(winsize);
			else{
				if (instance->winsize != winsize) delete instance;
				instance = new glScreenCapture(winsize);
			}
			return instance;
		}
	};
	glScreenCapture* glScreenCapture::instance = nullptr;
};
