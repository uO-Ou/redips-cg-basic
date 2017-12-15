/*
* Author : redips redips.xin@gmail.com
* Date : 2017.12.9
* Description : Material
*/

#pragma once
#include <map>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include "../Common/vec.h"
#include "../Common/FImage.h"
#include "../Common/utils.h"

namespace redips{
	class Material{
	public:
		Material(std::string name = "default material") :name(name){
			ambient = float3(0.2f);
			diffuse = float3(1.0f, 1.0f, 1.0f);
			specular = float3(1.0f);
			transparency = 1.0f;
			shininess = 0.0f;
			illum = 1;
			texture_ka = texture_kd = NULL;
		};

		float3 tex_diffuse(float u, float v) const{
			u = CLAMP(u, 0, 0.9999f);
			v = CLAMP(v, 0, 0.9999f);
			return texture_kd->tex2d(u, v);
		}
	public:
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
			if (mtl.texture_ka) os << "\thas ambient-texture" << std::endl;
			if (mtl.texture_kd) os << "\thas diffuse-texture" << std::endl;
			return os;
		}
	};

	class MtlLib{
	public:
		MtlLib(){
			//add default material
			mtls.push_back(new Material("default"));
			mtl_name_id_mapper["default"] = 0;
		}
		MtlLib(const char* file){
			//add default material
			mtls.push_back(new Material("default"));
			mtl_name_id_mapper["default"] = 0;
			//add from file
			load(file);
		}
		void load(const char *file){
			std::ifstream fin(file);
			if (!fin.is_open()){
				std::cerr << "[mtl loader] : load mtl-file [" << file << "] failed, file may not exists" << std::endl; 
				return;
			}

			std::string basepath = "";
			{
				int tid; if ((tid = std::string(file).find_last_of('/')) != -1){
					basepath = std::string(file).substr(0, tid + 1);
				}
				else if ((tid = std::string(file).find_last_of('\\')) != -1){
					basepath = std::string(file).substr(0, tid + 1);
				}
			}

			std::string buff;
			std::cout << "[mtl loader] : loading mtllib [" << file << "]\t...\n";
			while (fin >> buff){
				if (buff == "newmtl"){
					buff = StringUtil::Instance().read(fin).trim();
					mtl_name_id_mapper[file + std::string("-") + buff] = mtls.size();
					mtls.push_back(new Material(buff));
				}
				else if (buff == "Ns"){ fin >> mtls[mtls.size() - 1]->shininess; }
				else if (buff == "d"){ fin >> mtls[mtls.size() - 1]->transparency; }
				else if (buff == "illum"){ fin >> mtls[mtls.size() - 1]->illum; }
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
				else if (buff == "map_Ka"){
					buff = StringUtil::Instance().read(fin).trim();
					if (buff[1] != ':'){     //not a absolute path
						buff = basepath + buff;
					}
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
					buff = StringUtil::Instance().read(fin).trim();
					if (buff[1] != ':'){     //not a absolute path
						buff = basepath + buff;
					}
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
			std::cout << "[mtl loader] : finish" << std::endl;
		}
		~MtlLib(){
			for (auto iter = loadedImage.begin(); iter != loadedImage.end(); iter++){
				if (iter->second) delete iter->second;
			}
			loadedImage.clear();
			for (auto iter = mtls.begin(); iter != mtls.end(); iter++) delete (*iter);
			mtls.clear();
			mtl_name_id_mapper.clear();
		};
		int size() const{ return mtls.size(); };
		void setDefaultMaterial(const Material& defaultMtl){ (*mtls[0]) = defaultMtl; }
		const Material* operator[](int idx) const{ return mtls[idx]; }
		//get mtl id in mtls with name(libname-mtlname)
		int getMtlId(std::string libname, std::string mtlname){
			std::string key = libname + "-" + mtlname;
			auto iter = mtl_name_id_mapper.find(key);
			if (iter == mtl_name_id_mapper.end()) return 0; //default material
			return iter->second;
		}
	public:
		std::map<std::string, FImage*> loadedImage;
	private:
		std::vector<Material*> mtls;
		std::map<std::string, int> mtl_name_id_mapper;  //mtl [file.name,id(in mtls)]
	};
};
