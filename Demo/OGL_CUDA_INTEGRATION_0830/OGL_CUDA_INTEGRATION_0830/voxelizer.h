#pragma once
#include <geos/triangles.h>
#include <ogl/shader.h>
#include <ogl/glTexture.h>
#include <ogl/glMeshWrapper.h>
#include <assert.h>

#define VOXEL_PROJECTION_VIEWPORT_SIZE 1024

extern "C" unsigned int compact_cuda(GLuint invbo, int precision);
extern "C" unsigned int onedVoxelization_cuda(GLuint fragsvbo, unsigned int fragscnt, redips::float3 fragscenter, redips::float3 boxdim,const float* mats,const float* axises, unsigned int precision, GLuint resultvbo);
extern "C" void mdVoxelization_cuda(GLuint fragsvbo, unsigned int fragscnt, redips::float3 fragscenter, int rotx, int roty, const float* mats, unsigned int presicion, std::string outputdir);

class Voxelizer : public redips::glMeshWrapper{
public:
	Voxelizer(const redips::Triangles* model, bool setup_type = true) :glMeshWrapper(model, setup_type){
		bindVaoAttribData(0, -1, -1);
		mbox = this->m()->aabb();
	};
	Voxelizer(const redips::glMeshWrapper& another) :glMeshWrapper(another){
		bindVaoAttribData(0, -1, -1);
		mbox = this->m()->aabb();
	}
	void draw(redips::Shader& shader){
		drawAllMeshes();
	}
	~Voxelizer(){
		if (count_ssbo) glDeleteBuffers(1, &count_ssbo);
		if (frags_ssbo) glDeleteBuffers(1,&frags_ssbo);
		if (voxelvbo) glDeleteBuffers(1,&voxelvbo);
	};
	GLuint count_ssbo = 0;  //counter
	GLuint frags_ssbo = 0;   //fragments
	GLuint voxelvbo = 0;     //show voxels;
	unsigned int PRECESION;
	unsigned int TOTAL_FRAG_CNT = 0;
	unsigned int VOXEL_CNT = 0;
public:
	/**************************************
	first pass; a.count b.compact
	second pass; save frags
	***************************************/
	void saveFragments(redips::Shader* shader, unsigned int precision){
		using namespace redips;
		//glTexture counter;
		//counter.create3d(int3(resolution,resolution,resolution/32),GL_R32UI,GL_RED_INTEGER,GL_UNSIGNED_INT,nullptr);
		//use ssbo instead of texture
		assert(precision < 11);
		PRECESION = precision = MIN(precision, 10u);
		if (PRECESION < 5) { puts("[voxelizer] : !warnning, resolution should not lower than 5(32)"); return; };
		shader->Use();

		int bufferBytes = 1u << (precision * 3 - 3);
		{   //generate ssbo and bind
			glGenBuffers(1, &count_ssbo);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, count_ssbo);
			glBufferData(GL_SHADER_STORAGE_BUFFER, bufferBytes, nullptr, GL_DYNAMIC_COPY);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

			GLuint blockIndex = glGetProgramResourceIndex(shader->Program, GL_SHADER_STORAGE_BLOCK, "counter");
			glShaderStorageBlockBinding(shader->Program, blockIndex, 0);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, count_ssbo);
		}
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glUniform1ui(glGetUniformLocation(shader->Program, "precision"), precision);
		glUniformMatrix4fv(glGetUniformLocation(shader->Program, "model"), 1, GL_FALSE, ((this->m())->Transform()).transpose().ptr());
		{   //first pass, render 3 times
			//GLint orivp[4];
			//glGetIntegerv(GL_VIEWPORT, orivp);
			//glViewport(0, 0, VOXEL_PROJECTION_VIEWPORT_SIZE, VOXEL_PROJECTION_VIEWPORT_SIZE);
			//glViewport(0, 0, orivp[2] * 2, orivp[3]*2);
			glUniform1ui(glGetUniformLocation(shader->Program, "pass"), 0u);

			glUniformMatrix4fv(glGetUniformLocation(shader->Program, "projection"), 1, GL_FALSE, (GeoUtil::glOrtho(mbox)).transpose().ptr());
			glUniformMatrix3fv(glGetUniformLocation(shader->Program, "swizzler"), 1, GL_FALSE, Mat33f::xyz().transpose().ptr());
			this->draw(*shader);

			glUniformMatrix4fv(glGetUniformLocation(shader->Program, "projection"), 1, GL_FALSE, (GeoUtil::glOrtho(mbox.back(), mbox.front(), mbox.left(), mbox.right(), mbox.bottom(), mbox.top())).transpose().ptr());
			glUniformMatrix3fv(glGetUniformLocation(shader->Program, "swizzler"), 1, GL_FALSE, Mat33f::zxy().transpose().ptr());
			this->draw(*shader);

			glUniformMatrix4fv(glGetUniformLocation(shader->Program, "projection"), 1, GL_FALSE, (GeoUtil::glOrtho(mbox.bottom(), mbox.top(), mbox.back(), mbox.front(), mbox.left(), mbox.right())).transpose().ptr());
			glUniformMatrix3fv(glGetUniformLocation(shader->Program, "swizzler"), 1, GL_FALSE, Mat33f::yzx().transpose().ptr());
			this->draw(*shader);
			//glViewport(orivp[0], orivp[1], orivp[2], orivp[3]);
		}
		TOTAL_FRAG_CNT = compact_cuda(count_ssbo, precision);   //compact, for allocate memory&calculate
		{   //generate ssbo and bind
			glGenBuffers(1, &frags_ssbo);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, frags_ssbo);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * 4 * TOTAL_FRAG_CNT, nullptr, GL_DYNAMIC_COPY);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

			GLuint blockIndex = glGetProgramResourceIndex(shader->Program, GL_SHADER_STORAGE_BLOCK, "fragments");
			glShaderStorageBlockBinding(shader->Program, blockIndex, 1);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, frags_ssbo);
		}
		//second pass, render 3 times
		{
			glUniform1ui(glGetUniformLocation(shader->Program, "pass"), 1u);

			glUniformMatrix4fv(glGetUniformLocation(shader->Program, "projection"), 1, GL_FALSE, (GeoUtil::glOrtho(mbox)).transpose().ptr());
			glUniformMatrix3fv(glGetUniformLocation(shader->Program, "swizzler"), 1, GL_FALSE, Mat33f::xyz().transpose().ptr());
			this->draw(*shader);

			glUniformMatrix4fv(glGetUniformLocation(shader->Program, "projection"), 1, GL_FALSE, (GeoUtil::glOrtho(mbox.back(), mbox.front(), mbox.left(), mbox.right(), mbox.bottom(), mbox.top())).transpose().ptr());
			glUniformMatrix3fv(glGetUniformLocation(shader->Program, "swizzler"), 1, GL_FALSE, Mat33f::zxy().transpose().ptr());
			this->draw(*shader);

			glUniformMatrix4fv(glGetUniformLocation(shader->Program, "projection"), 1, GL_FALSE, (GeoUtil::glOrtho(mbox.bottom(), mbox.top(), mbox.back(), mbox.front(), mbox.left(), mbox.right())).transpose().ptr());
			glUniformMatrix3fv(glGetUniformLocation(shader->Program, "swizzler"), 1, GL_FALSE, Mat33f::yzx().transpose().ptr());
			this->draw(*shader);
		}
		glEnable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);

		printf("[voxelizer] : save fragments finish, total %d fragments\n", TOTAL_FRAG_CNT); inited = true;

		//release buffer
		glDeleteBuffers(1, &count_ssbo); count_ssbo = 0;
	}

	void onedVoxelization(int x,int y,int rotx,int roty,int precision){
		if (!inited) return;
		if (!voxelvbo){
			glGenBuffers(1,&voxelvbo);
			glBindBuffer(GL_ARRAY_BUFFER,voxelvbo);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * (1u<<(precision*3)),nullptr,GL_STATIC_DRAW);
			glBindBuffer(GL_ARRAY_BUFFER,0);
		}
		BOX tbox;
		float3 DIM = mbox.dim();
		Mat33f rmat = Mat33f::tilt(RAD(x*180.0f / rotx))*Mat33f::pan(RAD(y*180.0f / roty));
		for (unsigned int i = 0; i < 8; i++){ tbox += (rmat * (DIM*-0.5f + (DIM*float3::bits(i)))); }
		float3 dim = tbox.dim();
		Mat44f pmat = GeoUtil::glOrtho(dim.x*-0.5f, dim.x*0.5f, dim.y*-0.5f, dim.y*0.5f, dim.z*-0.5f, dim.z*0.5f)*Mat44f(rmat);

		VOXEL_CNT = onedVoxelization_cuda(frags_ssbo, TOTAL_FRAG_CNT, mbox.heart(), dim, pmat.ptr(), rmat.ptr(), precision, voxelvbo);
		printf("[voxelizer] : oned voxelization finish, total %d voxels\n", VOXEL_CNT);
	}

	void multidVoxelization(int rotx, int roty, int precision, std::string outputdir){
		if (!inited) return;
		using namespace redips;
		assert(frags_ssbo);
		assert(precision <= PRECESION);

		BOX tbox;
		float3 DIM = mbox.dim();
		Mat44f *mats = new Mat44f[roty*rotx];
		for (int y = 0; y < roty; y++) for (int x = 0; x < rotx; x++) {
			Mat33f rotate = Mat33f::tilt(RAD(x*180.0f / rotx))*Mat33f::pan(RAD(y*180.0f / roty));
			for (unsigned int i = 0; i < 8; i++){ tbox += (rotate * (DIM*-0.5f + (DIM*float3::bits(i)))); }
			float3 dim = tbox.dim();
			mats[y*rotx + x] = GeoUtil::glOrtho(dim.x*-0.5f, dim.x*0.5f, dim.y*-0.5f, dim.y*0.5f, dim.z*-0.5f, dim.z*0.5f)*Mat44f(rotate);
			tbox.reset();
		}
		mdVoxelization_cuda(frags_ssbo, TOTAL_FRAG_CNT, mbox.heart(), rotx, roty, mats[0].ptr(), precision, outputdir);
	}
private:
	redips::BOX mbox;
	bool inited = false;
};
