#pragma once
/*
* Author : redips redips.xin@gmail.com
* Date : 2018.07.20
* Description : a simple quad
*/
#include <map>
#include <string>
#include <OpenglWrappers/glslShaderWrapper.h>

namespace redips{
	class QuadMesh{
		GLuint quardVao = 0, quardVbo = 0;

		Shader shader;
		void useShader(const redips::ShaderSource& source){
			if (source.sourceType == ShaderSource::SourceType::_exists_program_){
				shader = Shader(source.value.program);
			}
			else if (source.sourceType == ShaderSource::SourceType::_from_file_){
				shader = Shader((std::string(source.value.path) + _vertex_shader_file_suffix_).c_str(), (std::string(source.value.path) + _fragment_shader_file_suffix_).c_str());
			}
		}

		std::map<int, GLuint> binded_textures;
	public:
		QuadMesh(std::string shader_path = std::string(_REDIPS_ROOT_PATH_)+"/OpenglWrappers/DemoMeshes/QuadMesh"){
			float data[] = { -1.0f, 1.0f, 0.0f, -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, -1.0f, 0.0f };
			glGenBuffers(1, &quardVbo);
			glBindBuffer(GL_ARRAY_BUFFER, quardVbo);
			glBufferData(GL_ARRAY_BUFFER, sizeof(data), &data, GL_STATIC_DRAW);

			glGenVertexArrays(1, &quardVao);
			glBindVertexArray(quardVao);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
			glBindBuffer(GL_ARRAY_BUFFER, 0);

			useShader(redips::ShaderSource(shader_path.c_str()));
		}
		QuadMesh(const QuadMesh&) = delete;
		~QuadMesh(){
			glDeleteBuffers(1, &quardVbo);
			glDeleteVertexArrays(1, &quardVao);
		}

		void bindTexture(int location, GLuint texid, const char* uniform_str){
			shader.uniformInt1(uniform_str, location);
			binded_textures[location] = texid;
		}

		void render(){
			shader.Use();
			glDisable(GL_DEPTH_TEST);
			glBindVertexArray(quardVao);
			for (const auto& tex : binded_textures){
				glActiveTexture(tex.first+GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, tex.second);
			}
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			glBindVertexArray(0);
			glEnable(GL_DEPTH_TEST);
		}
	};
}