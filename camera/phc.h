/*********************************************************
transform column-major point/vector
/*********************************************************/
#pragma once
#include "camera.h"
class PHC : public Camera{
public:
	PHC(std::string label="default phc"):name(label){
		    nearp = -1.0f; farp = -1000.0f;
		
		    focalLength = 35.0f;
		    filmSize = float2 {20.955f,11.3284f};
			hAov = ANGLE(atan(filmSize.width()*0.5f / focalLength)) * 2;
		    resolution = int2(int(filmSize.width())*25.6, int(filmSize.height())*25.6);
			imageAspectRatio = filmAspectRatio = filmSize.width() / filmSize.height();

			canvaSize.x = fabs(filmSize.x * nearp / focalLength);
			canvaSize.y = fabs(filmSize.y * nearp / focalLength);
			updateIntrinsic();

			type = _phc_;
			reset();
	};
	PHC(float hAov,float aspect/*width/height*/,float nearp,float farp,std::string label=""):nearp(-nearp),farp(-farp),hAov(hAov),name(label){
		    resolution = int2((int)(576*aspect),576);
		    imageAspectRatio = filmAspectRatio = aspect;

		    canvaSize.x = fabs(tan(RAD(hAov*0.5f)) * nearp * 2);
		    canvaSize.y = canvaSize.x  / aspect;

			filmSize = canvaSize; //unknown,set to canvaSize
			focalLength = fabs(nearp); //unknown,set to nearp
			updateIntrinsic();
			type = _phc_;
		    reset();
	};
	PHC(float focalLength,float2 filmSize,float nearp,float farp,std::string label=""):focalLength(focalLength),filmSize(filmSize),nearp(-nearp),farp(-farp),name(label){
		resolution = int2(int(filmSize.width()), int(filmSize.height()));
		imageAspectRatio = filmAspectRatio = filmSize.width() / filmSize.height();
		
		canvaSize.x = fabs(filmSize.x * nearp / focalLength);
		canvaSize.y = fabs(filmSize.y * nearp / focalLength);
		hAov = ANGLE(atan(filmSize.width()*0.5f / focalLength)) * 2;
		updateIntrinsic();
		type = _phc_;
		reset();
	}
	~PHC(){};
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
	void updateExtrinsic(){
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

	//gen rays in world space
	Ray getRay(float u, float v) {
		float x = (u / resolution.width() - 0.5) * canvaSize.x;
		float y = (v / resolution.height() - 0.5) * canvaSize.y;
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
		if (proj.y<0 || proj.y>=1 || proj.x<0 || proj.x>=1) flag = false;
		proj.y = (int)(proj.y*resolution.y + 0.5);
		proj.x = (int)(proj.x*resolution.x + 0.5);
		return flag;
	}

	bool project(const float4 &p, float2& proj) const{
		float3 ndc = ((projection * world2camera * p).vec3())*0.5f+float3(0.5f);
		proj.y = (int)(ndc.y*resolution.y + 0.5);
		proj.x = (int)(ndc.x*resolution.x + 0.5);
		if (ndc.x < 0 || ndc.x >= 1 || ndc.y < 0 || ndc.y >= 1 || ndc.z < 0 || ndc.z >= 1) return false;
		return true;
	}
public:
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
	}

	void load(const char* file){
		using namespace std;
		ifstream fin(file);
		if (!fin.is_open()){
			std::cout << "load finished" << endl; return;
		}
		fin >> cameraX.x >> cameraX.y >> cameraX.z;
		fin >> cameraY.x >> cameraY.y >> cameraY.z;
		fin >> cameraZ.x >> cameraZ.y >> cameraZ.z;
		fin >> cameraO.x >> cameraO.y >> cameraO.z;
		updateExtrinsic();
	}

	friend std::ostream& operator<<(std::ostream& os, const PHC& phc){
		using namespace std;
		os << phc.name.c_str() << " : {" << endl;
		os << "\t position : (" << phc.cameraO.x << "," << phc.cameraO.y << "," << phc.cameraO.z << ")" << endl;
		os << "\t camera-X : (" << phc.cameraX.x << "," << phc.cameraX.y << "," << phc.cameraX.z << ")" << endl;
		os << "\t camera-Y : (" << phc.cameraY.x << "," << phc.cameraY.y << "," << phc.cameraY.z << ")" << endl;
		os << "\t camera-Z : (" << phc.cameraZ.x << "," << phc.cameraZ.y << "," << phc.cameraZ.z << ")" << endl;
		os << "\t horizonal-field-of-view : " << phc.hAov << endl;
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

	//zoom-in when ratio>0, other wise zoom out
	void zoom(float ratio){ 
		if (1.0f + ratio > 0)	{
			setFocalLength(focalLength*(1 + ratio));
		}
	}
	//notice: focus-pos shouldn't coinline with up! this may not be left-hand base
	void lookAt(const float3& pos, const float3& focus, const float3& up){
		cameraZ = (pos - focus).unit();
		cameraX = (up ^ cameraZ).unit();
		cameraY = (cameraZ ^ cameraX).unit();
		cameraO = pos;
		updateExtrinsic();
	}
	//all in rad
	void pan(float angle){
		Mat33f rotate = Mat33f::pan(angle);
		cameraX = rotate*cameraX;
		cameraY = rotate*cameraY;
		cameraZ = rotate*cameraZ;
		updateExtrinsic();
	}
	void tilt(float angle){
		Mat33f rotate = Mat33f::tilt(angle);
		cameraX = rotate*cameraX;
		cameraY = rotate*cameraY;
		cameraZ = rotate*cameraZ;
		updateExtrinsic();
	}
	void roll(float angle){
		Mat33f rotate = Mat33f::roll(angle);
		cameraX = rotate*cameraX ;
		cameraY = rotate*cameraY ;
		cameraZ = rotate*cameraZ ;
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

public:
	std::string name;

	float2 filmSize;
	float focalLength;
	
	float nearp, farp;
	float2 canvaSize;

	int2 resolution;

	//in world coordinates;
	float3 cameraX, cameraY, cameraZ, cameraO;
private:
	float hAov;
	float filmAspectRatio;
	float imageAspectRatio;

	Mat33f camera2world3;
	Mat44f camera2world4;
	Mat44f world2camera;
	Mat44f projection;
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
		hAov = ANGLE(atan(filmSize.x*0.5f/focalLength))*2;
		canvaSize.x = fabs(filmSize.x * nearp / focalLength);
		canvaSize.y = fabs(filmSize.y * nearp / focalLength);
		updateIntrinsic();
	}
	void setHAov(float angle/*rad*/){
		hAov = ANGLE(angle);
		focalLength = filmSize.width()*0.5f / tan(angle*0.5f);
		canvaSize.x = fabs(filmSize.x * nearp / focalLength);
		canvaSize.y = fabs(filmSize.y * nearp / focalLength);
		updateIntrinsic();
	}
};

