#pragma once
#include "FreeImage/FreeImage.h"
class FImage{
public:
	FImage(const char *file){
		dib = FImage::read(file);
		if (dib != NULL){
			width = FreeImage_GetWidth(dib);
			height = FreeImage_GetHeight(dib);
			bpp = FreeImage_GetLine(dib)/width;
			imageType = FreeImage_GetImageType(dib);
		}
		else{
			width = height = 0;
		}
	};
	~FImage(){};
public:
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
	FIBITMAP *dib;
	FREE_IMAGE_TYPE imageType;
	int width, height, bpp;
	float3 tex2d(float u,float v){
		int tu = u*width; int tv = v*height;
		BYTE *bits = FreeImage_GetScanLine(dib,tv);
		if (bpp == 24){
			   return float3(bits[tu * 3 + 0] / 255.0f, bits[tu * 3 + 1] / 255.0f, bits[tu * 3 + 2] / 255.0f);
		}
		else if (bpp == 8){
			   float tfloat = bits[tu] / 255.0f;
			   return float3(tfloat);
		}
	}
};

