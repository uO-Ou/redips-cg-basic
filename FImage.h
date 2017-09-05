#pragma once
#include "FreeImage/FreeImage.h"
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
		float3 tex2d(float u, float v) const{
			int tu = u*width; int tv = v*height;
			BYTE *bits = FreeImage_GetScanLine(dib, tv);
			if (bpp == 24 || bpp == 32){
				int step = bpp / 8;
				return float3(bits[tu * step + 0] / 255.0f, bits[tu * step + 1] / 255.0f, bits[tu * step + 2] / 255.0f);
			}
			else if (bpp == 8){
				float tfloat = bits[tu] / 255.0f;
				return float3(tfloat);
			}
			else{
				printf("[FImage] : unsupported bpp %d\n", bpp);
				return float3(0, 1, 0);
			}
		}
		BYTE* ptr() const{
			if (dib) return FreeImage_GetBits(dib);
			return NULL;
		}
	private:
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
		FIBITMAP *dib = NULL;
		FREE_IMAGE_TYPE imageType;
	};
};


