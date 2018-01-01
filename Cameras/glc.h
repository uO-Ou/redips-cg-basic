#pragma once
#include "camera.h"
//#include "../Dependencies/eigen3/"
#include "../Dependencies/eigen3/Eigen/Dense"

namespace redips{
	//a 4-ray glc
	class GLC : public Camera{
	public:
		GLC(){
			type = CAMERA_TYPE::_glc_;
		};
		GLC(const float3& n0, const float3& n1, const float3& n2, const float3 &n3,
			const float3& f0, const float3& f1, const float3& f2, const float3 &f3){
			type = CAMERA_TYPE::_glc_;
			create(n0,n1,n2,n3,f0,f1,f2,f3);
		}
		~GLC(){};
		Ray getRay(float u, float v) const {
			return Ray();
		}
		void create(const float3& n0, const float3& n1, const float3& n2, const float3 &n3,
			        const float3& f0, const float3& f1, const float3& f2, const float3 &f3){
			nearp[0] = n0; nearp[1] = n1; nearp[2] = n2; nearp[3] = n3;
			farp[0] = f0; farp[1] = f1; farp[2] = f2; farp[3] = f3;

			float3 n30 = nearp[0] - nearp[3];
			float3 n31 = nearp[1] - nearp[3];
			float3 n32 = nearp[2] - nearp[3];
			area = (n30^n31).length() + (n31^n32).length();

			for (int i = 0; i < 3; ++i){
				A(i, 0) = n30[i], A(i, 1) = n31[i], A(i, 2) = n32[i];
			}
			//det = (n30[0] * n31[1] * n32[2] + n30[2] * n31[0] * n32[1] + n30[1] * n31[2] * n32[0])-
			//	  (n30[2] * n31[1] * n32[0] + n30[1] * n31[0] * n32[2] + n30[0] * n31[2] * n32[1]);
			//_RUNTIME_ASSERT_(det>0,"assert glc det>0 failed");
		}

		//check if point in the quadrangle specified by 4-ray.
		//need 2 change a method later
		/*
		bool contains(float3 point) const{
			float3 pa = nearp[0] - point;
			float3 pb = nearp[1] - point;
			float3 pc = nearp[2] - point;
			float3 pd = nearp[3] - point;
			return fabs((pa^pb).length() + (pb^pc).length() + (pc^pd).length() + (pd^pa).length() - area)<1e-4;
		}*/
		float contains(float3 point) const{
			float3 pa = nearp[0] - point;
			float3 pb = nearp[1] - point;
			float3 pc = nearp[2] - point;
			float3 pd = nearp[3] - point;
			return fabs((pa^pb).length() + (pb^pc).length() + (pc^pd).length() + (pd^pa).length() - area);
		}
		//calculate A x = b
		float3 cart2barycentric(const float3 &p) const {
			//float3 b = p - nearp[3];
			//float tx = (b[0] * n31[1] * n32[2] + b[2] * n31[0] * n32[1] + b[1] * n31[2] * n32[0]) - (b[2] * n31[1] * n32[0] + b[1] * n31[0] * n32[2] + b[0] * n31[2] * n32[1]);
			//float ty = (n30[0] * b[1] * n32[2] + n30[2] * b[0] * n32[1] + n30[1] * b[2] * n32[0]) - (n30[2] * b[1] * n32[0] + n30[1] * b[0] * n32[2] + n30[0] * b[2] * n32[1]);
			//float tz = (n30[0] * n31[1] * b[2] + n30[2] * n31[0] * b[1] + n30[1] * n31[2] * b[0]) - (n30[2] * n31[1] * b[0] + n30[1] * n31[0] * b[2] + n30[0] * n31[2] * b[1]);
			//return float3(tx/det,ty/det,tz/det);
			Eigen::Vector3d b(p[0] - nearp[3][0], p[1] - nearp[3][1], p[2] - nearp[3][2]);
			Eigen::Vector3d x = A.colPivHouseholderQr().solve(b);
			return float3(x[0],x[1],x[2]);
		};
		float3 barycentric2cart(const float3 &barycoords) const {
			return farp[0] * barycoords.x + farp[1] * barycoords.y + farp[2] * barycoords.z + farp[3] * (1.0f - barycoords.x - barycoords.y - barycoords.z);
		}

		float3 getEndsPoint(float3 startPoint) const{
			return barycentric2cart(cart2barycentric(startPoint));
		}

		Ray getRay(float3 startPoint) const {
			float3 ends = barycentric2cart(cart2barycentric(startPoint));
			return Ray(startPoint, ends - startPoint);
		}
	private:
		float3 nearp[4];
		float3 farp[4];
		float area,det;
		Eigen::Matrix3d A;
	};

};

