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
public:
	void load(const char* file){
		using namespace std;
		ifstream fin(file);
		while (fin >> strbuf){
			int pos = -1;
			if ((pos = startwith(strbuf, "name")) > -1){
				properties["name"] = new string(string(strbuf).substr(5, strlen(strbuf) - 5));
			}
			else if ((pos = startwith(strbuf, "operation")) > -1){
				int len = strlen("operation=");
				properties["operation"] = new string(string(strbuf).substr(len, strlen(strbuf) - len));
			}
			else if ((pos = startwith(strbuf, "algorithm")) > -1){
				int len = strlen("algorithm=");
				properties["algorithm"] = new string(string(strbuf).substr(len, strlen(strbuf) - len));
			}
			else if ((pos = startwith(strbuf, "model_chassis_path")) > -1){
				int len = strlen("model_chassis_path=");
				properties["model_chassis_path"] = new string(string(strbuf).substr(len, strlen(strbuf) - len));
			}
			else if ((pos = startwith(strbuf, "sphere_path")) > -1){
				int len = strlen("sphere_path=");
				properties["sphere_path"] = new string(string(strbuf).substr(len, strlen(strbuf) - len));
			}
			else if ((pos = startwith(strbuf, "model_path")) > -1){
				properties["model_path"] = new string(string(strbuf).substr(11, strlen(strbuf) - 11));
			}
			else if ((pos = startwith(strbuf, "meng_result")) > -1){
				int len = strlen("meng_result=");
				properties["meng_result"] = new string(string(strbuf).substr(len, strlen(strbuf) - len));
			}
			else if ((pos = startwith(strbuf, "rt_result")) > -1){
				int len = strlen("rt_result=");
				properties["rt_result"] = new string(string(strbuf).substr(len, strlen(strbuf) - len));
			}
			else if ((pos = startwith(strbuf, "mine_result")) > -1){
				int len = strlen("mine_result=");
				properties["mine_result"] = new string(string(strbuf).substr(len, strlen(strbuf) - len));
			}
			else if ((pos = startwith(strbuf, "draw_chassis")) > -1){
				int len = strlen("draw_chassis=");
				properties["draw_chassis"] = new string(string(strbuf).substr(len, strlen(strbuf) - len));
			}
			else if ((pos = startwith(strbuf, "camera_type")) > -1){
				int len = strlen("camera_type=");
				properties["camera_type"] = new string(string(strbuf).substr(len, strlen(strbuf) - len));
			}
			else if ((pos = startwith(strbuf, "box_type")) > -1){
				int len = strlen("box_type=");
				properties["box_type"] = new string(string(strbuf).substr(len, strlen(strbuf) - len));
			}
			else if ((pos = startwith(strbuf, "compress_option")) > -1){
				int len = strlen("compress_option=");
				int *op = new int;
				sscanf(strbuf + len, "%d", op);
				properties["compress_option"] = op;
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
				int *param = new int;
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
			else if ((pos = startwith(strbuf, "frame_range")) > -1){
				int len = strlen("frame_range=[");
				int sframe, eframe;
				sscanf(strbuf + len, "%d,%d]", &sframe, &eframe);
				properties["frame_range"] = new int2(sframe, eframe);
			}
			else if ((pos = startwith(strbuf, "mdv_save_result")) > -1){
				int len = strlen("mdv_save_result=");
				properties["mdv_save_result"] = new string(string(strbuf).substr(len, strlen(strbuf) - len));
			}
		}
		fin.close();
	}
	int getOperation(){
		std::string op = std::string(*((std::string*)(properties["operation"])));
		if (op == "compare") return 0;
		else if (op == "look") return 1;
		else return -1;
	};
	int getAlgorithm(){
		std::string algo = std::string(*((std::string*)(properties["algorithm"])));
		if (algo == "optix") return 1;
		return 0;
	}
	std::string getName(){ return std::string(*((std::string*)(properties["name"]))); }
	std::string getModelPath(){ return std::string(*((std::string*)(properties["model_path"]))); };
	std::string getSpherePath(){ return std::string(*((std::string*)(properties["sphere_path"]))); };
	std::string getModelWithChassisPath(){ return std::string(*((std::string*)(properties["model_chassis_path"]))); };
	std::string getChoosedColumnFile(){
		int resolution = getVoxelResolution();
		if (resolution == 7) return getMdVResultPath() + "/128/choosed_column.txt";
		else if (resolution == 8) return getMdVResultPath() + "/256/choosed_column.txt";
		return "";
	}
	std::string getColumnIndexPath(){
		int resolution = getVoxelResolution();
		if (resolution == 7) return getMdVResultPath() + "/128/indexes";
		else if (resolution == 8) return getMdVResultPath() + "/256/indexes";
		return "";
	}
	int getChoosedColumnCnt(){
		int resolution = getVoxelResolution();
		std::string _tmp_file_ = "";
		if (resolution == 7) _tmp_file_ = getMdVResultPath() + "/128/tmp";
		else if (resolution == 8) _tmp_file_ = getMdVResultPath() + "/256/tmp";

		ifstream fin(_tmp_file_.c_str());
		unsigned int tint;
		fin >> tint;
		return tint;
		fin.close();
	}
	std::string getMengResultPath(){ return std::string(*((std::string*)(properties["meng_result"]))); };
	std::string getMineResultPath(){ return std::string(*((std::string*)(properties["mine_result"]))); };
	std::string getRtResultPath(){ return std::string(*((std::string*)(properties["rt_result"]))); };

	int getCompressOp(){
		return *((int*)(properties["compress_option"]));
	}
	bool useCompressedResult(){
		return (*((int*)(properties["compress_option"]))) >= 0;
	}

	std::string getStaticLightsPath(){ return std::string(*((std::string*)(properties["static_lights"]))); };
	std::string getMovingPhcPath(){ return std::string(*((std::string*)(properties["movingphc_path"]))); };
	std::string getMdVResultPath(){ return std::string(*((std::string*)(properties["voxelization_result_dir"]))); };
	int getVoxelResolution(){ return *((int*)(properties["voxelization_resolution"])); }
	std::string getResolutionStr(){
		unsigned int resolution = 1u << (getVoxelResolution());
		char str[10]; sprintf(str, "%u", resolution);
		return std::string(str);
	}
	int getVFragsResolution(){ return *((int*)(properties["voxelization_frags_resolution"])); };
	int2 getFrameRange(){ return *((int2*)properties["frame_range"]); }
	const redips::float4* getPhcIntrinsic(){ return (redips::float4*)(properties["phc_intrinsic"]); }
	const redips::Mat33f* getPhcExtrinsic() { return (redips::Mat33f*)(properties["phc_extrinsic"]); }
	bool getBoxType(){
		if (std::string(*((std::string*)(properties["box_type"]))) == "true") return true;
		return false;
	}
	bool cameraAuto(){
		if (std::string(*((std::string*)(properties["camera_type"]))) == "auto") return true;
		return false;
	}
	bool isMdVoxelizationOn(){
		if (std::string(*((std::string*)(properties["mdvoxelization"]))) == "on") return true;
		return false;
	}
	bool saveMdvResult(){
		if ((std::string(*((std::string*)(properties["mdv_save_result"]))) != "false") || (getVoxelResolution()>7)) return true;
		return false;
	}
	bool drawChassis(){
		if ((std::string(*((std::string*)(properties["draw_chassis"]))) == "true")) return true;
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

