/*
* Author : redips redips.xin@gmail.com
* Date : 2017.12.25
* Description : constants and enums
*/
#pragma once

//important! 
//_REDIPS_ROOT_PATH_ indicates where to find redips-related resources, such as vs/fs shaders
#define _REDIPS_ROOT_PATH_ "E:/Documents/CG/CGLib/redips"

namespace redips{
	enum class MODEL_TYPE{ _sphere_, _triangle_, _panel_ };
	enum class CAMERA_TYPE{ _phc_, _glc_, _mpc_ };
	enum class GROUP_FACE_TYPE{ _unknown_face_type_, _single_, _withnormal_, _withtex_, _other_ };
	enum class GL_TEXTURE_TYPE { _1d_, _2d_, _3d_, _cubemap_, _unknown_texture_type_ };
};

#ifndef PI
#define PI (3.1415926535897932)
#endif

#define PI_INV 0.31830989f

#define ANGLE_PER_RAD (57.29578f)
#define RAD_PER_AGNEL (0.0174533f)

#define RAD(angle) ((angle)*RAD_PER_AGNEL)
#define ANGLE(rad) ((rad)*ANGLE_PER_RAD)

#ifndef MAX
#define MAX(a,b) (a)>=(b)?(a):(b)
#endif

#ifndef MIN
#define MIN(a,b) (a)<=(b)?(a):(b)
#endif

#ifndef CLAMP
#define CLAMP(a,b,c) ((a)<(b)?(b):((a)>(c)?(c):(a)))
#endif

#define MIX(a,b,mix) ((b*mix)+(a*(1.0f-mix)))

typedef unsigned char BYTE;

#define CHECK_GL_ERROR(s) {if(glGetError()!=GL_NO_ERROR){printf("glError %s\n",(s));exit(-1);};}

#define _RUNTIME_ASSERT_(exp,str) {if((exp)!=true){printf("assert [%s] failed\n",str);exit(-2);}}