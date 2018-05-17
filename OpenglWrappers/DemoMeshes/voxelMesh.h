/*
* Author : redips redips.xin@gmail.com
* Date : 2018.04.16
* Description : VoxelMesh @ Qingdao
*/
#pragma once
#include <vector>
#include "../glMeshWrapper.h"

namespace redips{
	class VoxelMesh : public redips::glMeshWrapper{
		GLuint count_ssbo = 0;   //counter
		GLuint frags_ssbo = 0;    //fragments
		unsigned int TOTAL_FRAG_CNT = 0;
		redips::BOX mbox;
	public:
		unsigned int PRECESION = 0;
		std::vector<unsigned int> voxel_cnt_vec;
		GLuint voxelvbo = 0;      //show voxels;
		unsigned int VOXEL_CNT = 0;
	public:
		VoxelMesh(const redips::Triangles* model) : glMeshWrapper(model, redips::ShaderSource()){
			bindVaoAttribData(0, -1, -1);
			char strbuf[512];
			sprintf_s(strbuf, "%s/OpenglWrappers/DemoMeshes/voxel", _REDIPS_ROOT_PATH_);
			useShader(strbuf);
			mbox = this->model_ptr()->aabb_T();
		};
		VoxelMesh(const glMeshWrapper& another) : glMeshWrapper(another, redips::ShaderSource()){
			bindVaoAttribData(0, -1, -1);
			char strbuf[512];
			sprintf_s(strbuf, "%s/OpenglWrappers/DemoMeshes/voxel", _REDIPS_ROOT_PATH_);
			useShader(strbuf);
			mbox = this->model_ptr()->aabb_T();
		}
		~VoxelMesh(){
			glDeleteBuffers(1,&count_ssbo);
		};
		void process(unsigned int precision = 8u){
			_RUNTIME_ASSERT_(precision<10u && precision>0u, "assert [precision<10u && precision>0u] failed");
			PRECESION = precision;
			m_shader->Use();
			int bufferBytes = 1u << (precision * 3 + 2);
			{   //generate ssbo and bind
				glGenBuffers(1, &count_ssbo);
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, count_ssbo);
				glBufferData(GL_SHADER_STORAGE_BUFFER, bufferBytes, nullptr, GL_DYNAMIC_COPY);
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

				GLuint blockIndex = glGetProgramResourceIndex(m_shader->Program, GL_SHADER_STORAGE_BLOCK, "counter");
				glShaderStorageBlockBinding(m_shader->Program, blockIndex, 0);
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, count_ssbo);
			}
			glDisable(GL_CULL_FACE);
			glDisable(GL_DEPTH_TEST);

			glUniform1ui(glGetUniformLocation(m_shader->Program, "voxel_resolution"), precision);
			glUniformMatrix4fv(glGetUniformLocation(m_shader->Program, "model"), 1, GL_FALSE, ((this->model_ptr())->Transform()).transpose().ptr());
			{   //first pass, render 3 times
				glUniform1ui(glGetUniformLocation(m_shader->Program, "pass"), 0u);

				glUniformMatrix4fv(glGetUniformLocation(m_shader->Program, "projection"), 1, GL_FALSE, (GeoUtil::glOrtho(mbox)).transpose().ptr());
				glUniformMatrix3fv(glGetUniformLocation(m_shader->Program, "swizzler"), 1, GL_FALSE, Mat33f::xyz().transpose().ptr());
				this->drawAllMeshes();

				glUniformMatrix4fv(glGetUniformLocation(m_shader->Program, "projection"), 1, GL_FALSE, (GeoUtil::glOrtho(mbox.back(), mbox.front(), mbox.left(), mbox.right(), mbox.bottom(), mbox.top())).transpose().ptr());
				glUniformMatrix3fv(glGetUniformLocation(m_shader->Program, "swizzler"), 1, GL_FALSE, Mat33f::zxy().transpose().ptr());
				this->drawAllMeshes();

				glUniformMatrix4fv(glGetUniformLocation(m_shader->Program, "projection"), 1, GL_FALSE, (GeoUtil::glOrtho(mbox.bottom(), mbox.top(), mbox.back(), mbox.front(), mbox.left(), mbox.right())).transpose().ptr());
				glUniformMatrix3fv(glGetUniformLocation(m_shader->Program, "swizzler"), 1, GL_FALSE, Mat33f::yzx().transpose().ptr());
				this->drawAllMeshes();
				//glViewport(orivp[0], orivp[1], orivp[2], orivp[3]);
			}
			glEnable(GL_CULL_FACE);
			glEnable(GL_DEPTH_TEST);
			
			//copy counter 2 cpu
			voxel_cnt_vec.resize(bufferBytes >> 2);
			glBindBuffer(GL_ARRAY_BUFFER,count_ssbo);
			glGetBufferSubData(GL_ARRAY_BUFFER, 0, bufferBytes,&voxel_cnt_vec[0]);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			{
				auto model_center = mbox.heart();
				float resolution_inv = 1.0f/(1u << (precision));
				std::vector<redips::float3> dots;
				for (unsigned int i = 0; i < voxel_cnt_vec.size(); ++i){
					if (voxel_cnt_vec[i]){
						float x = i >> (precision + precision);
						float y = (i >> precision) & ((1u << precision) - 1u);
						float z = i & ((1u << precision) - 1u);
						dots.push_back((redips::float3(x, y, z)*resolution_inv - 0.5f)*mbox.dim() + model_center);
					}
				}
				VOXEL_CNT = dots.size();
				glGenBuffers(1,&voxelvbo);
				glBindBuffer(GL_ARRAY_BUFFER, voxelvbo);
				glBufferData(GL_ARRAY_BUFFER, sizeof(float)* 3 * VOXEL_CNT, &dots[0], GL_DYNAMIC_DRAW);
				glBindBuffer(GL_ARRAY_BUFFER, 0);
				dots.clear();
			}
			
			std::cout << "[voxel mesh]: process finish" << std::endl;
		}
	};
};

