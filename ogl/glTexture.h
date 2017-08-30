#pragma once
#include <GL/glew.h>
class glTexture{
public:
	GLuint texId;
	glTexture(){ texId = 0; };
	~glTexture(){
		if (texId) glDeleteTextures(1,&texId);
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