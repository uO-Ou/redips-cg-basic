#pragma once
#include "camera.h"

//classic pinhole camera
class PHC : public Camera{
public:
	PHC(){
		nearp = 1.0f;
		farp = 1000.0f;
		/*
		focalLength = 35.0f;
		filmsize = float2 {20.955f,11.3284f};
		resolution = int2(int(filmsize.width()), int(filmsize.height()));
		hAOV = ANGLE(atan(filmsize.width()*0.5f/focalLength))*2;
		*/
		filmsize = float2(40, 30);
		resolution = int2(768,576);
		setHAov(RAD(60));
		
		canvaSize.x = filmsize.x * nearp / focalLength;
		canvaSize.y = filmsize.y * nearp / focalLength;
		
		imageAspectRatio = filmAspectRatio = filmsize.width() / filmsize.height();
		reset();

		name = std::string("pinholeCamera");
		type = _phc_;
	};

	PHC(float focalLength, float2 filmsize, float nearp, float farp,std::string label="") : focalLength(focalLength), filmsize(filmsize), nearp(nearp), farp(farp){
		resolution = int2(int(filmsize.width()), int(filmsize.height()));
		canvaSize.x = filmsize.x * nearp / focalLength;
		canvaSize.y = filmsize.y * nearp / focalLength;
		hAOV = ANGLE(atan(filmsize.width()*0.5f / focalLength)) * 2;
		imageAspectRatio = filmAspectRatio = filmsize.width() / filmsize.height();
		reset();

		name = "pinholeCamera" + (label=="")?label:("-"+label);
		type = _phc_;
	};

	void reset(){
		cameraX = float3(1, 0, 0);
		cameraY = float3(0, 1, 0);
		cameraZ = float3(0, 0, -1);
		cameraO = float3(0, 0, 0);
		update();
	}

	void update(){
		//update camera-to-world-matrix
		camera2world.setrow(cameraX, 0);
		camera2world.setrow(cameraY, 1);
		camera2world.setrow(cameraZ, 2);

		//update world-to-camera-matrix
		Mat44f rotation = Mat44f::eye();
		rotation.setUpperLeft(camera2world.transpose());
		world2camera = Mat44f::translation(cameraO*-1) * rotation;
	}

	//gen rays in world space
	Ray getRay(float u, float v) {
		float x = (u / resolution.width() - 0.5) * canvaSize.x;
		float y = (v / resolution.height() - 0.5) * canvaSize.y;
		//ray in camera-space
		//return Ray(float3(x, y, nearp), float3(x, y, nearp));
		float3 direction = float3(x, y, nearp) * camera2world;
		return Ray(cameraO + direction, direction);
	}

	//return 2d-coord in screen space;
	bool project(const float4 &p, float2& proj) const{
		    float4 cpos = p*world2camera;
			if ((cpos.z<(nearp - 1e-2)) || (cpos.z>(farp + 1e-2))) return false;
			float scale = nearp / cpos.z;
			proj.y = cpos.y * scale / canvaSize.y + 0.5;
			proj.x = cpos.x * scale / canvaSize.x + 0.5;
			if (proj.y<0 || proj.y>1 || proj.x<0 || proj.x>1) return false;
			proj.y = (int)(proj.y*resolution.height());
			proj.x = (int)(proj.x*resolution.width());
			return true;
	}

	void setResolution(int width,int height = 0){
		if(height) resolution = int2(width,height);
		else resolution = int2(width, int(width / filmAspectRatio));
		imageAspectRatio = resolution.x*1.0f / resolution.y;
	}

	void setFocalLength(float len){
		focalLength = len;
		canvaSize.x = filmsize.x * nearp / focalLength;
		canvaSize.y = filmsize.y * nearp / focalLength;
	}

	void setHAov(float angle){
		   hAOV = ANGLE(angle);
		   setFocalLength(filmsize.width()*0.5f / tan(angle*0.5f));
	}

	//zoom-in when ratio>0, other wise zoom out
	void zoom(float ratio){ if(1.0f+ratio>0)	setFocalLength(focalLength*(1 + ratio)); }

	//all in rad
	void pan(float angle){
		   Mat33f rotate = Mat33f::pan(angle);
		   cameraX = cameraX * rotate;
		   cameraY = cameraY * rotate;
		   cameraZ = cameraZ * rotate;
		   update();
	}
	void tilt(float angle){
		   Mat33f rotate = Mat33f::tilt(angle);
		   cameraX = cameraX * rotate;
		   cameraY = cameraY * rotate;
		   cameraZ = cameraZ * rotate;
		   update();
	}
	void roll(float angle){
		   Mat33f rotate = Mat33f::roll(angle);
		   cameraX = cameraX * rotate;
		   cameraY = cameraY * rotate;
		   cameraZ = cameraZ * rotate;
		   update();
	}
	void translate(const float3& offset){
		   cameraO += offset;
		   update();
	}
	void moveTo(const float3& pos){
		cameraO = pos;
		update();
	}
	//notice: focus-pos shouldn't coinline with up!
	void lookAt(const float3& pos, const float3& focus, const float3& up){
		cameraZ = (focus - pos).unit();
		cameraX = (cameraZ ^ up).unit();
		cameraY = (cameraX ^ cameraZ).unit();
		cameraO = pos;
		update();
	}
	//check if points in nearp-farp, (in camera space)
	bool contains(const float3& point){
		return ((point.z >= nearp+1e-2) && (point.z <= farp-1e-2));
	}

	const float3& pos() const{ return cameraO; }

	const Mat33f& c2w() const { return camera2world; }

	const Mat44f& w2c() const { return world2camera; }

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
		fin >> cameraX.x >> cameraX.y >> cameraX.z ;
		fin >> cameraY.x >> cameraY.y >> cameraY.z ;
		fin >> cameraZ.x >> cameraZ.y >> cameraZ.z ;
		fin >> cameraO.x >> cameraO.y >> cameraO.z ;
		update();
	}

	friend std::ostream& operator<<(std::ostream& os,const PHC& phc){
		using namespace std;
		os << phc.name.c_str() << " : {" << endl;
		os << "\t position : (" << phc.cameraO.x << "," << phc.cameraO.y << "," << phc.cameraO.z << ")" << endl;
		os << "\t camera-X : (" << phc.cameraX.x << "," << phc.cameraX.y << "," << phc.cameraX.z << ")" << endl;
		os << "\t camera-Y : (" << phc.cameraY.x << "," << phc.cameraY.y << "," << phc.cameraY.z << ")" << endl;
		os << "\t camera-Z : (" << phc.cameraZ.x << "," << phc.cameraZ.y << "," << phc.cameraZ.z << ")" << endl;
		os << "\t horizonal-field-of-view : " << phc.hAOV << endl;
		os << "\t focal-length : " << phc.focalLength << endl;
		os << "\t neap - farp : " << phc.nearp << "," << phc.farp << endl;
		os << "\t film size " << phc.filmsize.width() << "," << phc.filmsize.height() << "(aspect-ratio:"<<phc.filmAspectRatio<<")"<<endl;
		os << "\t canva size " << phc.canvaSize.width() << "," << phc.canvaSize.height() << endl;
		os << "\t image size " << phc.resolution.width() << "," << phc.resolution.height() << "(aspect-ratio:" << phc.imageAspectRatio << ")" << endl;
		os << "\t camera-2-world-matrix : {" << endl;
		for (int i = 0; i < 3; i++) printf("\t\t %f %f %f\n", phc.camera2world[i][0], phc.camera2world[i][1], phc.camera2world[i][2]);
		os << "\t }" << endl;
		os << "\t world-2-camera-matrix : {" << endl;
		for (int i = 0; i < 4; i++) printf("\t\t %f %f %f %f\n", phc.world2camera[i][0], phc.world2camera[i][1], phc.world2camera[i][2], phc.world2camera[i][3]);
		os << "\t }" << endl;
		os << "}" << endl;
		return os;
	};

	~PHC(){};
public:
	float focalLength;
	float nearp, farp;

	int2 resolution;
	float2 filmsize;
	float2 canvaSize;

	//in world coordinates;
	float3 cameraX, cameraY, cameraZ, cameraO;
private:
	float hAOV;
	float filmAspectRatio;
	float imageAspectRatio;

	Mat33f camera2world;
	Mat44f world2camera;
};

