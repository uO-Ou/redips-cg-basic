#pragma once
#include <GL/glew.h>
#include "../FImage.h"
class glTexture{
public:
	GLuint texId;
	glTexture(){ texId = 0; };
	~glTexture(){
		destroy();
	}
	void destroy(){
		if (texId) glDeleteTextures(1, &texId);
	}
	GLuint create2d(const FImage* fimage){
		if (texId) glDeleteTextures(1, &texId);
		glGenTextures(1,&texId);
		glBindTexture(GL_TEXTURE_2D,texId);
		switch (fimage->bpp){
		case 24: {
					 glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, fimage->width, fimage->height, 0, GL_RGB, GL_UNSIGNED_BYTE, fimage->ptr());
					 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
					 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
					 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
					 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					 break;
		}
		case 32: {
					 glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, fimage->width, fimage->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, fimage->ptr());
					 //glTexStorage2D(GL_TEXTURE_2D, 0, GL_RGBA8, fimage->width, fimage->height); 
					 //glTexSubImage2D(GL_TEXTURE_2D,0,0,0,fimage->width,fimage->height,GL_RGBA,GL_UNSIGNED_BYTE,fimage->ptr());
					 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
					 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
					 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
					 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					 break;
		}
		default:{
					printf("[glTexture] : unsupported image file\n");
					break;
		}
		}
		glBindTexture(GL_TEXTURE_2D, 0);
		return texId;
	}
	int create3d(int3 dim,GLuint internalFormat,GLuint format,GLuint type,const void* data){
		if (texId) glDeleteTextures(1, &texId);
		glGenTextures(1,&texId);
		glBindTexture(GL_TEXTURE_3D,texId);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
		glTexImage3D(GL_TEXTURE_3D,0,internalFormat,dim.x,dim.y,dim.z,0,format,type,data);
		glClearTexImage(texId,0,format,type,nullptr);
		glBindTexture(GL_TEXTURE_3D,0);
		return texId;
	}
};