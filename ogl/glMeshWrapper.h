#pragma once
#include "../geos/triangles.h"
#include <GL/glew.h>
#include "shader.h"

class glMeshWrapper{
public:
	glMeshWrapper(const Triangles* model){
		int SIZE_PER_VERT[] = { 0,  3 /*_single_*/, 3 + 3/*_withnormal_*/, 3 + 2 + 3/*_withtex_*/ };
		this->model = model;
		this->mesh = model->mesh;
		//allocate memory , vbo
		meshCnt = mesh->groups.size();
		vbos = new GLuint[meshCnt];
		vaos = new GLuint[meshCnt];
		int maxFaceCnt = mesh->groups[0].faceCnt;  for (int i = 1; i < meshCnt; i++) maxFaceCnt = MAX(maxFaceCnt, mesh->groups[i].faceCnt);
		vertexBufferData = new float[maxFaceCnt*3*8];
		glGenBuffers(meshCnt,vbos);
		//copy data
		const std::vector<float3>& vertices = mesh->vertices;
		const std::vector<float3>& texcoords = mesh->texcoords;
		const std::vector<float3>& normals = mesh->normals;

		for (int i = 0; i < meshCnt; i++){
			int faceCnt = mesh->groups[i].faceCnt;
			//copy vertices-coords, type is float3
			float3 *ptrf3 = (float3*)(vertexBufferData);
			int fid = mesh->groups[i].fsid;
			for (int feid = fid + faceCnt; fid < feid; fid++){
				  const int3& indices = mesh->faces_v[fid];
				  (*ptrf3++) = vertices[indices.x];
				  (*ptrf3++) = vertices[indices.y];
				  (*ptrf3++) = vertices[indices.z];
			}
			//copy tex-coords if exists, type float2
			if (mesh->groups[i].faceType == _withtex_){
				float2 *ptrf2 = (float2*)(vertexBufferData + (faceCnt * 3 * 3));
				int tid = mesh->groups[i].tsid;
				for (int teid = tid + faceCnt; tid < teid; tid++) {
					const int3& indices = mesh->faces_vt[tid];
					(*ptrf2++) = (texcoords[indices.x].vec2());
					(*ptrf2++) = (texcoords[indices.y].vec2());
					(*ptrf2++) = (texcoords[indices.z].vec2());
				}
			}
			//copy normals if exists, type float3
			if (mesh->groups[i].faceType >= _withnormal_){
				float3* ptrf3 = (float3*)(vertexBufferData + (faceCnt * 3 * (mesh->groups[i].faceType == _withtex_ ? 5 : 3)));
				int nid = mesh->groups[i].nsid;
				for (int neid = nid + faceCnt; nid < neid; nid++){
					const int3& indices = mesh->faces_vn[nid];
					(*ptrf3++) = normals[indices.x];
					(*ptrf3++) = normals[indices.y];
					(*ptrf3++) = normals[indices.z];
				}
			}
			//copy data to gpu
			glBindBuffer(GL_ARRAY_BUFFER,vbos[i]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * SIZE_PER_VERT[mesh->groups[i].faceType] * faceCnt * 3, vertexBufferData, GL_STATIC_DRAW);
			glBindBuffer(GL_ARRAY_BUFFER,0);
			std::cout << "[ glMeshWrapper ] : " << mesh->groups[i].name << " transfered" << std::endl;
		}
		glGenVertexArrays(meshCnt,vaos);
		for (int i = 0; i < meshCnt; i++){
			glBindVertexArray(vaos[i]);
			glBindBuffer(GL_ARRAY_BUFFER, vbos[i]);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
			if (mesh->groups[i].faceType == _withtex_){
				glEnableVertexAttribArray(_GL_SHADER_VERTEXATTRIB_TEXCOORD_LOCATION_);
				glVertexAttribPointer(_GL_SHADER_VERTEXATTRIB_TEXCOORD_LOCATION_, 2, GL_FLOAT, GL_FALSE, 0, (const GLvoid*)(sizeof(float)*(mesh->groups[i].faceCnt * 3 * 3)));
				glDisableVertexArrayAttrib(vaos[i], _GL_SHADER_VERTEXATTRIB_TEXCOORD_LOCATION_);
			}
			if (mesh->groups[i].faceType >= _withnormal_){
				glEnableVertexAttribArray(_GL_SHADER_VERTEXATTRIB_NORMAL_LOCATION_);
				glVertexAttribPointer(_GL_SHADER_VERTEXATTRIB_NORMAL_LOCATION_, 3, GL_FLOAT, GL_FALSE, 0, (const GLvoid*)(sizeof(float)*(mesh->groups[i].faceCnt * 3 * (mesh->groups[i].faceType == _withtex_ ? 5 : 3))));
				glDisableVertexArrayAttrib(vaos[i], _GL_SHADER_VERTEXATTRIB_NORMAL_LOCATION_);
			}
		}
		glBindVertexArray(0);
		std::cout << "[ glMeshWrapper ] : copy mesh to gpu finish" << std::endl;
		delete vertexBufferData;
	}
	~glMeshWrapper(){ }
public:
	GLuint *vbos,*vaos;
	int meshCnt = 0;
	float* vertexBufferData;
	const Mesh* mesh;
	const Triangles* model;

	void draw(bool usenormal = false, bool usetexcoord = false) const {
		for (int i = 0; i < meshCnt; i++){
			glBindVertexArray(vaos[i]);
			if (usenormal&&mesh->groups[i].faceType >= _withnormal_)  glEnableVertexAttribArray(_GL_SHADER_VERTEXATTRIB_NORMAL_LOCATION_);
			if (usetexcoord&&mesh->groups[i].faceType == _withtex_) glEnableVertexAttribArray(_GL_SHADER_VERTEXATTRIB_TEXCOORD_LOCATION_);
			glDrawArrays(GL_TRIANGLES, 0, 3 * mesh->groups[i].faceCnt);
			if (usenormal&&mesh->groups[i].faceType >= _withnormal_) glDisableVertexAttribArray(_GL_SHADER_VERTEXATTRIB_NORMAL_LOCATION_);
			if (usetexcoord&&mesh->groups[i].faceType == _withtex_) glDisableVertexAttribArray(_GL_SHADER_VERTEXATTRIB_TEXCOORD_LOCATION_);
		}
		glBindVertexArray(0);
	}
};
