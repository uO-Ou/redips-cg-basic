/*
* Author : redips redips.xin@gmail.com
* Date : 2018.06.23 latest-modification : 2018.08.09, 2018.08.22, 2018.09.19
* Description : Spherical Harmonic Lighting
* notice: assuming each vertices has associated normal
*/
#pragma once
#include "../glMeshWrapper.h"
#include "../../Geometry/sh.h"

namespace redips {
	//currently only support Bandwidth={2,3,4,5,6}
	namespace SphericalHarmonic {
		template<int BAND_WIDTH = 2>
		class SHLightingMesh : public glMeshWrapper {

			const static int DIM = (BAND_WIDTH + 1) * (BAND_WIDTH + 1);

			GLuint*			   vbos_4_shcoefs_gpu = nullptr;
			VecXd<float, DIM>* buff_4_shcoefs_cpu = nullptr;

			std::vector<int > how_to_split;
		public:
			SHLightingMesh(const redips::Triangles* model, int sample_number, std::string save_to = "", redips::ShaderSource shaderSource = redips::ShaderSource()) : glMeshWrapper(model, shaderSource, WrapOption::_genMipmap_) {
				bindVaoAttribData(0, 1, 2);
				if (shaderSource.sourceType == redips::ShaderSource::SourceType::_null_) {
					char strbuf[512];
					sprintf_s(strbuf, "%s/OpenglWrappers/DemoMeshes/SHLighting_band_%d", _REDIPS_ROOT_PATH_, BAND_WIDTH);
					useShader(strbuf);
				}
				
				switch (BAND_WIDTH) {
					case 2: { how_to_split = std::vector<int >(3, 3); break; } //9 = 3 + 3 + 3;	 
					case 3: { how_to_split = std::vector<int >(4, 4); break; } //16 = 4 + 4 + 4 + 4;
					case 4: {												   //25 = 4 * 4 + 3 * 3;
						how_to_split = std::vector<int >(4, 4); 
						for (int i = 0; i < 3; ++i)	how_to_split.push_back(3);
						break;
					}
					case 5 : { how_to_split = std::vector<int >(9, 4); break; }   //36 = 4 * 9;
					case 6 : {													   //49 = 4 * 10 + 3 * 3;
						how_to_split = std::vector<int >(10, 4); 
						for (int i = 0; i < 3; ++i) 
							how_to_split.push_back(3);
						break;
					}
				}

				if (BAND_WIDTH < 2 || BAND_WIDTH > 6) {
					std::cerr << "[SHLightingMesh] : sorry, SHLightingMesh currently only support BAND_WIDTH in range [2,6]" << std::endl;
				}
				else {
					computeAndTransferShs(sample_number, save_to);
				}
			};
			
			SHLightingMesh(const redips::Triangles* model, std::string shfile, redips::ShaderSource shaderSource = redips::ShaderSource()) : glMeshWrapper(model, shaderSource, WrapOption::_genMipmap_) {
				bindVaoAttribData(0, 1, 2);
				if (shaderSource.sourceType == redips::ShaderSource::SourceType::_null_) {
					char strbuf[512];
					sprintf_s(strbuf, "%s/OpenglWrappers/DemoMeshes/SHLighting_band_%d", _REDIPS_ROOT_PATH_, BAND_WIDTH);
					useShader(strbuf);
				}

				switch (BAND_WIDTH) {
				case 2: { how_to_split = std::vector<int >(3, 3); break; } //9 = 3 + 3 + 3;	 
				case 3: { how_to_split = std::vector<int >(4, 4); break; } //16 = 4 + 4 + 4 + 4;
				case 4: {												   //25 = 4 * 4 + 3 * 3;
					how_to_split = std::vector<int >(4, 4);
					for (int i = 0; i < 3; ++i)	how_to_split.push_back(3);
					break;
				}
				case 5: { how_to_split = std::vector<int >(9, 4); break; }   //36 = 4 * 9;
				case 6: {													   //49 = 4 * 10 + 3 * 3;
					how_to_split = std::vector<int >(10, 4);
					for (int i = 0; i < 3; ++i)
						how_to_split.push_back(3);
					break;
				}
				}

				if (BAND_WIDTH < 2 || BAND_WIDTH > 6) {
					std::cerr << "[SHLightingMesh] : sorry, SHLightingMesh currently only support BAND_WIDTH in range [2,6]" << std::endl;
				}
				else {
					loadShsFromDisk(shfile + std::to_string(DIM));
				}
			}

			SHLightingMesh(const glMeshWrapper& another, redips::ShaderSource shaderSource = redips::ShaderSource()) = delete;
			SHLightingMesh(const SHLightingMesh&) = delete;
			
			~SHLightingMesh() { 
				glDeleteBuffers(mesh->groups.size(), vbos_4_shcoefs_gpu); 
			}
			
			void littedBy(const VecXd<float, DIM>& light_r, const VecXd<float, DIM>& light_g, const VecXd<float, DIM>& light_b) {
				
				for (int i = 0, offset = 0; i < how_to_split.size(); offset += how_to_split[i], ++i) {
					if (how_to_split[i] == 3) {
						uniformFloat3((std::string("light_r_") + std::to_string(i)).c_str(), *((float3*)(&light_r[offset])));
						uniformFloat3((std::string("light_g_") + std::to_string(i)).c_str(), *((float3*)(&light_g[offset])));
						uniformFloat3((std::string("light_b_") + std::to_string(i)).c_str(), *((float3*)(&light_b[offset])));
					}
					else {
						uniformFloat4((std::string("light_r_") + std::to_string(i)).c_str(), *((float4*)(&light_r[offset])));
						uniformFloat4((std::string("light_g_") + std::to_string(i)).c_str(), *((float4*)(&light_g[offset])));
						uniformFloat4((std::string("light_b_") + std::to_string(i)).c_str(), *((float4*)(&light_b[offset])));
					}
				}

			}

			void littedBy(const VecXd<float, DIM>& light) {
				for (int i = 0, offset = 0; i < how_to_split.size(); offset += how_to_split[i], ++i) {
					if (how_to_split[i] == 3) {
						uniformFloat3((std::string("light_") + std::to_string(i)).c_str(), *((float3*)(&light[offset])));
					}
					else {
						uniformFloat4((std::string("light_") + std::to_string(i)).c_str(), *((float4*)(&light[offset])));
					}
				}
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
			
			void computeAndTransferShs(int sample_number, std::string save_to = "") {
				SamplesOnUnitSphere<BAND_WIDTH > samples(sample_number);
				std::cout << "[SHLightingMesh]: computing SHs per-vertex ... " << std::endl;

				//allocate memory
				vbos_4_shcoefs_gpu = new GLuint[mesh->groups.size()]; {
					glGenBuffers(mesh->groups.size(), vbos_4_shcoefs_gpu);
					int maxFaceCnt = 0; {
						for (auto& g : mesh->groups) maxFaceCnt = std::max(maxFaceCnt, g.faceCnt);
						_RUNTIME_ASSERT_(maxFaceCnt > 0, "assert [maxFaceCnt>0] failed");

						buff_4_shcoefs_cpu = new redips::VecXd<float, DIM>[maxFaceCnt * 3];
					}
				}

				//build-tree, accelerte intersection calculation
				const_cast<Triangles*>(model_ptr())->buildTree();

				//in case need to save to disk
				bool need_save = false; std::ofstream fout;
				if (save_to.length() > 0) {
					fout.open(save_to + std::to_string(DIM));
					need_save = true;
				}

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

					if (need_save) {
						for (int j = 0; j < faceCnt * 3; ++j) {
							for (int k = 0; k < 49; ++k) {
								fout << buff_4_shcoefs_cpu[j][k] << " ";
							}
							fout << std::endl;
						}
					}

					//copy to gpu and bind to vertex
					glBindVertexArray(vaos[i]);
					glBindBuffer(GL_ARRAY_BUFFER, vbos_4_shcoefs_gpu[i]);
					glBufferData(GL_ARRAY_BUFFER, sizeof(float) * DIM * faceCnt * 3, buff_4_shcoefs_cpu, GL_STATIC_DRAW);

					for (int j = 0, offset = 0; j < how_to_split.size(); offset += how_to_split[j], ++j) {
						glVertexAttribPointer(3 + j, how_to_split[j], GL_FLOAT, GL_FALSE, sizeof(float) * DIM, (const void*)(sizeof(float) * offset));
						glEnableVertexAttribArray(3 + j);
					}
				}

				delete buff_4_shcoefs_cpu;
				if (need_save) fout.close();

				std::cout << "[SHLightingMesh]: finish" << std::endl;
			}

			void loadShsFromDisk(std::string from) {
				std::ifstream fin(from);
				if (!fin.is_open()) {
					std::cerr << "[SHLightingMesh] : cannot open file " << from << std::endl;
					return;
				}

				//allocate memory
				vbos_4_shcoefs_gpu = new GLuint[mesh->groups.size()]; {
					glGenBuffers(mesh->groups.size(), vbos_4_shcoefs_gpu);
					int maxFaceCnt = 0; {
						for (auto& g : mesh->groups) maxFaceCnt = std::max(maxFaceCnt, g.faceCnt);
						_RUNTIME_ASSERT_(maxFaceCnt > 0, "assert [maxFaceCnt>0] failed");

						buff_4_shcoefs_cpu = new redips::VecXd<float, DIM>[maxFaceCnt * 3];
					}
				}

				for (int i = 0; i < mesh->groups.size(); ++i) {
					auto& g = mesh->groups[i];
					if (g.faceCnt <= 0) continue;
					int faceCnt = g.faceCnt;
					for (int j = 0; j < faceCnt * 3; ++j) {
						for (int k = 0; k < DIM; ++k) {
							fin >> buff_4_shcoefs_cpu[j][k];
						}
					}

					//copy to gpu and bind to vertex
					glBindVertexArray(vaos[i]);
					glBindBuffer(GL_ARRAY_BUFFER, vbos_4_shcoefs_gpu[i]);
					glBufferData(GL_ARRAY_BUFFER, sizeof(float) * DIM * faceCnt * 3, buff_4_shcoefs_cpu, GL_STATIC_DRAW);

					for (int j = 0, offset = 0; j < how_to_split.size(); offset += how_to_split[j], ++j) {
						glVertexAttribPointer(3 + j, how_to_split[j], GL_FLOAT, GL_FALSE, sizeof(float) * DIM, (const void*)(sizeof(float) * offset));
						glEnableVertexAttribArray(3 + j);
					}
				}
			}

			void CalculateSHCoefficients(const float3& vertice, const float3& normal, const SamplesOnUnitSphere<BAND_WIDTH>& samples, redips::VecXd<float, DIM>& result) {
				VecXd<double, DIM> tmp;
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

				for (int i = 0; i < DIM; ++i) result[i] = tmp[i];
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
