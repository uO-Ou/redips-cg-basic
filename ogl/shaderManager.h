#pragma once
#include <io.h>
#include <map>
#include <string>
#include "shader.h"
//shader manager : load all .vs & .fs in a folder;
class ShaderManager{
public :
	ShaderManager(const char* folder){
		std::vector<std::string> flist;
		getAllFiles(folder,flist);
		
		for (int i = 0; i < flist.size(); i++){
			if (flist[i].find(".vs")!=std::string::npos){
				std::string sname = flist[i].substr(0, flist[i].length() - 3);
				shaders[sname] = new Shader((std::string(folder) + "/" + sname + ".vs").c_str(), (std::string(folder) + "/" + sname + ".fs").c_str());
				std::cout << "[shader manager] : shader " << sname << " loaded" << std::endl;
			}
		}
		flist.clear();
	};
	~ShaderManager(){};
	Shader* shader(std::string name){ 
		if (shaders.count(name)) return shaders[name]; 
		return NULL;
	}
private:
	std::map<std::string, Shader*> shaders;
	void getAllFiles(std::string path, std::vector<std::string>& files) {
		intptr_t hFile = 0;    //handle
		struct _finddata_t fileinfo;
		/*
		struct _finddata_t{
             unsigned attrib;
             time_t time_create;
             time_t time_access;
             time_t time_write;
             _fsize_t size;
             char name[_MAX_FNAME];
		};
		*/
		std::string str;
		if ((hFile = _findfirst(str.assign(path).append("\\*").c_str(), &fileinfo)) != -1) {
			do {
				if ((fileinfo.attrib & _A_SUBDIR)) {  //directory , recursive search
					if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0) {
						files.push_back(str.assign(path).append("\\").append(fileinfo.name));
						getAllFiles(str.assign(path).append("\\").append(fileinfo.name), files);
					}
				}
				else {
					files.push_back(fileinfo.name);
				}
			} while (_findnext(hFile, &fileinfo) == 0);  //寻找下一个，成功返回0，否则-1
			_findclose(hFile);
		}
	}

};

