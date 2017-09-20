#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <vec.h>
class PropManager{
public:
	PropManager(const char * file){ load(file); };
	~PropManager(){
		for (auto iter = properties.begin(); iter != properties.end(); iter++){
			delete iter->second;
		}
	};
	void print(){
		printf("name=%s\n",this->getName().c_str());
		printf("model_path=%s\n", this->getModelPath().c_str());
		printf("movingphc_path=%s\n", this->getMovingPhcPath().c_str());
		printf("static_lights=%s\n", this->getStaticLightsPath().c_str());
		printf("voxelization_resolution=%d\n", this->getVoxelResolution());
		printf("phc_intrinsic=[%f,%f,%f,%f]\n", (*(this->getPhcIntrinsic())).x, (*(this->getPhcIntrinsic())).y, (*(this->getPhcIntrinsic())).z, (*(this->getPhcIntrinsic())).w);
		printf("phc_extrinsic=["); for (int i = 0; i < 9; i++) printf("%f,", (*(this->getPhcExtrinsic()))[i / 3][i % 3]); printf("\b]\n");
	}
public:
	void load(const char* file){
		using namespace std;
		ifstream fin(file);
		while (fin >> strbuf){
			int pos = -1;
			if ((pos = startwith(strbuf, "name")) > -1){
				properties["name"] = new string(string(strbuf).substr(5, strlen(strbuf) - 5));
			}
			else if ((pos = startwith(strbuf, "model_path")) > -1){
				properties["model_path"] = new string(string(strbuf).substr(11, strlen(strbuf) - 11));
			}
			else if ((pos = startwith(strbuf, "useMovingPhc")) > -1){
				properties["useMovingPhc"] = new string(string(strbuf).substr(13, strlen(strbuf) - 13));
			}
			else if ((pos = startwith(strbuf, "phc_intrinsic")) > -1){
				redips::float4* param = new redips::float4();
				sscanf(strbuf + 15, "%f,%f,%f,%f]", &(param->x), &(param->y), &(param->z), &(param->w));
				properties["phc_intrinsic"] = param;
			}
			else if ((pos = startwith(strbuf, "phc_extrinsic")) > -1){
				redips::Mat33f* param = new redips::Mat33f();
				sscanf(strbuf + 15, "%f,%f,%f,%f,%f,%f,%f,%f,%f]", &((*param)[0][0]), &((*param)[0][1]), &((*param)[0][2]),
					&((*param)[1][0]), &((*param)[1][1]), &((*param)[1][2]),
					&((*param)[2][0]), &((*param)[2][1]), &((*param)[2][2]));
				properties["phc_extrinsic"] = param;
			}
			else if ((pos = startwith(strbuf, "movingphc_path")) > -1){
				properties["movingphc_path"] = new string(string(strbuf).substr(15, strlen(strbuf) - 15));
			}
			else if ((pos = startwith(strbuf, "static_lights")) > -1){
				properties["static_lights"] = new string(string(strbuf).substr(14, strlen(strbuf) - 14));
			}
			else if ((pos = startwith(strbuf, "voxelization_resolution")) > -1){
				int *param =new int;
				sscanf(strbuf + 24, "%d", param);
				properties["voxelization_resolution"] = param;
			}
			else if ((pos = startwith(strbuf, "voxelization_frags_resolution")) > -1){
				int *param = new int;
				sscanf(strbuf + strlen("voxelization_frags_resolution="), "%d", param);
				properties["voxelization_frags_resolution"] = param;
			}
			else if ((pos = startwith(strbuf, "voxelization_result_dir")) > -1){
				int len = strlen("voxelization_result_dir=");
				properties["voxelization_result_dir"] = new string(string(strbuf).substr(len, strlen(strbuf) - len));
			}
			else if ((pos = startwith(strbuf, "mdvoxelization")) > -1){
				int len = strlen("mdvoxelization=");
				properties["mdvoxelization"] = new string(string(strbuf).substr(len, strlen(strbuf) - len));
			}
		}
		fin.close();
	}
	std::string getName(){ return std::string(*((std::string*)(properties["name"]))); }
	std::string getModelPath(){ return std::string(*((std::string*)(properties["model_path"]))); };
	std::string getStaticLightsPath(){ return std::string(*((std::string*)(properties["static_lights"]))); };
	std::string getMovingPhcPath(){ return std::string(*((std::string*)(properties["movingphc_path"]))); };
	std::string getMdVResultPath(){ return std::string(*((std::string*)(properties["voxelization_result_dir"]))); };
	int getVoxelResolution(){ return *((int*)(properties["voxelization_resolution"])); }
	int getVFragsResolution(){ return *((int*)(properties["voxelization_frags_resolution"])); };
	const redips::float4* getPhcIntrinsic(){ return (redips::float4*)(properties["phc_intrinsic"]); }
	const redips::Mat33f* getPhcExtrinsic() { return (redips::Mat33f*)(properties["phc_extrinsic"]); }
	bool useMovingPhc(){
		if (std::string(*((std::string*)(properties["useMovingPhc"]))) == "false") return false;
		return true;
	}
	bool isMdVoxelizationOn(){
		if (std::string(*((std::string*)(properties["mdvoxelization"]))) == "on") return true;
		return false;
	}

private:
	std::map<std::string, void*> properties;
	char strbuf[666];
	int startwith(const char* a, const char* b){
		int len = strlen(b);
		if (strlen(a) <= len) return -1;
		for (int i = 0; i < len; i++) if (a[i] != b[i]) return -1;
		return len;
	}
};

