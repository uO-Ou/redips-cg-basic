#pragma once
#include "../vec.h"
#include "../geos/geometry.h"
#include <fstream>
namespace redips{
	class Camera{
	public:
		Camera(){};
		~Camera(){};
		Ray getRay(float u, float v) {};
	public:
		CAMERA_TYPE type;
		std::string name;
	};
};
