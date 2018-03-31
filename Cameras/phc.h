/*
* Author : redips redips.xin@gmail.com
* Date : 2017.12.9
* Description : standard pinhole camera, use/transform column-major point/vector
*/
#pragma once
#include "camera.h"
namespace redips{
	class PhC : public Camera{
	public:
		PhC(std::string label = "default phc") :name(label){
			nearp = -1.0f; farp = -1000.0f;

			focalLength = 35.0f;
			filmSize = float2{ 20.955f, 11.3284f };
			vAov = ANGLE(atan(filmSize.height()*0.5f / focalLength)) * 2;

			resolution = int2(int(filmSize.width())*25.6, int(filmSize.height())*25.6);
			imageAspectRatio = filmAspectRatio = filmSize.width() / filmSize.height();

			canvaSize.x = fabs(filmSize.x * nearp / focalLength);
			canvaSize.y = fabs(filmSize.y * nearp / focalLength);
			updateIntrinsic();

			type = CAMERA_TYPE::_phc_;
			reset();
		};
		PhC(float vAov, float aspect/*width/height*/, float nearp, float farp, std::string label = "") :nearp(-nearp), farp(-farp), vAov(vAov), name(label){
			resolution = int2((int)(576 * aspect), 576);
			imageAspectRatio = filmAspectRatio = aspect;

			/**************************************************
			canvaSize.x = fabs(tan(RAD(hAov*0.5f)) * nearp * 2);
			canvaSize.y = canvaSize.x / aspect;
			**************************************************/
			canvaSize.y = fabs(tan(RAD(vAov*0.5f)) * nearp * 2);
			canvaSize.x = canvaSize.y * aspect;

			filmSize = canvaSize;         //unknown,set to canvaSize
			focalLength = fabs(nearp);    //unknown,set to nearp
			updateIntrinsic();
			type = CAMERA_TYPE::_phc_;
			reset();
		};
		PhC(float focalLength, float2 filmSize, float nearp, float farp, std::string label = "") :focalLength(focalLength), filmSize(filmSize), nearp(-nearp), farp(-farp), name(label){
			resolution = int2(int(filmSize.width()), int(filmSize.height()));
			imageAspectRatio = filmAspectRatio = filmSize.width() / filmSize.height();

			canvaSize.x = fabs(filmSize.x * nearp / focalLength);
			canvaSize.y = fabs(filmSize.y * nearp / focalLength);
			vAov = ANGLE(atan(filmSize.height()*0.5f / focalLength)) * 2;
			updateIntrinsic();
			type = CAMERA_TYPE::_phc_;
			reset();
		}
		PhC& operator=(const PhC& another){
			resolution = another.resolution;
			filmAspectRatio = another.filmAspectRatio;
			imageAspectRatio = another.imageAspectRatio;

			cameraO = another.cameraO;
			cameraX = another.cameraX;
			cameraY = another.cameraY;
			cameraZ = another.cameraZ;
			updateExtrinsic();

			farp = another.farp;
			nearp = another.nearp;
			filmSize = another.filmSize;
			setFocalLength(another.focalLength);
			return *this;
		}
		~PhC(){};
		
		//notice: focus-pos shouldn't coinline with up! this may not be left-hand base
		void lookAt(const float3& pos, const float3& focus, const float3& up){
			cameraZ = (pos - focus).unit();
			cameraX = (up ^ cameraZ).unit();
			cameraY = (cameraZ ^ cameraX).unit();
			cameraO = pos;
			updateExtrinsic();
		}
		//rotate around y (rad)
		void pan(float angle){
			Mat33f rotate = Mat33f::pan(angle);
			cameraX = rotate*cameraX;
			cameraY = rotate*cameraY;
			cameraZ = rotate*cameraZ;
			updateExtrinsic();
		}
		//rotate around x (rad)
		void tilt(float angle){
			Mat33f rotate = Mat33f::tilt(angle);
			cameraX = rotate*cameraX;
			cameraY = rotate*cameraY;
			cameraZ = rotate*cameraZ;
			updateExtrinsic();
		}
		//rotate around z (rad)
		void roll(float angle){
			Mat33f rotate = Mat33f::roll(angle);
			cameraX = rotate*cameraX;
			cameraY = rotate*cameraY;
			cameraZ = rotate*cameraZ;
			updateExtrinsic();
		}
		void translate(const float3& offset){
			cameraO += offset;
			updateExtrinsic();
		}
		void moveTo(const float3& pos){
			cameraO = pos;
			updateExtrinsic();
		}

		//zoom-in when ratio>0, other wise zoom out
		void zoom(float ratio){
			if (1.0f + ratio > 0)	{
				setFocalLength(focalLength*(1 + ratio));
			}
		}

		void updateIntrinsic(){
			//update projection-matrix
			/*
			2n/(r-l)		0		(r+l)/(r-l)		0
			0		2n/(t-b)		(t+b)/(t-b)		0
			0		0		-(f+n)/(f-n)		-2fn/(f-n)
			0		0		-1		0
			*/
			projection[0][0] = -nearp * 2 / canvaSize.x;
			projection[1][1] = -nearp * 2 / canvaSize.y;
			projection[2][2] = (farp + nearp) / (nearp - farp);
			projection[2][3] = farp*nearp * 2 / (farp - nearp);
			projection[3][2] = -1;
		}
		virtual void updateExtrinsic(){
			//update camera-to-world-matrix
			camera2world4.setcol(float4(cameraX, 0.0f), 0);
			camera2world4.setcol(float4(cameraY, 0.0f), 1);
			camera2world4.setcol(float4(cameraZ, 0.0f), 2);
			camera2world4.setcol(float4(cameraO, 1.0f), 3);

			camera2world3.setcol(cameraX, 0);
			camera2world3.setcol(cameraY, 1);
			camera2world3.setcol(cameraZ, 2);

			//update world-to-camera-matrix
			world2camera.setrow(float4(cameraX, 0.0f), 0);
			world2camera.setrow(float4(cameraY, 0.0f), 1);
			world2camera.setrow(float4(cameraZ, 0.0f), 2);
			world2camera.setrow(float4(0.0f, 0.0f, 0.0f, 1.0f), 3);
			world2camera = world2camera * Mat44f::translation(cameraO*-1);
		}

		const float3& pos() const{ return cameraO; }
		const Mat33f& c2w3() const { return camera2world3; }
		const Mat44f& c2w4() const { return camera2world4; };
		const Mat44f& w2c() const { return world2camera; }
		const Mat44f& glProjection() const { return projection.transpose(); };
		const Mat44f& glView() const { return world2camera.transpose(); };
		
		//get pixel position in world-space
		float3 getPixelWPos(int x,int y){
			   float tx = (((0.5f + x) / resolution.x) - 0.5f) * canvaSize.x;
			   float ty = (((0.5f + y) / resolution.y) - 0.5f) * canvaSize.y;
			   return (camera2world3 * float3(tx, ty, nearp))+cameraO;
		}
		//gen rays in world-space
		Ray getRay(float u, float v) const{
			float x = ((u+0.5f) / resolution.x - 0.5f) * canvaSize.x;
			float y = ((v+0.5f) / resolution.y - 0.5f) * canvaSize.y;
			float3 direction = camera2world3 * float3(x, y, nearp);
			return Ray(cameraO + direction, direction);
		}

		//return 2d-coord in screen space;
		bool project_old(const float4 &p, float2& proj) const{
			bool flag = true;
			float4 cpos = world2camera * p;
			if ((cpos.z>(nearp + 1e-2)) || (cpos.z<(farp - 1e-2))) flag = false;
			float scale = nearp / cpos.z;
			proj.y = (cpos.y * scale / canvaSize.y + 0.5);
			proj.x = (cpos.x * scale / canvaSize.x + 0.5);
			if (proj.y<0 || proj.y >= 1 || proj.x<0 || proj.x >= 1) flag = false;
			proj.y = (int)(proj.y*resolution.y + 0.5);
			proj.x = (int)(proj.x*resolution.x + 0.5);
			return flag;
		}

		bool project(const float4 &p, float2& proj) const{
			float3 ndc = ((projection * world2camera * p).vec3())*0.5f + float3(0.5f);
			proj.y = (int)(ndc.y*resolution.y + 0.5);
			proj.x = (int)(ndc.x*resolution.x + 0.5);
			if (ndc.x < 0 || ndc.x >= 1 || ndc.y < 0 || ndc.y >= 1 || ndc.z < 0 || ndc.z >= 1) return false;
			return true;
		}
	public:
		//io function
		void save(const char* file){
			using namespace std;
			ofstream fout(file);
			if (!fout.is_open()){
				std::cout << "save camera failed" << endl; return;
			}
			fout << cameraX.x << " " << cameraX.y << " " << cameraX.z << endl;
			fout << cameraY.x << " " << cameraY.y << " " << cameraY.z << endl;
			fout << cameraZ.x << " " << cameraZ.y << " " << cameraZ.z << endl;
			fout << cameraO.x << " " << cameraO.y << " " << cameraO.z << endl;

			fout << nearp << " " << farp << endl;
			fout << focalLength << endl;
			fout << filmSize.x << " " << filmSize.y << endl;

			fout << resolution.x << " " << resolution.y << endl;
			fout << vAov << endl;
			fout.close();

			std::cout << "[phc] : save camera parameters finished!" << endl;
		}
		void load(const char* file){
			using namespace std;
			ifstream fin(file);
			if (!fin.is_open()){
				std::cout << "[PhC] : load phc [" << file << "] failed" << endl; return;
			}
			fin >> cameraX.x >> cameraX.y >> cameraX.z;
			fin >> cameraY.x >> cameraY.y >> cameraY.z;
			fin >> cameraZ.x >> cameraZ.y >> cameraZ.z;
			fin >> cameraO.x >> cameraO.y >> cameraO.z;
			updateExtrinsic();

			fin >> nearp >> farp;
			fin >> focalLength;
			fin >> filmSize.x >> filmSize.y;

			fin >> resolution.x >> resolution.y;
			setFocalLength(focalLength);

			imageAspectRatio = resolution.x*1.0 / resolution.y;
			filmAspectRatio = filmSize.x / filmSize.y;
			fin.close();
		}
		friend std::ostream& operator<<(std::ostream& os, const PhC& phc){
			using namespace std;
			os << phc.name.c_str() << " : {" << endl;
			os << "\t position : (" << phc.cameraO.x << "," << phc.cameraO.y << "," << phc.cameraO.z << ")" << endl;
			os << "\t camera-X : (" << phc.cameraX.x << "," << phc.cameraX.y << "," << phc.cameraX.z << ")" << endl;
			os << "\t camera-Y : (" << phc.cameraY.x << "," << phc.cameraY.y << "," << phc.cameraY.z << ")" << endl;
			os << "\t camera-Z : (" << phc.cameraZ.x << "," << phc.cameraZ.y << "," << phc.cameraZ.z << ")" << endl;
			os << "\t vertical-field-of-view : " << phc.vAov << "[" << RAD(phc.vAov) << "]" << endl;
			os << "\t focal-length : " << phc.focalLength << endl;
			os << "\t neap - farp : " << phc.nearp << "," << phc.farp << endl;
			os << "\t film size " << phc.filmSize.width() << "," << phc.filmSize.height() << "(aspect-ratio:" << phc.filmAspectRatio << ")" << endl;
			os << "\t canva size " << phc.canvaSize.width() << "," << phc.canvaSize.height() << endl;
			os << "\t image size " << phc.resolution.width() << "," << phc.resolution.height() << "(aspect-ratio:" << phc.imageAspectRatio << ")" << endl;
			os << "\t camera-2-world-matrix : {" << endl;
			for (int i = 0; i < 3; i++) printf("\t\t %f %f %f\n", phc.camera2world3[i][0], phc.camera2world3[i][1], phc.camera2world3[i][2]);
			os << "\t }" << endl;
			os << "\t world-2-camera-matrix : {" << endl;
			for (int i = 0; i < 4; i++) printf("\t\t %f %f %f %f\n", phc.world2camera[i][0], phc.world2camera[i][1], phc.world2camera[i][2], phc.world2camera[i][3]);
			os << "\t }" << endl;
			os << "}" << endl;
			return os;
		};

		void setResolution(int width, int height = 0){
			if (height) {
				resolution = int2(width, height);
				imageAspectRatio = resolution.x*1.0f / resolution.y;
			}
			else resolution = int2(width, int(width / imageAspectRatio));
		}
		void setExtrinsic(const Mat33f& axises, const float3& position){
			cameraX = axises.col(0);
			cameraY = axises.col(1);
			cameraZ = axises.col(2);
			cameraO = position;
			updateExtrinsic();
		}
	private:
		void reset(){
			cameraX = float3(1, 0, 0);
			cameraY = float3(0, 1, 0);
			cameraZ = float3(0, 0, 1);
			cameraO = float3(0, 0, 0);
			updateExtrinsic();
		}
		void setFocalLength(float len){
			focalLength = len;
			vAov = ANGLE(atan(filmSize.y*0.5f / focalLength)) * 2;
			canvaSize.x = fabs(filmSize.x * nearp / focalLength);
			canvaSize.y = fabs(filmSize.y * nearp / focalLength);
			updateIntrinsic();
		}
		void setVAov(float angle/*rad*/){
			vAov = ANGLE(angle);
			focalLength = filmSize.height()*0.5f / tan(angle*0.5f);
			canvaSize.x = fabs(filmSize.x * nearp / focalLength);
			canvaSize.y = fabs(filmSize.y * nearp / focalLength);
			updateIntrinsic();
		}

	public:
		std::string name;

		float2 filmSize;
		float focalLength;

		float nearp, farp;
		float2 canvaSize;

		//in world coordinates;
		float3 cameraX, cameraY, cameraZ, cameraO;
	public:
		//float hAov;   //deprecated, horizontal angle of view
		float vAov;     //vertical angle of view
		
		float filmAspectRatio;
		float imageAspectRatio;

		Mat33f camera2world3;
		Mat44f camera2world4;
		Mat44f world2camera;
		Mat44f projection;
	};
};
