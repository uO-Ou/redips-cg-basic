#pragma once
#include "triangles.h"
#include "particles.h"
#include "../ogl/shader.h"
#include "../ogl/glTexture.h"
#include "../ogl/glMeshWrapper.h"

extern "C" GLuint packVoxel(GLuint invbo, int resolution);

class Voxelizer{
public:
	Voxelizer(){};
	~Voxelizer(){};
	GLuint ssbo = 0;
	int resolution;
public:
	void run(const glMeshWrapper* model,Shader* shader,int resolution){
		//glTexture counter;
		//counter.create3d(int3(resolution,resolution,resolution/32),GL_R32UI,GL_RED_INTEGER,GL_UNSIGNED_INT,nullptr);
		//use ssbo instead of texture
		shader->Use();
		this->resolution = resolution;
		int bufferBytes = resolution*resolution*resolution>>3;
		glGenBuffers(1,&ssbo);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
		glBufferData(GL_SHADER_STORAGE_BUFFER, bufferBytes, nullptr, GL_DYNAMIC_COPY);
		//glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		GLuint blockIndex = glGetProgramResourceIndex(shader->Program,GL_SHADER_STORAGE_BLOCK,"counter");
		glShaderStorageBlockBinding(shader->Program,blockIndex,0);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER,0,ssbo);
		
		glUniform1i(glGetUniformLocation(shader->Program,"N"),resolution);
		Mat44f projection = GeoUtil::glOrtho(model->model->aabb());
		glUniformMatrix4fv(glGetUniformLocation(shader->Program, "projection_model"), 1, GL_FALSE, (projection*model->model->Transform()).transpose().ptr());
	    
		model->draw(false,false);
	
		/*
		unsigned int * tmp = new unsigned int[bufferBytes>>2];
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, bufferBytes, tmp);
		freopen("debug5.txt","w",stdout);
		int cnt = 0;
		for (int i = 0; i < resolution*resolution*resolution >> 5; i++){
			for (int j = 0; j < 32; j++) {
				printf("%d", (tmp[i] & (1 << j))>0);
				if ((tmp[i] & (1 << j))>0) cnt++;
			}printf("   -->%u\n", tmp[i]);
		}
		fclose(stdout);
		freopen("CON","w",stdout);
		printf("haha cnt is %d\n",cnt);
		*/

		packVoxel(ssbo,resolution);
		puts("[voxlization] : finish");
		
	}

	void renderVoxels(Shader* shader){
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	}
};

