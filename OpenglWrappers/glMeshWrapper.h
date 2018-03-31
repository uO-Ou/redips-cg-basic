/*
* Author : redips redips.xin@gmail.com
* Date : 2017.12.15
* Description : opengl mesh wrapper
		a. if option& 1 == 1, setup per mtl, else per group; 
		   if option & 2 == 1, generate geometry normal, else use normal from obj file
		   if option & 4 == 1, generate mipmap texture
		b. when delete a glMeshWrapper, release vbo/textures if only its' visitorCnt<=0 

bug: row 63,what if a .mtl has noused mtl
*/
#pragma once
#include "glTextureWrapper.h"
#include "glslShaderWrapper.h"
#include "../Geometry/triangles.h"

namespace redips{
	class glMeshWrapper{
	public:
		class WrapOption{ 
		public:
			static const unsigned int _default_ = 1u;
			static const unsigned int _geNormal_ = 2u;
			static const unsigned int _genMipmap_ = 4u;
			WrapOption(){ value = _default_; }
			WrapOption(unsigned int value) :value(value){};
		private:
			unsigned int value;
			operator unsigned int(){ return value; };
		};
	protected:
		glMeshWrapper(const Triangles* model, ShaderSource& shaderSource, unsigned int option = WrapOption::_default_ | WrapOption::_genMipmap_){
			this->origion = this;
			this->visitorCnt = 1;
			useShader(shaderSource);

			this->model = model;
			this->mesh = model->mesh_ptr();
			meshFaceCnt.resize(mesh->groups.size());
			meshFaceTypes.resize(mesh->groups.size());
			setup_type = option & WrapOption::_default_;
			bool genGeoNormal = (option & WrapOption::_geNormal_);
			bool genMipmapTexture = (option & WrapOption::_genMipmap_);

			// check if groups with same mtl have same face-type
			if (setup_type){
				for (int i = 0; i < mesh->mtllib.size(); i++) {
					meshFaceCnt[i] = 0;
					meshFaceTypes[i] = GROUP_FACE_TYPE::_unknown_face_type_;
				}
				for (int i = 0; i < mesh->groups.size(); i++) {
					int mid = mesh->groups[i].mtlId;
					meshFaceCnt[mid] += mesh->groups[i].faceCnt;
					if (meshFaceTypes[mid] == GROUP_FACE_TYPE::_unknown_face_type_) meshFaceTypes[mid] = mesh->groups[i].faceType;
					else{
						if (meshFaceTypes[mid] != mesh->groups[i].faceType){
							printf("[glMeshWrapper] : warning! groups with same material have different face type, setup_type set to false\n");
							setup_type = false; break;
						}
					}
				}
			}

			//initialize meshMtls | meshFaceCnt | meshFaceType, calculate max meshFaceCnt
			int maxFaceCnt = 0;
			if (setup_type){
				this->meshCnt = mesh->mtllib.size();
				meshMtls.resize(meshCnt);
				for (int i = 0; i < meshCnt; i++) {
					meshMtls[i] = mesh->mtllib[i];
					maxFaceCnt = MAX(maxFaceCnt, meshFaceCnt[i]);
				}
			}
			else{
				this->meshCnt = mesh->groups.size();
				meshMtls.resize(meshCnt);
				for (int i = 0; i < meshCnt; i++) {
					meshMtls[i] = (mesh->mtllib)[mesh->groups[i].mtlId];
					meshFaceCnt[i] = mesh->groups[i].faceCnt;
					meshFaceTypes[i] = mesh->groups[i].faceType;
					maxFaceCnt = MAX(maxFaceCnt, meshFaceCnt[i]);
				}
			}
			vertexBuffer = new float[maxFaceCnt * 3 * 3];
			texcoordBuffer = new float[maxFaceCnt * 3 * 2];
			normalBuffer = new float[maxFaceCnt * 3 * 3];

			//generate vaos/vbos, copy data to gpu
			vaos = new GLuint[meshCnt];
			glGenVertexArrays(meshCnt,vaos);

			vbos = new GLuint[meshCnt];
			glGenBuffers(meshCnt, vbos);

			if (setup_type) setup_permtl(genGeoNormal);
			else setup_pergroup(genGeoNormal);

			//generate textures
			textureCnt = mesh->mtllib.loadedImage.size();
			if (textureCnt>0){
				textures = new glTexture[textureCnt];
				int texId = 0;
				for (auto iter = mesh->mtllib.loadedImage.begin(); iter != mesh->mtllib.loadedImage.end(); iter++){
					textures[texId].create2d(iter->second, genMipmapTexture);
					mtlTextureHandle[iter->second] = textures[texId++];
				}
			}

			if (vertexBuffer) delete[]vertexBuffer;
			if (texcoordBuffer) delete[]texcoordBuffer;
			if (normalBuffer) delete[]normalBuffer;
		}
		glMeshWrapper(const glMeshWrapper& another, ShaderSource& shaderSource){
			this->origion = another.origion;
			this->origion->visitorCnt++;
			useShader(shaderSource);

			this->model = another.model;
			this->mesh = model->mesh_ptr();
			this->setup_type = another.setup_type;
			this->meshCnt = another.meshCnt;
			this->textureCnt = 0;

			meshMtls.resize(meshCnt);
			meshFaceCnt.resize(meshCnt);
			meshFaceTypes.resize(meshCnt);
			std::copy(another.meshMtls.begin(), another.meshMtls.begin() + meshCnt, this->meshMtls.begin());
			std::copy(another.meshFaceCnt.begin(), another.meshFaceCnt.begin() + meshCnt, this->meshFaceCnt.begin());
			std::copy(another.meshFaceTypes.begin(), another.meshFaceTypes.begin() + meshCnt, this->meshFaceTypes.begin());
			for (auto iter = another.mtlTextureHandle.begin(); iter != another.mtlTextureHandle.end(); iter++){
				this->mtlTextureHandle[iter->first] = iter->second;
			}

			vbos = new GLuint[meshCnt];
			memcpy(vbos, another.vbos, sizeof(GLuint)*meshCnt);

			vaos = new GLuint[meshCnt];
			glGenVertexArrays(meshCnt, vaos);
		}
		virtual ~glMeshWrapper(){
			if ((--(this->origion->visitorCnt)) <= 0){
				this->origion->releaseBuffer();
			}
			meshMtls.clear();
			meshFaceCnt.clear();
			meshFaceTypes.clear();
			mtlTextureHandle.clear();

			if (meshCnt) {
				glDeleteVertexArrays(meshCnt, vaos);
				delete[] vaos;
			}
			if (vbos) delete[] vbos;
		}
		void bindVaoAttribData(GLint positionLoc = 0, GLint normalLoc = 1, GLint texcoordLoc = 2){
			for (int i = 0; i < meshCnt; i++){
				if (meshFaceCnt[i] <= 0) continue;
				glBindVertexArray(vaos[i]);
				glBindBuffer(GL_ARRAY_BUFFER, vbos[i]);
				glVertexAttribPointer(positionLoc, 3, GL_FLOAT, GL_FALSE, 0, NULL);
				glEnableVertexAttribArray(positionLoc);
				if (meshFaceTypes[i] >= GROUP_FACE_TYPE::_withnormal_&&normalLoc != -1){
					glVertexAttribPointer(normalLoc, 3, GL_FLOAT, GL_FALSE, 0, (const GLvoid*)(sizeof(float)*(meshFaceCnt[i] * 3 * 3)));
					glEnableVertexAttribArray(normalLoc);
				}
				if (meshFaceTypes[i] == GROUP_FACE_TYPE::_withtex_&&texcoordLoc != -1){
					glVertexAttribPointer(texcoordLoc, 2, GL_FLOAT, GL_FALSE, 0, (const GLvoid*)(sizeof(float)*(meshFaceCnt[i] * 3 * 6)));
					glEnableVertexAttribArray(texcoordLoc);
				}
			}
			glBindVertexArray(0);
			this->isVaoBinded = true;
		}

	public:
		GLuint getShader(){ return m_shader->Program; };
		void useShader(const ShaderSource& shaderSource){
			if (shaderSource.sourceType == ShaderSource::SourceType::_from_file_){
				if (m_shader) delete m_shader;
				m_shader = new Shader(shaderSource.vertexShaderPath().c_str(), shaderSource.fragmentShaderPath().c_str());
			}
			else if (shaderSource.sourceType == ShaderSource::SourceType::_exists_program_){
				if (m_shader) delete m_shader;
				m_shader = new Shader(shaderSource.value.program);
			}
		}
		void releaseBuffer(){
			if (meshCnt){
				glDeleteBuffers(meshCnt, vbos);
			}
			if (textureCnt){
				for (int i = 0; i < textureCnt; i++) textures[i].destroy();
				delete[] textures;
			}
		}
		void drawAllMeshes(){
			for (int i = 0; i < meshCnt; i++){
				glBindVertexArray(vaos[i]);
				glDrawArrays(GL_TRIANGLES, 0, meshFaceCnt[i] * 3);
			}
		}
		virtual void draw() {
			if (!m_shader) { 
				std::cerr << "shader error" << std::endl; return; 
			};
			m_shader->Use();
			drawAllMeshes();
		};

		const Triangles* model_ptr()  const{ return this->model; }

		//shader uniforms
	    void uniformMat44f(const char* name,const float* value){ 
			m_shader->Use();
			glUniformMatrix4fv(glGetUniformLocation(m_shader->Program, name), 1, GL_FALSE, value); 
		}
		void uniformFloat3(const char* name,const redips::float3& value){
			m_shader->Use();
			glUniform3f(glGetUniformLocation(m_shader->Program, name), value.x, value.y, value.z);
		}
		void uniformInt1(const char* name, int value){ 
			m_shader->Use();
			glUniform1i(glGetUniformLocation(m_shader->Program, name), value); 
		};
	protected:
		int meshCnt = 0;
		int textureCnt = 0;
		bool isVaoBinded = false;
		glTexture *textures = nullptr;
		GLuint *vbos = nullptr, *vaos = nullptr;

		bool setup_type;
		const Mesh* mesh;
		const Triangles* model;

		std::vector<int> meshFaceCnt;
		std::vector<const Material*> meshMtls;
		std::vector<GROUP_FACE_TYPE> meshFaceTypes;
		//mtl->image, texture-id map
		std::map<FImage*, GLuint> mtlTextureHandle;

		Shader* m_shader = nullptr;
	private:
		void setup_pergroup(bool genGeometryNormal){
			int SIZE_PER_VERT[] = { 0, 3 /*_single_*/, 3 + 3/*_withnormal_*/, 3 + 2 + 3/*_withtex_*/ };
			//copy data
			const std::vector<float3>& vertices = mesh->vertices;
			const std::vector<float3>& normals = mesh->normals;
			const std::vector<float3>& texcoords = mesh->texcoords;
			for (int i = 0; i < meshCnt; i++){
				//copy vertices-coords, type is float3
				int faceCnt = meshFaceCnt[i];
				if (faceCnt <= 0) continue;
				float3 *ptrf3 = (float3*)(vertexBuffer);
				int fid = mesh->groups[i].fsid;
				for (int feid = fid + faceCnt; fid < feid; fid++){
					const int3& indices = mesh->faces_v[fid];
					(*ptrf3++) = vertices[indices.x];
					(*ptrf3++) = vertices[indices.y];
					(*ptrf3++) = vertices[indices.z];
				}
				//copy normals if exists, type float3
				if (meshFaceTypes[i] >= GROUP_FACE_TYPE::_withnormal_ && genGeometryNormal == false){
					float3* ptrf3 = (float3*)(normalBuffer);
					int nid = mesh->groups[i].nsid;
					for (int neid = nid + faceCnt; nid < neid; nid++){
						const int3& indices = mesh->faces_vn[nid];
						(*ptrf3++) = normals[indices.x];
						(*ptrf3++) = normals[indices.y];
						(*ptrf3++) = normals[indices.z];
					}
				}
				//generate geometry normal
				else if (genGeometryNormal){
					float3* ptrf3 = (float3*)(normalBuffer);
					float3* vptrf3 = (float3*)(vertexBuffer);
					for (int vid = 0; vid < faceCnt; vid++){
						float3 edg1 = vptrf3[1] - vptrf3[0];
						float3 edg2 = vptrf3[2] - vptrf3[1];
						float3 norm = (edg1 ^ edg2).unit();
						ptrf3[0] = ptrf3[1] = ptrf3[2] = norm;
						vptrf3 += 3; ptrf3 += 3;
					}
					if (meshFaceTypes[i] < GROUP_FACE_TYPE::_withnormal_) meshFaceTypes[i] = GROUP_FACE_TYPE::_withnormal_;
				}
				//copy tex-coords if exists, type float2
				if (meshFaceTypes[i] == GROUP_FACE_TYPE::_withtex_){
					float2 *ptrf2 = (float2*)(texcoordBuffer);
					int tid = mesh->groups[i].tsid;
					for (int teid = tid + faceCnt; tid < teid; tid++) {
						const int3& indices = mesh->faces_vt[tid];
						(*ptrf2++) = (texcoords[indices.x].vec2());
						(*ptrf2++) = (texcoords[indices.y].vec2());
						(*ptrf2++) = (texcoords[indices.z].vec2());
					}
				}

				//copy data to gpu
				glBindBuffer(GL_ARRAY_BUFFER, vbos[i]);
				glBufferData(GL_ARRAY_BUFFER, faceCnt * 3 * SIZE_PER_VERT[int(meshFaceTypes[i])] * sizeof(float), NULL, GL_STATIC_DRAW);
				glBufferSubData(GL_ARRAY_BUFFER, 0, faceCnt * 3 * 3 * sizeof(float), vertexBuffer);
				if (meshFaceTypes[i] >= GROUP_FACE_TYPE::_withnormal_) glBufferSubData(GL_ARRAY_BUFFER, faceCnt * 3 * 3 * sizeof(float), faceCnt * 3 * 3 * sizeof(float), normalBuffer);
				if (meshFaceTypes[i] == GROUP_FACE_TYPE::_withtex_) glBufferSubData(GL_ARRAY_BUFFER, faceCnt * 3 * 6 * sizeof(float), faceCnt * 3 * 2 * sizeof(float), texcoordBuffer);
				glBindBuffer(GL_ARRAY_BUFFER, 0);
				//std::cout << "[glMeshWrapper] : " << mesh->groups[i].name << " transfered" << std::endl;
			}
			std::cout << "[glMeshWrapper] : copy mesh to gpu finish" << std::endl;
		}
		void setup_permtl(bool genGeometryNormal){
			int SIZE_PER_VERT[] = { 0, 3 /*_single_*/, 3 + 3/*_withnormal_*/, 3 + 2 + 3/*_withtex_*/ };
			//copy data
			const std::vector<float3>& vertices = mesh->vertices;
			const std::vector<float3>& normals = mesh->normals;
			const std::vector<float3>& texcoords = mesh->texcoords;

			for (int i = 0; i < meshCnt; i++){
				if (meshFaceCnt[i] <= 0) continue;
				GROUP_FACE_TYPE mtype = meshFaceTypes[i];
				float3 *vptr = (float3*)(vertexBuffer);
				float3 *nptr = (float3*)(normalBuffer);
				float2 *tptr = (float2*)(texcoordBuffer);

				for (int j = 0; j < mesh->groups.size(); j++){
					if (mesh->groups[j].mtlId != i) continue;
					int faceCnt = mesh->groups[j].faceCnt;
					if (faceCnt <= 0) continue;

					int fid = mesh->groups[j].fsid;
					for (int feid = fid + faceCnt; fid < feid; fid++){
						const int3& indices = mesh->faces_v[fid];
						(*vptr++) = vertices[indices.x];
						(*vptr++) = vertices[indices.y];
						(*vptr++) = vertices[indices.z];
					}
					//copy normals if exists, type float3
					if (mtype >= GROUP_FACE_TYPE::_withnormal_ && genGeometryNormal == false){
						int nid = mesh->groups[j].nsid;
						for (int neid = nid + faceCnt; nid < neid; nid++){
							const int3& indices = mesh->faces_vn[nid];
							(*nptr++) = normals[indices.x];
							(*nptr++) = normals[indices.y];
							(*nptr++) = normals[indices.z];
						}
					}
					//generate geometry normal
					else if (genGeometryNormal){
						float3* ptrf3 = (float3*)(normalBuffer);
						float3* vptrf3 = (float3*)(vertexBuffer);
						for (int vid = 0; vid < faceCnt; vid++){
							float3 edg1 = vptrf3[1] - vptrf3[0];
							float3 edg2 = vptrf3[2] - vptrf3[1];
							float3 norm = (edg1 ^ edg2).unit();
							ptrf3[0] = ptrf3[1] = ptrf3[2] = norm;
							vptrf3 += 3; ptrf3 += 3;
						}
						if (meshFaceTypes[i] < GROUP_FACE_TYPE::_withnormal_) meshFaceTypes[i] = GROUP_FACE_TYPE::_withnormal_;
					}
					//copy tex-coords if exists, type float2
					if (mtype == GROUP_FACE_TYPE::_withtex_){
						int tid = mesh->groups[j].tsid;
						for (int teid = tid + faceCnt; tid < teid; tid++) {
							const int3& indices = mesh->faces_vt[tid];
							(*tptr++) = (texcoords[indices.x].vec2());
							(*tptr++) = (texcoords[indices.y].vec2());
							(*tptr++) = (texcoords[indices.z].vec2());
						}
					}
				}
				//copy data to gpu
				glBindBuffer(GL_ARRAY_BUFFER, vbos[i]);
				glBufferData(GL_ARRAY_BUFFER, meshFaceCnt[i] * 3 * SIZE_PER_VERT[int(mtype)] * sizeof(float), NULL, GL_STATIC_DRAW);
				glBufferSubData(GL_ARRAY_BUFFER, 0, meshFaceCnt[i] * 3 * 3 * sizeof(float), vertexBuffer);
				if (meshFaceTypes[i] >= GROUP_FACE_TYPE::_withnormal_) glBufferSubData(GL_ARRAY_BUFFER, meshFaceCnt[i] * 3 * 3 * sizeof(float), meshFaceCnt[i] * 3 * 3 * sizeof(float), normalBuffer);
				if (meshFaceTypes[i] == GROUP_FACE_TYPE::_withtex_) glBufferSubData(GL_ARRAY_BUFFER, meshFaceCnt[i] * 3 * 6 * sizeof(float), meshFaceCnt[i] * 3 * 2 * sizeof(float), texcoordBuffer);
				glBindBuffer(GL_ARRAY_BUFFER, 0);
				//std::cout << "[glMeshWrapper] : " << mesh->mtllib[i]->name << " transfered" << std::endl;
			}
			std::cout << "[glMeshWrapper] : copy mesh to gpu finish" << std::endl;
		}
	private:
		//when visitor is less or equal 0 , release
		int visitorCnt;
		glMeshWrapper* origion = nullptr;

		//buffer cpu
		float* vertexBuffer = nullptr;
		float* normalBuffer = nullptr;
		float* texcoordBuffer = nullptr;
	};
};

