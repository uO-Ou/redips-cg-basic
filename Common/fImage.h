/*
* Author : redips redips.xin@gmail.com
* Date : 2017.12.9
* Description : encapsulated FreeImage
*/
#pragma once
#include "../Dependencies/FreeImage/FreeImage.h"
#include "vec.h"
namespace redips{
	class FImage{
	public:
		FImage(const char *file){
			dib = FImage::read(file);
			if (dib != NULL){
				width = FreeImage_GetWidth(dib);
				height = FreeImage_GetHeight(dib);
				bpp = FreeImage_GetLine(dib) / width * 8;
				imageType = FreeImage_GetImageType(dib);
			}
			else{
				width = height = bpp = 0;
				printf("[FreeImage] : load picture %s failed \n", file);
			}
		};
		~FImage(){
			if (dib) FreeImage_Unload(dib);
		};
		int width, height, bpp;
		Vec3<float> tex2d(float u, float v) const{
			int tu = u*width; int tv = v*height;
			BYTE *bits = FreeImage_GetScanLine(dib, tv);
			if (bpp == 24 || bpp == 32){
				int step = bpp / 8;
				return Vec3<float>(bits[tu * step + 2] / 255.0f, bits[tu * step + 1] / 255.0f, bits[tu * step + 0] / 255.0f);
			}
			else if (bpp == 8){
				float tfloat = bits[tu] / 255.0f;
				return Vec3<float>(tfloat);
			}
			else{
				printf("[FImage] : unsupported bpp %d\n", bpp);
				return Vec3<float>(0, 1, 0);
			}
		}
		BYTE* ptr() const{
			if (dib) return FreeImage_GetBits(dib);
			return NULL;
		}
	public:
		static bool saveImage(const BYTE* bytes, int width, int height, int byte_per_pixel, const char* path){
			FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;
			fif = FreeImage_GetFileType(path);
			if (fif == FIF_UNKNOWN){ fif = FreeImage_GetFIFFromFilename(path); }
			if (fif == FIF_UNKNOWN){
				printf("[FreeImage] : unsupported file format, save picture [%s] failed \n", path); return false;
			}
			
			const BYTE* bptr = bytes;
			FIBITMAP* bitmap = FreeImage_Allocate(width, height, byte_per_pixel * 8, 8, 8, 8);
			for (int y = 0; y < height; y++){
				BYTE* bits = FreeImage_GetScanLine(bitmap, y);
				for (int x = 0; x < width; x++, bits += byte_per_pixel){
					for (int i = 0; i < byte_per_pixel; ++i) {
						bits[i] = bptr[x*byte_per_pixel + i];
					}
				}
				bptr += width * byte_per_pixel;
			}
			bool bSuccess = FreeImage_Save(fif, bitmap, path, 0);
			FreeImage_Unload(bitmap);
			return bSuccess;
		}
		static FIBITMAP* read(const char *picname){
			FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;
			fif = FreeImage_GetFileType(picname);
			if (fif == FIF_UNKNOWN){ fif = FreeImage_GetFIFFromFilename(picname); }
			if (fif != FIF_UNKNOWN && FreeImage_FIFSupportsReading(fif)){
				FIBITMAP* dib = FreeImage_Load(fif, picname, 0);
				return dib;
			}
			return NULL;
		}

	private:
		FIBITMAP *dib = NULL;
		FREE_IMAGE_TYPE imageType;
	};
};


