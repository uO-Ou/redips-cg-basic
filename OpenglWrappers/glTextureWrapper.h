/*
* Author : redips redips.xin@gmail.com
* Date : 2017.12.20
* Description : opengl texture wrapper
*/
#pragma once
#include <GL/glew.h>
#include "../Common/FImage.h"
namespace redips{
	class glTexture{
	public:
		int3 dim;
		GLint InternalFormat;
		GLenum Format,Type;
		GL_TEXTURE_TYPE texture_type;
	public:
		operator GLuint() const{	return texId;	}
		glTexture(){ texId = 0; };
		~glTexture(){
			destroy();
		}
		void destroy(){ 
			if (texId) glDeleteTextures(1, &texId);
		}

		void update(const BYTE* imgdata,GLuint level = 0){
			if (texture_type == GL_TEXTURE_TYPE::_2d_){
				glBindTexture(GL_TEXTURE_2D,texId);
				glTexSubImage2D(GL_TEXTURE_2D, level, 0, 0, dim.x, dim.y, Format, Type, imgdata);
			}
			else{
			
			}
		}

		GLuint create1d(GLsizei nTextureWidth, GLint nInternalFormat, GLenum nFormat, GLenum nType, const void *gData){
			if (texId) glDeleteTextures(1, &texId);
			glGenTextures(1, &texId);
			glBindTexture(GL_TEXTURE_1D, texId);
			glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);

			glTexImage1D(GL_TEXTURE_1D, 0, nInternalFormat, nTextureWidth, 0, nFormat, nType, gData);
			glBindTexture(GL_TEXTURE_1D, 0);

			this->texture_type = GL_TEXTURE_TYPE::_1d_;
			this->dim.x = nTextureWidth;
			this->InternalFormat = nInternalFormat;
			this->Format = nFormat;
			this->Type = nType;

			return texId;
		}
		GLuint create2d(int2 dim, GLint nInternalFormat, GLenum nFormat, GLenum nType, const void* gData){
			if (texId) glDeleteTextures(1, &texId);
			glGenTextures(1, &texId);
			//GL_TEXTURE_2D_MULTISAMPLE
			glBindTexture(GL_TEXTURE_2D, texId);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexImage2D(GL_TEXTURE_2D, 0, nInternalFormat, dim.x, dim.y, 0, nFormat, nType, gData);
			glBindTexture(GL_TEXTURE_2D, 0);

			this->texture_type = GL_TEXTURE_TYPE::_2d_;
			this->dim.x = dim.x, this->dim.y = dim.y;
			this->InternalFormat = nInternalFormat;
			this->Format = nFormat;
			this->Type = nType;

			return texId;
		}
		GLuint create2d_msaa(int2 dim,GLuint format,GLuint nSamples){
			if (texId) glDeleteTextures(1, &texId);
			glGenTextures(1, &texId);
			//GL_TEXTURE_2D_MULTISAMPLE
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, texId);
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, nSamples, format, dim.x, dim.y, GL_TRUE);
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

			this->texture_type = GL_TEXTURE_TYPE::_2d_;
			this->dim.x = dim.x, this->dim.y = dim.y;
	
			return texId;
		}
		GLuint create2d(const FImage* fimage,bool genMipmap){
			switch (fimage->bpp){
			        case 24: {
						 this->InternalFormat = GL_RGBA32F;
						 this->Format = GL_RGB;
						 this->Type = GL_UNSIGNED_BYTE;
						 break;
			        }
			        case 32: {
						 this->InternalFormat = GL_RGBA32F;
						 this->Format = GL_RGBA;
						 this->Type = GL_UNSIGNED_BYTE;
						 break;
			        }
					case 8:{
					     this->InternalFormat = GL_R32F;
						 this->Format = GL_R;
						 this->Type = GL_UNSIGNED_BYTE;
						 break;
					}
			        default:{
						printf("[glTexture] : unsupported image file,bpp is %d\n",fimage->bpp);
						return 0;
			       }
			}

			if (texId) glDeleteTextures(1, &texId);
			glGenTextures(1, &texId);

			glBindTexture(GL_TEXTURE_2D, texId);
			
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			if (genMipmap){
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 4);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			}
			else{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			}
			glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat, fimage->width, fimage->height, 0, Format, Type, fimage->ptr());
			if(genMipmap) glGenerateMipmap(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, 0);
			this->texture_type = GL_TEXTURE_TYPE::_2d_;
			this->dim.x = fimage->width, this->dim.y = fimage->height;

			return texId;
		}
		GLuint create3d(int3 dim, GLuint internalFormat, GLuint format, GLuint type, const void* data){
			if (texId) glDeleteTextures(1, &texId);
			glGenTextures(1, &texId);
			glBindTexture(GL_TEXTURE_3D, texId);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
			glTexImage3D(GL_TEXTURE_3D, 0, internalFormat, dim.x, dim.y, dim.z, 0, format, type, data);
			//glClearTexImage(texId, 0, format, type, nullptr);
			glBindTexture(GL_TEXTURE_3D, 0);

			this->dim = dim;
			this->Type = type;
			this->Format = format;
			this->texture_type = GL_TEXTURE_TYPE::_3d_;
			this->InternalFormat = internalFormat;

			return texId;
		}

		void save2disk(const char* file) {
			int4 flags;
			if (map(flags)){
				std::ofstream fout(file);
				fout << flags.y << " " << flags.x << " " << flags.z << std::endl;
				if (Type == GL_FLOAT){
					for (int y = 0; y < dim.y; y++) {
						for (int x = 0; x < dim.x; x++) {
							for (int i = 0; i < flags.z; i++)
								fout << ((float*)buffer)[y*dim.x*flags.z + x*flags.z + i] << " ";
						}
						fout << std::endl;
					}
				}
				else{
					puts("[glTexture] : dont support uchar");
				}
				fout.close();
				puts("[glTexture] : write texture to disk finish");
				unmap();
			}
			else{
				puts("[glTexture] : write texture to disk failed");
			}
		}
		
		//if a texture mapped twice or more,[map] operation only allocate one block of memory.
		void* map(int4& flags){
			if (buffer) {
				//delete buffer;
				//buffer = nullptr;
				flags.x = dim.x;
				flags.y = dim.y;
				switch (Format){
					case GL_RGBA: {  flags.z = 4; break; }
					case GL_RGB: {  flags.z = 3; break; }
					default: { flags.z = 0; return nullptr; }
				}
				if (Type == GL_FLOAT){
					flags.w = 0;
				}
				else if (Type == GL_UNSIGNED_BYTE){
					flags.w = 1;
				}
				buffer_visitor++;
				return buffer;
			}
			if (texture_type == GL_TEXTURE_TYPE::_2d_){
				int cpp = 0;
				switch (Format){
					case GL_RGBA: {  cpp = 4; break; }
					case GL_RGB: {  cpp = 3; break; }
					default: puts("[glTexture] : unsupported texture format, map failed"); return nullptr;
				}

				if (Type == GL_FLOAT){ 
					flags.w = 0;
					buffer = new float[cpp * dim.x*dim.y]; 
					buffer_visitor = 1;
				}
				else if (Type == GL_UNSIGNED_BYTE){ 
					flags.w = 1;
					buffer = new unsigned char[cpp * dim.x*dim.y]; 
					buffer_visitor = 1;
				}
				else { puts("[glTexture] : unsupported texture type, map failed"); return nullptr; };

				glBindTexture(GL_TEXTURE_2D, texId);
				glGetTexImage(GL_TEXTURE_2D, 0, Format, Type, buffer);
				glBindTexture(GL_TEXTURE_2D, 0);

				flags.x = dim.x;
				flags.y = dim.y;
				flags.z = cpp;
			}
			else{
			
			}
			return buffer;
		}
		void unmap(){
			if ((--buffer_visitor) <= 0){
				delete buffer;
				buffer = nullptr;
			}
		}

		void* tex2d(int x,int y) const{
			if (texture_type != GL_TEXTURE_TYPE::_2d_) return nullptr;
			glBindTexture(GL_TEXTURE_2D, texId);
			int cpp = 0;
			switch (Format){
			     case GL_RGBA: {  cpp = 4; break; }
			     case GL_RGB: {  cpp = 3; break; }
			     default: puts("[glTexture] : unsupported texture format, write to disk failed"); return nullptr;
			}
			void* buffer;
			if (Type == GL_FLOAT){ buffer = new float[cpp * dim.x*dim.y]; }
			else if (Type == GL_UNSIGNED_BYTE){ buffer = new unsigned char[cpp * dim.x*dim.y]; }
			else { puts("[glTexture] : unsupported texture type, write to disk failed"); return nullptr; };

			glGetTexImage(GL_TEXTURE_2D, 0, Format, Type, buffer);
			if (Type == GL_FLOAT){
				float* ret = new float[cpp];
				for (int i = 0; i < cpp; i++) ret[i] = ((float*)buffer)[y*dim.x*cpp + x*cpp + i];
				return ret;
			}
			else{
				return nullptr;
			}
		}
	private:
		GLuint texId;
		void* buffer = nullptr;
		int buffer_visitor = 0;
	};
};
