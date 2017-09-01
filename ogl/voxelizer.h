#pragma once
#include "../geos/triangles.h"
#include "shader.h"
#include "glTexture.h"
#include "glMeshWrapper.h"

#define VOXEL_PROJECTION_VIEWPORT_SIZE 1024

extern "C" GLuint packVoxel(GLuint invbo, int res2, unsigned int &TOTAL_VOXEL_CNT, redips::float3 boxcenter, redips::float3 boxdim);
extern "C" void mdVoxelization(GLuint dotsvbo, int dotscnt, redips::float3 dotscenter, int rotx, int roty,const float* mats, unsigned int presition);
class Voxelizer{
public:
	Voxelizer(){};
	~Voxelizer(){};
	GLuint ssbo = 0;
	GLuint voxelvbo = 0;
	unsigned int res2;
	unsigned int TOTAL_VOXEL_CNT = 0;
public:
	void onedVoxelization(glMeshWrapper* model,Shader* shader,unsigned int res2){
		//glTexture counter;
		//counter.create3d(int3(resolution,resolution,resolution/32),GL_R32UI,GL_RED_INTEGER,GL_UNSIGNED_INT,nullptr);
		//use ssbo instead of texture
		assert(res2 < 11);
		this->res2 = res2 = MIN(res2,10);
		
		curModel = model;

		int bufferBytes = 1u << (res2 * 3 - 3);
		glGenBuffers(1,&ssbo);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
		glBufferData(GL_SHADER_STORAGE_BUFFER, bufferBytes, nullptr, GL_DYNAMIC_COPY);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		
		shader->Use();
		GLuint blockIndex = glGetProgramResourceIndex(shader->Program,GL_SHADER_STORAGE_BLOCK,"counter");
		glShaderStorageBlockBinding(shader->Program,blockIndex,0);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
		
		BOX mbox = model->model->aabb();
		//render 3 times
		{
			//GLint orivp[4];
			//glGetIntegerv(GL_VIEWPORT, orivp);
			//glViewport(0, 0, VOXEL_PROJECTION_VIEWPORT_SIZE, VOXEL_PROJECTION_VIEWPORT_SIZE);
			glDisable(GL_CULL_FACE);
			glDisable(GL_DEPTH_TEST);
			
			glUniform1ui(glGetUniformLocation(shader->Program, "res2"), res2);
			glUniformMatrix4fv(glGetUniformLocation(shader->Program, "model"), 1, GL_FALSE, (model->model->Transform()).transpose().ptr());

			glUniformMatrix4fv(glGetUniformLocation(shader->Program, "projection"), 1, GL_FALSE, (GeoUtil::glOrtho(mbox)).transpose().ptr());
			glUniformMatrix3fv(glGetUniformLocation(shader->Program, "swizzler"), 1, GL_FALSE, Mat33f::xyz().transpose().ptr());
			model->draw(false, false);

			glUniformMatrix4fv(glGetUniformLocation(shader->Program, "projection"), 1, GL_FALSE, (GeoUtil::glOrtho(mbox.back(), mbox.front(), mbox.left(), mbox.right(), mbox.bottom(), mbox.top())).transpose().ptr());
			glUniformMatrix3fv(glGetUniformLocation(shader->Program, "swizzler"), 1, GL_FALSE, Mat33f::zxy().transpose().ptr());
			model->draw(false, false);

			glUniformMatrix4fv(glGetUniformLocation(shader->Program, "projection"), 1, GL_FALSE, (GeoUtil::glOrtho(mbox.bottom(), mbox.top(), mbox.back(), mbox.front(), mbox.left(), mbox.right())).transpose().ptr());
			glUniformMatrix3fv(glGetUniformLocation(shader->Program, "swizzler"), 1, GL_FALSE, Mat33f::yzx().transpose().ptr());
			model->draw(false, false);

			glEnable(GL_DEPTH_TEST);
			glEnable(GL_CULL_FACE);
			//glViewport(orivp[0], orivp[1], orivp[2], orivp[3]);
		}
		
		voxelvbo = packVoxel(ssbo, res2, TOTAL_VOXEL_CNT, mbox.heart(), mbox.dim());
		isVoxelBoxSetup = false;
		puts("[voxlization] : finish");
	}

	void renderVoxels(){
		if (!isVoxelBoxSetup){
			if (voxel) delete voxel; if (glVoxelWrapper) delete glVoxelWrapper;
			voxel = new Triangles(curModel->model->aabb().dim()*(1.0f / (1u << res2)));
			glVoxelWrapper = new glMeshWrapper(voxel);
			
			glBindVertexArray(glVoxelWrapper->vaos[0]);
			glEnableVertexAttribArray(1);
			glBindBuffer(GL_ARRAY_BUFFER, voxelvbo);
			glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, NULL);
			glEnableVertexAttribArray(3);
			glVertexAttribDivisor(3, 1);
			
			isVoxelBoxSetup = true;
		}
		glBindVertexArray(glVoxelWrapper->vaos[0]);
		glDrawArraysInstanced(GL_TRIANGLES, 0, 36, TOTAL_VOXEL_CNT);
	}

	void multidVoxelization(int rotx, int roty, int precision){
		   assert(voxelvbo);
		   assert(precision<=res2);
		   BOX box = curModel->model->aabb(),tbox;
		   Mat44f *mats = new Mat44f[roty*rotx];
		   for (int x = 0; x < rotx; x++) for (int y = 0; y < roty; y++){
			   Mat33f rotate = Mat33f::tilt(RAD(x*180.0f / rotx))*Mat33f::pan(RAD(y*180.0f/roty));
			   for (unsigned int i = 0; i < 8; i++){ tbox += (rotate * (box.lbb + (box.dim()*float3::bits(i)))); }
			   float3 dim = tbox.dim();
			   mats[y*rotx + x] = GeoUtil::glOrtho(dim.x*-0.5f,dim.x*0.5f,dim.y*-0.5f,dim.y*0.5f,dim.z*-0.5f,dim.z*0.5f)*Mat44f(rotate);
			   tbox.reset();
		   }
		   mdVoxelization(voxelvbo, TOTAL_VOXEL_CNT, box.heart(), rotx, roty, mats[0].ptr(), precision);
	}
private:
	Triangles* voxel = nullptr;
	glMeshWrapper* glVoxelWrapper = nullptr;
	glMeshWrapper* curModel = nullptr;
	bool isVoxelBoxSetup = false;
};


/*
freopen("centers.txt","w",stdout);
float* ds = new float[TOTAL_VOXEL_CNT*3];
glBindBuffer(GL_ARRAY_BUFFER,voxelvbo);
glGetBufferSubData(GL_ARRAY_BUFFER, 0, TOTAL_VOXEL_CNT * 3 * sizeof(float), ds);
for (int i = 0; i < TOTAL_VOXEL_CNT; i++) printf("%f %f %f\n", ds[i * 3 + 0], ds[i * 3 + 1], ds[i * 3 + 2]);
fclose(stdout);
freopen("CON","w",stdout);
*/
/*
unsigned int * tmp = new unsigned int[bufferBytes>>2];
glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, bufferBytes, tmp);
freopen("debug5.txt","w",stdout);
int cnt = 0;
for (int i = 0; i < (1u<<(res2*3-5)); i++){
for (int j = 0; j < 32; j++) {
printf("%d", (tmp[i] & (1 << j))>0);
if ((tmp[i] & (1 << j))>0) cnt++;
}printf("   -->%u\n", tmp[i]);
}
fclose(stdout);
freopen("CON","w",stdout);
printf("haha cnt is %d\n",cnt);
*/
/*

GLuint feedback;
glGenTransformFeedbacks(1, &feedback);
glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, feedback);
GLuint feedbackBuf;
glGenBuffers(1, &feedbackBuf);
glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, feedbackBuf);
glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, model->model->faceCnt * 3 * 3*sizeof(float), NULL, GL_DYNAMIC_COPY);
glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, feedbackBuf);
static const char* const vars[] = { "debug" };
glTransformFeedbackVaryings(shader->Program, 1, vars, GL_INTERLEAVED_ATTRIBS);
glLinkProgram(shader->Program);


//glBeginTransformFeedback(GL_TRIANGLES);

//glEndTransformFeedback();



freopen("fragpos.txt","w",stdout);
float* lbuffer = new float[model->model->faceCnt*3*3];
glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, model->model->faceCnt * 3 * 3 * sizeof(float), lbuffer);
for (int i = 0; i < model->model->faceCnt * 3; i++) printf("%f %f %f\n", lbuffer[i * 3 + 0], lbuffer[i * 3 + 1], lbuffer[i * 3 + 2]);
fclose(stdout);
freopen("CON","w",stdout);
*/
/*
unsigned int * tmp = new unsigned int[bufferBytes >> 2];
freopen("C:/Users/Asrock/Desktop/128/1.txt","r",stdin);
for (int i = 0; i < 128; i++) for (int j = 0; j < 128; j++){
unsigned int e;
unsigned int* ptr = &tmp[i * 128 * 4 + j * 4];
scanf("%u %u %u %u %u %u %u %u", &e, &e, &e, &e, &ptr[0], &ptr[1], &ptr[2], &ptr[3]);
}
glBufferData(GL_SHADER_STORAGE_BUFFER, bufferBytes, tmp, GL_DYNAMIC_COPY);
glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, bufferBytes, tmp);
freopen("128.txt", "w", stdout);
int cnt = 0;
int N = 1u << res2;
for (int i = 0; i < N; i++) for (int j = 0; j < N; j++) {
printf("%d %d ",i,j);
for (int k = 0; k < (1u << (res2 - 5)); k++){
printf("%u ",tmp[i*N/32*N+j*N/32+k]);
}
puts("");
}
*/
//for (int i = 0; i < (1u << (res2 * 3 - 5)); i++){
//for (int j = 0; j < 32; j++) {
//printf("%d", (tmp[i] & (1 << j))>0);
//if ((tmp[i] & (1 << j))>0) cnt++;
//}printf("   -->%u\n", tmp[i]);
//}
//fclose(stdout);
//freopen("CON", "w", stdout);




