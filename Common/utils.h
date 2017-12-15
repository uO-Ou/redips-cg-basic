#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
namespace redips {
    class StringUtil{
    public:
		static StringUtil& Instance(){ 
			if (instance == nullptr) instance = new StringUtil();
			return *instance; 
		}
        StringUtil& read(std::istream& in){
			in.getline(strbuf, BUFFER_SIZE);
			return *instance;
        }
		//std::string&& str(){	return std::string(strbuf); }
		std::string trim(){
			return trim(std::string(strbuf));
		}
		std::string file2string(const char* filepath){
			std::ifstream file(filepath);
			if (!file){ std::cerr << "[ StringUtil ] : error! cannot open file [" << std::string(filepath) << "]" << std::endl; return ""; }
			std::stringstream buff;
			buff << file.rdbuf();
			return std::string(buff.str());
		}
		static std::string trim(std::string str){
			int id1 = 0;  while (id1 < str.length()){ if (str[id1] == ' ' || str[id1] == '\t' || str[id1] == '\n') id1++; else break; }
			if (id1 == str.length()) return "";

			int id2 = str.length() - 1; while (id2>id1){ if (str[id2] == ' ' || str[id2] == '\t' || str[id2] == '\n') id2--;  else break; }
			return str.substr(id1,id2-id1+1);
		}
    private:
		static const int BUFFER_SIZE = 2048;
		static StringUtil* instance;
		char strbuf[BUFFER_SIZE];
        StringUtil(){};
        ~StringUtil(){};
    };
	StringUtil* StringUtil::instance = nullptr;
};
