#pragma once
#include <GL/glew.h>
#include <vec.h>

#define CHECK_GL_ERROR(s) {if(glGetError()!=GL_NO_ERROR){printf("glError %s\n",(s));exit(-1);};}

extern "C" void blocker_initialize_cuda(
	        GLuint postex, GLuint normtex, GLuint shadowmap,
	        int lcnt, unsigned int width, unsigned int height, unsigned int rcnt, unsigned int columnCnt, unsigned int pres, bool compressed, redips::float3 heart,
	        const float* lights, const float* axises, const float* radius, const unsigned int* tags, const unsigned int* indexes, const unsigned int* columns
);
extern "C" void launch_4_compare();
extern "C" void launch_4_rendering();
extern "C" void clean_cuda_blocker();
extern "C" void compute_errors(GLuint optix_buffer, unsigned char* img_cpu, redips::int2& ret);

class Blocker{
public:
	//use uncompressed result
	Blocker(GLuint postex, GLuint normtex,int lightCnt,const float* lights,const float* axises,const float* radius,
		        const unsigned int* tags,
		        unsigned int width, unsigned int height, unsigned int rotcnt, int precision, redips::float3 heart)
	{
		useCompressedColumns = false;
		glGenBuffers(1, &shadowMapAB);
		glBindBuffer(GL_ARRAY_BUFFER, shadowMapAB);
		glBufferData(GL_ARRAY_BUFFER, sizeof(unsigned int)*width*height, NULL, GL_DYNAMIC_COPY);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		blocker_initialize_cuda(postex, normtex, shadowMapAB,
			                               lightCnt, width, height, rotcnt, 0, precision, useCompressedColumns,heart,
			                               lights, axises, radius, tags,nullptr,nullptr);

		CHECK_GL_ERROR("bad cuda-blocker enviroment");
		error_image = new unsigned char[3 * width*height];
		initialized = true;
	}
	//use compressed result
	Blocker(GLuint postex, GLuint normtex,
		        int lightCnt, const float* lights, const float* axises, const float* radius,
		        unsigned int columnCnt, const unsigned int* columns, const unsigned int* indexes,
		        unsigned int width, unsigned int height, unsigned int rotcnt, int precision, redips::float3 heart)
	{
		useCompressedColumns = true;
		glGenBuffers(1, &shadowMapAB);
		glBindBuffer(GL_ARRAY_BUFFER, shadowMapAB);
		glBufferData(GL_ARRAY_BUFFER, sizeof(unsigned int)*width*height, NULL, GL_DYNAMIC_COPY);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		
		blocker_initialize_cuda(postex, normtex, shadowMapAB,
			                               lightCnt, width, height, rotcnt, columnCnt, precision, useCompressedColumns, heart,
										   lights, axises, radius, nullptr, indexes, columns);

		CHECK_GL_ERROR("bad cuda-blocker enviroment");
		error_image = new unsigned char[3*width*height];
		initialized = true;
	}

	~Blocker(){
		if (shadowMapAB) glDeleteBuffers(1, &shadowMapAB); shadowMapAB = 0;
		clean_cuda_blocker();
	}

	void launch1(GLuint optix_buffer){//for compare
		//launch_4_compare();
		puts("wrong,delete");
		redips::int2 diffs;
		compute_errors(optix_buffer, error_image,diffs);
		float bige = diffs.x / 1024.0f / 512 / 512;
		float smalle = diffs.y / 1024.0f / 512 / 512;
		printf("bige %f,smalle %f\n",bige,smalle);
	};    
	void launch2(){//for rendering
		launch_4_rendering();
	};    
	GLuint smvbo(){ return shadowMapAB; }
private:
	bool initialized = false;
	bool useCompressedColumns;
	
	GLuint shadowMapAB = 0;
	unsigned char* error_image=nullptr;
};