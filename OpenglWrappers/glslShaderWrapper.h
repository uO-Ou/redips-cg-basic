/*
* Author : redips redips.xin@gmail.com
* Date : 2017.12.15
* Description : three shader-related-class
		a. ShaderSource => tell a Shader from file or from an exists ShaderProgram
		b. Shader => create a Shader from file or an exists ShaderProgram
		c. ShaderManager => load all shaders under a specific folder automaticlly
*/
#pragma once
#include <io.h>
#include <map>
#include <vector>
#include <GL/glew.h>
#include "../Common/utils.h"
namespace redips{
	class ShaderSource{
        #define _vertex_shader_file_suffix_ ".vs"
        #define _fragment_shader_file_suffix_ ".fs"
		union Value{
		public:
			GLuint program;
			const char* path;
		};
	public:
		enum class SourceType{ _null_, _exists_program_, _from_file_ };
		Value value;
		SourceType sourceType;
		ShaderSource(){ sourceType = SourceType::_null_; }
		ShaderSource(GLuint val){
			value.program = val;
			sourceType = SourceType::_exists_program_;
		};
		ShaderSource(const char* path){
			if (strlen(path) > 0){
				value.path = path;
				sourceType = SourceType::_from_file_;
			}
			else sourceType = SourceType::_null_;
		}
		std::string vertexShaderPath() const {
			if (sourceType != SourceType::_from_file_) return nullptr;
			return std::string(value.path) + _vertex_shader_file_suffix_;
		}
		std::string fragmentShaderPath() const {
			if (sourceType != SourceType::_from_file_) return nullptr;
			return std::string(value.path) + _fragment_shader_file_suffix_;
		}
	};

	class Shader{
	public:
		Shader(){ Program = 0; }
		explicit Shader(GLuint program) { this->Program = program; };
		Shader(const Shader& another) { Program = another.Program; };
		// Constructor generates the shader on the fly
		Shader(const GLchar* vertexPath, const GLchar* fragmentPath) {
			//read code from file
			std::string vertex_code(StringUtil::Instance().file2string(vertexPath));
			std::string fragment_code(StringUtil::Instance().file2string(fragmentPath));
			if (vertex_code.length() + fragment_code.length()<=0){
				std::string shaderName(vertexPath);
				std::cerr << "[Shader] : load shader file [" << shaderName.substr(0,shaderName.length()-3)<<"] failed\n";
			}
			const GLchar* vShaderCode = vertex_code.c_str();
			const GLchar * fShaderCode = fragment_code.c_str();
			// Compile shaders
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
				std::cout << "[" << std::string(vertexPath) << "] -> ERROR::SHADER::VERTEX::COMPILATION_FAILED" << infoLog << std::endl;
				exit(0);
			}
			// Fragment Shader
			fragment = glCreateShader(GL_FRAGMENT_SHADER);
			glShaderSource(fragment, 1, &fShaderCode, NULL);
			glCompileShader(fragment);
			// Print compile errors if any
			glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
			if (!success) {
				glGetShaderInfoLog(fragment, 512, NULL, infoLog);
				std::cout << "[" << std::string(fragmentPath) << "] -> ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
				return;
			}
			// Shader Program
			this->Program = glCreateProgram();
			glAttachShader(this->Program, vertex);
			glAttachShader(this->Program, fragment);
			glLinkProgram(this->Program);
			// Print linking errors if any
			glGetProgramiv(this->Program, GL_LINK_STATUS, &success);
			if (!success) {
				glGetProgramInfoLog(this->Program, 512, NULL, infoLog);
				std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
				return;
			}
			// Delete the shaders as they're linked into our program now and no longer necessery
			glDeleteShader(vertex);
			glDeleteShader(fragment);
		}
		Shader(const ShaderSource& shader_source) {
			if (shader_source.sourceType == ShaderSource::SourceType::_exists_program_) {
				this->Program = shader_source.value.program;
			}
			else if (shader_source.sourceType == ShaderSource::SourceType::_from_file_) {
				Shader::Shader(shader_source.vertexShaderPath().c_str(), shader_source.fragmentShaderPath().c_str());
			}
		}
		void CreateFromCode(const GLchar* vShaderCode, const GLchar* fShaderCode) {
			// Compile shaders
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
				std::cout << "[" << std::string() << "] -> ERROR::SHADER::VERTEX::COMPILATION_FAILED" << infoLog << std::endl;
				exit(0);
			}
			// Fragment Shader
			fragment = glCreateShader(GL_FRAGMENT_SHADER);
			glShaderSource(fragment, 1, &fShaderCode, NULL);
			glCompileShader(fragment);
			// Print compile errors if any
			glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
			if (!success) {
				glGetShaderInfoLog(fragment, 512, NULL, infoLog);
				std::cout << "[" << std::string() << "] -> ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
				return;
			}
			// Shader Program
			this->Program = glCreateProgram();
			glAttachShader(this->Program, vertex);
			glAttachShader(this->Program, fragment);
			glLinkProgram(this->Program);
			// Print linking errors if any
			glGetProgramiv(this->Program, GL_LINK_STATUS, &success);
			if (!success) {
				glGetProgramInfoLog(this->Program, 512, NULL, infoLog);
				std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
				return;
			}
			// Delete the shaders as they're linked into our program now and no longer necessery
			glDeleteShader(vertex);
			glDeleteShader(fragment);
		}
	public:
		GLuint Program;
		// Uses the current shader
		void Use() {
			if (Program == 0){ std::cerr << "[Shader] : warning!, shader program == 0" << std::endl;; return; }
			glUseProgram(this->Program);
		}
		bool operator== (const Shader& another) const { return this->Program == another.Program; }
		Shader operator=(const Shader& another) {
			this->Program = another.Program;
			return *this;
		};
		void uniformMat44f(const char* name, const float* value){
			Use();
			glUniformMatrix4fv(glGetUniformLocation(Program, name), 1, GL_FALSE, value);
		}
		void uniformFloat3(const char* name, const redips::float3& value){
			Use();
			glUniform3f(glGetUniformLocation(Program, name), value.x, value.y, value.z);
		}
		void uniformFloat4(const char* name, const redips::float4& value) {
			Use();
			glUniform4f(glGetUniformLocation(Program, name), value.x, value.y, value.z, value.w);
		}
		void uniformFloat2(const char* name, const redips::float2& value){
			Use();
			glUniform2f(glGetUniformLocation(Program, name), value.x, value.y);
		}
		void uniformInt1(const char* name, int value){
			Use();
			glUniform1i(glGetUniformLocation(Program, name), value);
		};
		void uniformFloat1(const char* name, float value){
			Use();
			glUniform1f(glGetUniformLocation(Program, name), value);
		}

	};

	class ShaderManager{
	public:
		ShaderManager(const char* folder){
			std::vector<std::string> flist;
			getAllFiles(folder, flist);

			for (int i = 0; i < flist.size(); i++){
				if (flist[i].find(_vertex_shader_file_suffix_) != std::string::npos){
					std::string sname = flist[i].substr(0, flist[i].length() - 3);
					shaders[sname] = Shader((std::string(folder) + "/" + sname + _vertex_shader_file_suffix_).c_str(), (std::string(folder) + "/" + sname + _fragment_shader_file_suffix_).c_str());
					std::cout << "[shader manager] : shader " << sname << " loaded" << std::endl;
				}
			}
			flist.clear();
		};
		~ShaderManager(){};
		Shader operator[](std::string name){
			if (shaders.count(name)) return shaders[name];
			return Shader(0);
		}
	private:
		std::map<std::string, Shader> shaders;
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