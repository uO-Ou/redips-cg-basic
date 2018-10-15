/*
* Author : redips redips.xin@gmail.com
* Date : 2018.06.23 latest-modification : 2018.08.09, 2018.08.22
* Description : Spherical Harmonic Lighting
* notice: assuming each vertices has associated normal
*/
#pragma once
#include "../glMeshWrapper.h"
#include "../../Geometry/sh.h"

namespace redips {
	namespace SphericalHarmonic {

		using float9  = VecXd<float,  9>;
		using double9 = VecXd<double, 9>;

		//currently only support BAND_WIDTH = 2
		class SHLightingMesh : public glMeshWrapper {
			GLuint* vbos_4_shcoefs_gpu = nullptr;
			VecXd<float, 9>* buff_4_shcoefs_cpu = nullptr;
		public:
			SHLightingMesh(const redips::Triangles* model, const SamplesOnUnitSphere<2>& samples, redips::ShaderSource shaderSource)
				: glMeshWrapper(model, shaderSource, WrapOption::_genMipmap_){
				bindVaoAttribData(0, 1, 2);
				if (shaderSource.sourceType == redips::ShaderSource::SourceType::_null_) {
					char strbuf[512];
					sprintf_s(strbuf, "%s/OpenglWrappers/DemoMeshes/SHLighting", _REDIPS_ROOT_PATH_);
					useShader(strbuf);
				}

				computeAndTransferShs(samples);
			};

			SHLightingMesh(const redips::Triangles* model, int sample_number, redips::ShaderSource shaderSource) : glMeshWrapper(model, shaderSource, WrapOption::_genMipmap_) {
				bindVaoAttribData(0, 1, 2);
				if (shaderSource.sourceType == redips::ShaderSource::SourceType::_null_) {
					char strbuf[512];
					sprintf_s(strbuf, "%s/OpenglWrappers/DemoMeshes/SHLighting", _REDIPS_ROOT_PATH_);
					useShader(strbuf);
				}

				SamplesOnUnitSphere<2> samples(sample_number);
				computeAndTransferShs(samples);
			};

			SHLightingMesh(const glMeshWrapper& another, redips::ShaderSource shaderSource = redips::ShaderSource()) = delete;

			~SHLightingMesh() { 
				glDeleteBuffers(mesh->groups.size(), vbos_4_shcoefs_gpu); 
			}
			
			void littedBy(const float9& light_r, const float9& light_g, const float9& light_b) {
				uniformFloat3("light_r_012", *((float3*)(&light_r[0])));
				uniformFloat3("light_r_345", *((float3*)(&light_r[3])));
				uniformFloat3("light_r_678", *((float3*)(&light_r[6])));
				uniformFloat3("light_g_012", *((float3*)(&light_g[0])));
				uniformFloat3("light_g_345", *((float3*)(&light_g[3])));
				uniformFloat3("light_g_678", *((float3*)(&light_g[6])));
				uniformFloat3("light_b_012", *((float3*)(&light_b[0])));
				uniformFloat3("light_b_345", *((float3*)(&light_b[3])));
				uniformFloat3("light_b_678", *((float3*)(&light_b[6])));
			}
			
			void draw() {
				if (!m_shader) { std::cerr << "shader error" << std::endl; return; };
				m_shader->Use();
				for (int i = 0; i < meshCnt; i++) {
					unsigned int flags = 0;
					if (meshMtls[i]->texture_ka != NULL && meshFaceTypes[i] == redips::GROUP_FACE_TYPE::_withtex_) {
						glActiveTexture(GL_TEXTURE0);
						glBindTexture(GL_TEXTURE_2D, mtlTextureHandle[meshMtls[i]->texture_ka]);
						glUniform1i(glGetUniformLocation(m_shader->Program, shaderAmbientTextureUniformStr), shaderAmbientTextureLocation);
						flags |= 1u;
					}
					{
						redips::float3 color = meshMtls[i]->ambient;
						glUniform3f(glGetUniformLocation(m_shader->Program, shaderAmbientColorUniformStr), color.x, color.y, color.z);
					}
					if (meshMtls[i]->texture_kd != NULL && meshFaceTypes[i] == redips::GROUP_FACE_TYPE::_withtex_) {
						glActiveTexture(GL_TEXTURE1);
						glBindTexture(GL_TEXTURE_2D, mtlTextureHandle[meshMtls[i]->texture_kd]);
						glUniform1i(glGetUniformLocation(m_shader->Program, shaderDiffuseTextureUniformStr), shaderDiffuseTextureLocation);
						flags |= 2u;
					}
					{
						redips::float3 color = meshMtls[i]->diffuse;
						glUniform3f(glGetUniformLocation(m_shader->Program, shaderDiffuseColorUniformStr), color.x, color.y, color.z);
					}
					glUniform1ui(glGetUniformLocation(m_shader->Program, shaderSurfaceTypeUniformStr), flags);

					glBindVertexArray(vaos[i]);
					glDrawArrays(GL_TRIANGLES, 0, 3 * meshFaceCnt[i]);
				}
			}
			
		private:
			void computeAndTransferShs(const SamplesOnUnitSphere<2>& samples) {
				std::cout << "[SHLightingMesh]: computing SHs per-vertex ... " << std::endl;

				//allocate memory
				vbos_4_shcoefs_gpu = new GLuint[mesh->groups.size()]; {
					glGenBuffers(mesh->groups.size(), vbos_4_shcoefs_gpu);
					int maxFaceCnt = 0; {
						for (auto& g : mesh->groups) maxFaceCnt = std::max(maxFaceCnt, g.faceCnt);
						_RUNTIME_ASSERT_(maxFaceCnt > 0, "assert [maxFaceCnt>0] failed");

						buff_4_shcoefs_cpu = new float9[maxFaceCnt * 3];
					}
				}

				//build-tree, accelerte intersection calculation
				const_cast<Triangles*>(model_ptr())->buildTree();
				
				//calculate sh coefficients
				const std::vector<float3>& normals = mesh->normals;
				const std::vector<float3>& vertices = mesh->vertices;
				for (int i = 0; i < mesh->groups.size(); ++i) {
					auto& g = mesh->groups[i];
					if (g.faceCnt <= 0) continue;
					_RUNTIME_ASSERT_(g.faceType >= GROUP_FACE_TYPE::_withnormal_, "assert [g.faceType>= GROUP_FACE_TYPE::_withnormal_] failed");

					//fill cpu array
					auto* ptr_fn = buff_4_shcoefs_cpu;
					int nid = g.nsid, fid = g.fsid, faceCnt = g.faceCnt;
					for (int neid = nid + faceCnt; nid < neid; ++nid, ++fid) {
						const int3& indices_n = mesh->faces_vn[nid];
						const int3& indices_v = mesh->faces_v[fid];
						CalculateSHCoefficients(vertices[indices_v.x], normals[indices_n.x].unit(), samples, *ptr_fn++);
						CalculateSHCoefficients(vertices[indices_v.y], normals[indices_n.y].unit(), samples, *ptr_fn++);
						CalculateSHCoefficients(vertices[indices_v.z], normals[indices_n.z].unit(), samples, *ptr_fn++);
					}

					//copy to gpu and bind to vertex
					glBindVertexArray(vaos[i]);
					glBindBuffer(GL_ARRAY_BUFFER, vbos_4_shcoefs_gpu[i]);
					glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 9 * faceCnt * 3, buff_4_shcoefs_cpu, GL_STATIC_DRAW);

					glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 9, NULL);
					glEnableVertexAttribArray(3);
					glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 9, (const void*)(sizeof(float) * 3));
					glEnableVertexAttribArray(4);
					glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 9, (const void*)(sizeof(float) * 6));
					glEnableVertexAttribArray(5);
				}

				delete buff_4_shcoefs_cpu;
				std::cout << "[SHLightingMesh]: finish" << std::endl;

			}

			void CalculateSHCoefficients(const float3& vertice, const float3& normal, const SamplesOnUnitSphere<2>& samples, float9& result) {
				double9 tmp;
				redips::HitRecord hitRecord;
				for (int i = 0; i < samples.size(); ++i) {
					auto fs_i = std::max(samples[i].dot(normal.unit()), 0.0);
					if (fs_i > 0.0) {
						Ray ray(vertice, normal);
						if (!const_cast<Triangles*>(model_ptr())->intersect(ray, hitRecord)) {
							tmp += samples.samples_vecn[i] * fs_i;
						}
					}
				}
				tmp = tmp * (4 * PI / samples.size());

				for (int i = 0; i < 9; ++i) result[i] = tmp[i];

			}

			const GLchar* shaderSurfaceTypeUniformStr = "material.flags";
			const GLchar* shaderAmbientColorUniformStr = "material.ambient";
			const GLchar* shaderDiffuseColorUniformStr = "material.diffuse";
			const GLchar* shaderAmbientTextureUniformStr = "material.ambientTexture";
			const GLchar* shaderDiffuseTextureUniformStr = "material.diffuseTexture";
			const GLuint shaderAmbientTextureLocation = 0;
			const GLuint shaderDiffuseTextureLocation = 1;
		};
	}
};
