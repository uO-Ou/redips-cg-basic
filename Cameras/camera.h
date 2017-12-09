/*
* Author : redips redips.xin@gmail.com
* Date : 2017.12.9
* Description : base camera class
*/
#pragma once
#include "../Common/vec.h"
#include "../Geometry/geometry.h"
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
