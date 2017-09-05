#pragma once
namespace redips{
	enum MODEL_TYPE{ _sphere_, _triangle_, _panel_ };
	enum CAMERA_TYPE{ _phc_, _glc_, _mpc_ };
};
#ifndef PI
#define PI 3.1415926f
#endif

#define ANGLE_PER_RAD (57.29578f)
#define RAD_PER_AGNEL (0.0174533f)

#define RAD(angle) (angle*RAD_PER_AGNEL)
#define ANGLE(rad) (rad*ANGLE_PER_RAD)

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

