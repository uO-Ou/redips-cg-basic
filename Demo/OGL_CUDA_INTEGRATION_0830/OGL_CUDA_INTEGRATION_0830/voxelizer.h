#pragma once
#include <geos/triangles.h>
#include <ogl/shader.h>
#include <ogl/glTexture.h>
#include <assert.h>
#include <ogl/demoEffects/lightMesh.h>
#include "VoxelMesh.h"

#define VOXEL_PROJECTION_VIEWPORT_SIZE 1024

extern "C" GLuint generateVoxelDots(GLuint invbo, int precision, unsigned int &TOTAL_VOXEL_CNT, redips::float3 boxcenter, redips::float3 boxdim);
extern "C" void mdVoxelization(GLuint dotsvbo, int dotscnt, redips::float3 dotscenter, int rotx, int roty, const float* mats, unsigned int presicion, std::string outputdir);

class Voxelizer{
public:
	Voxelizer(){};
	~Voxelizer(){
		if (ssbo) glDeleteBuffers(1,&ssbo);
		if (voxelvbo) glDeleteBuffers(1,&voxelvbo);
	};
	GLuint ssbo = 0;
	GLuint voxelvbo = 0;
	unsigned int PRECESION;
	unsigned int TOTAL_VOXEL_CNT = 0;
public:
	void onedVoxelization(VoxelMesh* model, redips::Shader* shader, unsigned int precision){
		using namespace redips;
		//glTexture counter;
		//counter.create3d(int3(resolution,resolution,resolution/32),GL_R32UI,GL_RED_INTEGER,GL_UNSIGNED_INT,nullptr);
		//use ssbo instead of texture
		assert(precision < 11);
		PRECESION = precision = MIN(precision, 10u);

		mbox = model->m()->aabb();

		int bufferBytes = 1u << (precision * 3 - 3);
		glGenBuffers(1, &ssbo);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
		glBufferData(GL_SHADER_STORAGE_BUFFER, bufferBytes, nullptr, GL_DYNAMIC_COPY);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		shader->Use();
		GLuint blockIndex = glGetProgramResourceIndex(shader->Program, GL_SHADER_STORAGE_BLOCK, "counter");
		glShaderStorageBlockBinding(shader->Program, blockIndex, 0);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);

		//render 3 times
		{
			//GLint orivp[4];
			//glGetIntegerv(GL_VIEWPORT, orivp);
			//glViewport(0, 0, VOXEL_PROJECTION_VIEWPORT_SIZE, VOXEL_PROJECTION_VIEWPORT_SIZE);
			//glViewport(0, 0, orivp[2] * 2, orivp[3]*2);
			glDisable(GL_CULL_FACE);
			glDisable(GL_DEPTH_TEST);

			glUniform1ui(glGetUniformLocation(shader->Program, "pres"), precision);
			glUniformMatrix4fv(glGetUniformLocation(shader->Program, "model"), 1, GL_FALSE, ((model->m())->Transform()).transpose().ptr());

			glUniformMatrix4fv(glGetUniformLocation(shader->Program, "projection"), 1, GL_FALSE, (GeoUtil::glOrtho(mbox)).transpose().ptr());
			glUniformMatrix3fv(glGetUniformLocation(shader->Program, "swizzler"), 1, GL_FALSE, Mat33f::xyz().transpose().ptr());
			model->draw(*shader);

			glUniformMatrix4fv(glGetUniformLocation(shader->Program, "projection"), 1, GL_FALSE, (GeoUtil::glOrtho(mbox.back(), mbox.front(), mbox.left(), mbox.right(), mbox.bottom(), mbox.top())).transpose().ptr());
			glUniformMatrix3fv(glGetUniformLocation(shader->Program, "swizzler"), 1, GL_FALSE, Mat33f::zxy().transpose().ptr());
			model->draw(*shader);

			glUniformMatrix4fv(glGetUniformLocation(shader->Program, "projection"), 1, GL_FALSE, (GeoUtil::glOrtho(mbox.bottom(), mbox.top(), mbox.back(), mbox.front(), mbox.left(), mbox.right())).transpose().ptr());
			glUniformMatrix3fv(glGetUniformLocation(shader->Program, "swizzler"), 1, GL_FALSE, Mat33f::yzx().transpose().ptr());
			model->draw(*shader);

			glEnable(GL_DEPTH_TEST);
			glEnable(GL_CULL_FACE);
			//glViewport(orivp[0], orivp[1], orivp[2], orivp[3]);
		}

		voxelvbo = generateVoxelDots(ssbo, precision, TOTAL_VOXEL_CNT, mbox.heart(), mbox.dim());
		puts("[voxlization] : finish");

		//release buffer
		glDeleteBuffers(1, &ssbo); ssbo = 0;
	}

	void multidVoxelization(int rotx, int roty, int precision, std::string outputdir){
		using namespace redips;
		assert(voxelvbo);
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
		mdVoxelization(voxelvbo, TOTAL_VOXEL_CNT, mbox.heart(), rotx, roty, mats[0].ptr(), precision, outputdir);
	}
private:
	redips::BOX mbox;
};
