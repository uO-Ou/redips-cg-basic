#ifndef SHADER_H
#define SHADER_H

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <io.h>
#include <map>
#include <string>
#include <GL/glew.h>
namespace redips{
	class Shader{
	public:
		GLuint Program;
		// Constructor generates the shader on the fly
		Shader(const GLchar* vertexPath, const GLchar* fragmentPath) {
			// 1. Retrieve the vertex/fragment source code from filePath
			std::string vertexCode;
			std::string fragmentCode;
			std::ifstream vShaderFile;
			std::ifstream fShaderFile;
			// ensures ifstream objects can throw exceptions:
			vShaderFile.exceptions(std::ifstream::badbit);
			fShaderFile.exceptions(std::ifstream::badbit);
			try {
				// Open files
				vShaderFile.open(vertexPath);
				fShaderFile.open(fragmentPath);
				std::stringstream vShaderStream, fShaderStream;
				// Read file's buffer contents into streams
				vShaderStream << vShaderFile.rdbuf();
				fShaderStream << fShaderFile.rdbuf();
				// close file handlers
				vShaderFile.close();
				fShaderFile.close();
				// Convert stream into string
				vertexCode = vShaderStream.str();
				fragmentCode = fShaderStream.str();
			}
			catch (std::ifstream::failure e) {
				std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
			}
			const GLchar* vShaderCode = vertexCode.c_str();
			const GLchar * fShaderCode = fragmentCode.c_str();
			// 2. Compile shaders
			GLuint vertex, fragment;
			GLint success;
			GLchar infoLog[512];
			// Vertex Shader
			vertex = glCreateShader(GL_VERTEX_SHADER);
			glShaderSource(vertex, 1, &vShaderCode, NULL);
			glCompileShader(vertex);
			// Print compile errors if any
			glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
			if (!success) {
				glGetShaderInfoLog(vertex, 512, NULL, infoLog);
				std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
			}
			// Fragment Shader
			fragment = glCreateShader(GL_FRAGMENT_SHADER);
			glShaderSource(fragment, 1, &fShaderCode, NULL);
			glCompileShader(fragment);
			// Print compile errors if any
			glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
			if (!success)
			{
				glGetShaderInfoLog(fragment, 512, NULL, infoLog);
				std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
			}
			// Shader Program
			this->Program = glCreateProgram();
			glAttachShader(this->Program, vertex);
			glAttachShader(this->Program, fragment);
			glLinkProgram(this->Program);
			// Print linking errors if any
			glGetProgramiv(this->Program, GL_LINK_STATUS, &success);
			if (!success)
			{
				glGetProgramInfoLog(this->Program, 512, NULL, infoLog);
				std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
			}
			// Delete the shaders as they're linked into our program now and no longer necessery
			glDeleteShader(vertex);
			glDeleteShader(fragment);
		}
		// Uses the current shader
		void Use() {
			glUseProgram(this->Program);
		}
	};

	//shader manager : load all .vs & .fs in a folder;
	class ShaderManager{
	public:
		ShaderManager(const char* folder){
			std::vector<std::string> flist;
			getAllFiles(folder, flist);

			for (int i = 0; i < flist.size(); i++){
				if (flist[i].find(".vs") != std::string::npos){
					std::string sname = flist[i].substr(0, flist[i].length() - 3);
					shaders[sname] = new Shader((std::string(folder) + "/" + sname + ".vs").c_str(), (std::string(folder) + "/" + sname + ".fs").c_str());
					std::cout << "[shader manager] : shader " << sname << " loaded" << std::endl;
				}
			}
			flist.clear();
		};
		~ShaderManager(){};
		Shader* operator[](std::string name){
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
};
#endif