#pragma once
#include "camera.h"
//used two sove linear equations
#include <opencv2/opencv.hpp>
namespace redips{
	//a 4-ray glc
	class GLC : public Camera{
	public:
		GLC(){
			type = _glc_;
			left = cv::Mat(3, 3, CV_32F);
			right = cv::Mat(3, 1, CV_32F);
			baryCoord = cv::Mat(3, 1, CV_32F);
		};
		~GLC(){};
		void set(const float3& n0, const float3& n1, const float3& n2, const float3 &n3,
			const float3& f0, const float3& f1, const float3& f2, const float3 &f3){
			nearp[0] = n0; nearp[1] = n1; nearp[2] = n2; nearp[3] = n3;
			farp[0] = f0; farp[1] = f1; farp[2] = f2; farp[3] = f3;

			float3 n30 = nearp[0] - nearp[3];
			float3 n31 = nearp[1] - nearp[3];
			float3 n32 = nearp[2] - nearp[3];
			for (int i = 0; i < 3; i++) {
				left.at<float>(i, 0) = n30[i];
				left.at<float>(i, 1) = n31[i];
				left.at<float>(i, 2) = n32[i];
			}

			area = (n30^n31).length() + (n31^n32).length();
		}

		float3 cart2barycentric(const float3 &p){
			right.at<float>(0, 0) = p.x - nearp[3].x;
			right.at<float>(1, 0) = p.y - nearp[3].y;
			right.at<float>(2, 0) = p.z - nearp[3].z;
			cv::solve(left, right, baryCoord, CV_SVD);
			return float3(baryCoord.at<float>(0, 0), baryCoord.at<float>(1, 0), baryCoord.at<float>(2, 0));
		};
		float3 barycentric2cart(const float3 &barycoords) const {
			return farp[0] * barycoords.x + farp[1] * barycoords.y + farp[2] * barycoords.z + farp[3] * (1.0f - barycoords.x - barycoords.y - barycoords.z);
		}

		Ray getRay(float3 pos){
			float3 ends = barycentric2cart(cart2barycentric(pos));
			return Ray(pos, ends - pos);
		}
		//check if point in the quadrangle specified by 4-ray.
		//need 2 change a method later
		bool contains(float3 point) {
			float3 pa = nearp[0] - point;
			float3 pb = nearp[1] - point;
			float3 pc = nearp[2] - point;
			float3 pd = nearp[3] - point;
			return fabs((pa^pb).length() + (pb^pc).length() + (pc^pd).length() + (pd^pa).length() - area)<1e-4;
		}
	private:
		float3 nearp[4];
		float3 farp[4];
		float area;
		cv::Mat left, right, baryCoord;
	};

};

