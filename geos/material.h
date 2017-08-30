#pragma once

#include <map>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include "../vec.h"
#include "../FImage.h"

class Material{
public :
	Material(std::string name):name(name){
		ambient = float3(0.2f);
		diffuse = float3(1.0f,1.0f,1.0f);
		specular = float3(1.0f);
		transparency = 1.0f;
		shininess = 0.0f;
		illum = 1;
		texture_ka = texture_kd = NULL;
	};

	static int mtllib(const char* file,std::vector<Material*>& mtls){
		std::ifstream fin(file);
		if (!fin.is_open()){
			std::cerr << "[mtl loader] : load mtl-file [" << file << "] failed, file may not exists" << std::endl;
			return -1;
		}
		std::string buff;
		int mtlcnt = 0;
		std::cout << "[mtl loader] : loading mtllib [" << file<<"]\t...\t";

		std::map<std::string, FImage*> loadedImage;

		while (fin >> buff){
			if (buff == "newmtl"){
				fin >> buff;
				mtlcnt++;
				mtls.push_back(new Material(buff));
			}
			else if (buff == "Ns"){ fin >> mtls[mtls.size() - 1]->shininess; }
			else if (buff == "d"){ 	fin >> mtls[mtls.size() - 1]->transparency; }
			else if (buff == "illum"){	fin >> mtls[mtls.size() - 1]->illum; }
			else if (buff == "Ka"){		
				float3* fptr = &(mtls[mtls.size() - 1]->ambient);
				fin >> fptr->x >> fptr->y >> fptr->z;
			}
			else if (buff == "Kd"){
				float3* fptr = &(mtls[mtls.size() - 1]->diffuse);
				fin >> fptr->x >> fptr->y >> fptr->z;
			}
			else if (buff == "Ks"){
				float3* fptr = &(mtls[mtls.size() - 1]->specular);
				fin >> fptr->x >> fptr->y >> fptr->z;
			}
			else if (buff=="map_Ka"){
				fin >> buff;
				bool flag = true;
				for (auto var = loadedImage.begin(); var != loadedImage.end(); var++){
					if (var->first == buff) {
						mtls[mtls.size() - 1]->texture_ka = var->second;
						flag = false; break;
					}
				}
				if (flag){
					FImage* newImage = new FImage(buff.c_str());
					mtls[mtls.size() - 1]->texture_ka = newImage;
					loadedImage[buff] = newImage;
				}
			}
			else if (buff == "map_Kd"){
				fin >> buff;
				bool flag = true;
				for (auto var = loadedImage.begin(); var != loadedImage.end(); var++){
					if (var->first == buff) {
						mtls[mtls.size() - 1]->texture_kd = var->second;
						flag = false; break;
					}
				}
				if (flag){
					FImage* newImage = new FImage(buff.c_str());
					mtls[mtls.size() - 1]->texture_kd = newImage;
					loadedImage[buff] = newImage;
				}
			}
		}
		loadedImage.clear();
		std::cout << "finish" << std::endl;
		return mtlcnt;
	}

	float3 tex_diffuse(float u,float v) const{
		u = CLAMP(u,0,0.9999f);
		v = CLAMP(v,0,0.9999f);
		return texture_kd->tex2d(u,v);
	}
public :
	std::string name;
	float3 ambient, diffuse, specular;
	FImage *texture_ka, *texture_kd;
	float transparency;
	float shininess;
	int illum;
	friend std::ostream& operator<<(std::ostream& os, const Material& mtl){
		os << "newmtl " << mtl.name << std::endl;
		os << "\tKa " << mtl.ambient.x << "," << mtl.ambient.y << "," << mtl.ambient.z << std::endl;
		os << "\tKd " << mtl.diffuse.x << "," << mtl.diffuse.y << "," << mtl.diffuse.z << std::endl;
		os << "\tKs " << mtl.specular.x << "," << mtl.specular.y << "," << mtl.specular.z << std::endl;
		os << "\tillum " << mtl.illum << std::endl;
		os << "\td " << mtl.transparency << std::endl;
		os << "\tNs " << mtl.shininess << std::endl;
		return os;
	}
};

class MaterialManager{
public:
	MaterialManager(std::vector<Material*>* container){
		mtls = container;
	}
	~MaterialManager(){};
public:
	int loadMtllib(const char *file){
		if (isMtllibLoaded(file)) return 0;
		int curpos = this->mtls->size();

		int cnt = Material::mtllib(file, *(this->mtls));
		if (cnt < 0) return cnt;
		loadedFileNames.push_back(file);

		if (cnt>0){
			for (int i = curpos; i < curpos + cnt; i++){
				mtl_name_id_mapper[std::string(file) + "-" + (*mtls)[i]->name] = i;
			}
		}
		return cnt;
	}
	//get mtl id in mtls with name(libname-mtlname)
	int getMtlId(std::string libname, std::string mtlname){
		std::string key = libname + "-" + mtlname;
		auto iter = mtl_name_id_mapper.find(key);
		if (iter == mtl_name_id_mapper.end()) return 0; //default material
		return iter->second;
	}
private:
	bool isMtllibLoaded(const char *file){
		for (auto var = loadedFileNames.begin(); var != loadedFileNames.end(); var++){
			if (strcmp((*var), file) == 0) return true;
		}
		return false;
	};
private:
	std::vector<Material*>* mtls;
	std::vector<const char *> loadedFileNames;  //loaded mtl files list
	std::map<std::string, int> mtl_name_id_mapper;  //mtl [file.name,id(in mtls)]
};
