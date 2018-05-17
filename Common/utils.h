#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include "./vec.h"

namespace redips {
	class StringUtil;
	static StringUtil* instance = nullptr;
    class StringUtil{
    public:
		static StringUtil& Instance(){ 
			if (instance == nullptr) instance = new StringUtil();
			return *instance; 
		}
		bool startwith(std::string s1,const char* s2){
			int len = strlen(s2);
			for (int i = 0; i < len; i++) if (s1[i] != s2[i]) return false;
			return true;
		}
		explicit operator std::string(){ return std::string(strbuf); }
		static int split(std::string str,std::string spliters,std::vector<std::string>& result){
			int spliter_cnt = spliters.size();
			auto checker = [=](char c){ for (int i = 0; i < spliter_cnt; i++) if (c == spliters[i]) return false; return true; };
			
			result.clear();
			int i, j;
			for (i = 0; i < str.length(); i=j){
				j = i;
				while (j < str.length() && checker(str[j])) j++;
				if (j>i) result.push_back(str.substr(i,j-i));
				else j=i+1;
			}
			return result.size();
		}
		static float3 split2Float3(char* str){
			//check if c is a num/./-/e/E
			auto checker = [](char c){ return (c >= '0'&&c <= '9') || (c == '.') || (c == '-') || (c == 'e') || (c == 'E'); };

			float3 value(0.0f);
			int i, j; int fid = 0; int len = strlen(str);
			for (i = 0; i < len; i = j){
				j = i + 1;
				if (checker(str[i])) {
					while (checker(str[j])) j++;
					str[j] = '\0';
					if(fid<3) value[fid++] = atof(str + i);
				}
			}
			return value;
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
		//static StringUtil* instance;
		char strbuf[BUFFER_SIZE];
        StringUtil(){};
        ~StringUtil(){};
    };
	//StringUtil* StringUtil::instance = nullptr;
	
	class MathUtil{
	public:
		static int solve_quadratic_equation(double a,double b,double c,redips::float2& result){
			if(fabs(a)<1e-6){
				if(fabs(b)<1e-6){
					if(fabs(c)<1e-6) { result.x = 0; return 1; }
					else return -1;
				}
				else{ result.x = -c/b; return 1;}				
			}
			else{
				auto delta = b*b-4*a*c;
				if(delta<0) return -1;
				if(fabs(delta)<1e-6){
					result.x = -b/a*0.5; return 1;
				}
				else{
					delta = sqrt(delta);
					result.x = (-b-delta)/(2*a);
					result.y = (-b+delta)/(2*a);
					return 2;
				}
			}
			
		}
	};
	
	
};

#define STRING_UTIL redips::StringUtil::Instance()