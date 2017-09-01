#pragma once
#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include "stdio.h"
#include <helper_gl.h>
// includes, cuda
#include <cuda_runtime.h>
#include <cuda_gl_interop.h>

// Utilities and timing functions
#include <helper_functions.h>    // includes cuda.h and cuda_runtime_api.h
#include <timer.h>               // timing functions

// CUDA helper functions
#include <helper_cuda.h>         // helper functions for CUDA error check
#include <helper_cuda_gl.h>      // helper functions for CUDA/GL interop

#include <vector_types.h>

#include <thrust/scan.h>
#include <thrust/execution_policy.h>
#include <thrust/device_vector.h>
#include <vec.h>

#define _MAX_GMEM_4_VOXELRESULT_IN_GB_ 2.0
#define _DIRECTION_CNT_PER_BLOCK_ 16u
#define _THREAD_NUM_PER_BLOCK_ 1024

/******************************************************************/
/**********                          kernels                               ***********/
/******************************************************************/
__device__ void multy(){

}
__global__ void calcnt(unsigned int *input,unsigned int *output,int workload){
	const unsigned int gap = gridDim.x * blockDim.x;
	unsigned int tid = blockIdx.x * blockDim.x + threadIdx.x;
	for (; tid < workload;tid+=gap){
		unsigned int status = input[tid];
		int cnt = 0;
		for (int i = 0; i < 32; i++){
			if (!status) break;
			if (status & 1u) cnt++;
			status >>= 1;
		}
		output[tid] = cnt;
	}
}
__global__ void gendots(unsigned int *input, unsigned int *presum, float3* dots, int res2, int workload, redips::float3 boxcenter, redips::float3 boxdim){
	const unsigned int gap = gridDim.x * blockDim.x;
	unsigned int tid = blockIdx.x * blockDim.x + threadIdx.x;
	float resolution = 1u << res2;
	float3 dot;
	for (; tid < workload; tid += gap){
		unsigned int status = input[tid];
		if (!status) continue;
		float3* ptrf3 = dots + presum[tid];
		unsigned int voxelx = tid >> (res2+res2 - 5);
		unsigned int voxely = (tid&((1u<<(res2+res2-5))-1u)) >> (res2 - 5);
		unsigned int voxelz = (tid&((1u<<(res2 - 5)) - 1)) << 5;
		
		for (int i = 0; i < 32; i++){
			if (status&(1u << i)) {
				dot.x = (voxelx / resolution - 0.5f)*boxdim.x + boxcenter.x;
				dot.y = (voxely / resolution - 0.5f)*boxdim.y + boxcenter.y;
				dot.z = ((voxelz + i) / resolution - 0.5f)*boxdim.z + boxcenter.z;
				*(ptrf3++) = dot;
			}
		}
	}
}
__global__ void transform(float3* dots, unsigned dotcnt, redips::float3 center, float* mats,unsigned int sid,unsigned int eid,unsigned int presicion,unsigned int * result){
	const unsigned int gap = blockDim.x;
	unsigned int tid = threadIdx.x;
	unsigned int matSId = blockIdx.x * _DIRECTION_CNT_PER_BLOCK_;
	unsigned int matEId = matSId + _DIRECTION_CNT_PER_BLOCK_ - 1; if (matEId + sid > eid) matEId = eid - sid;

	__shared__ float mats_sm[_DIRECTION_CNT_PER_BLOCK_ << 4];
	__shared__ float dots_sm[_THREAD_NUM_PER_BLOCK_*3];
	{
		float* matsptr = mats + ((sid+matSId) << 4);
		if (tid < ((matEId - matSId + 1) << 4)){
			mats_sm[tid] = matsptr[tid];
		}
		__syncthreads();
	}
	
	for (; tid < dotcnt; tid += gap){
		for (int mid = matSId; mid <= matEId; mid++){  // mid+sid 

		}
	}
}

extern "C"
bool cudaInit(){
	cudaError_t cudaStatus;
	cudaStatus = cudaSetDevice(0);
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaSetDevice failed!  Do you have a CUDA-capable GPU installed?");
		return false;
	}
	return true;
}

extern "C"
GLuint packVoxel(GLuint invbo, int res2, unsigned int &TOTAL_VOXEL_CNT,redips::float3 boxcenter, redips::float3 boxdim){
	GLuint outvbo = 0;
	unsigned int* inputptr;  float3* outptr; size_t num_bytes;
	struct cudaGraphicsResource *cuda_input,*cuda_output;
	//step1: map vbo 2 cuda
	checkCudaErrors(cudaGraphicsGLRegisterBuffer(&cuda_input, invbo, cudaGraphicsMapFlagsReadOnly));
	checkCudaErrors(cudaGraphicsMapResources(1, &cuda_input, 0));
	checkCudaErrors(cudaGraphicsResourceGetMappedPointer((void **)&inputptr, &num_bytes, cuda_input));
	SDK_CHECK_ERROR_GL();
	printf("[cuda] : mapped %d bytes voxel input\n", num_bytes);
	//step2: compact, calculate presum
	unsigned int workload = 1u << (res2*3-5);
	unsigned int* presum_dev;
	checkCudaErrors(cudaMalloc((void**)&presum_dev,workload*sizeof(unsigned int)));
	calcnt << <(workload - 1) / 512 + 1, 512 >> >(inputptr,presum_dev,workload);
	cudaDeviceSynchronize();
	thrust::device_ptr<unsigned int > presumPtr(presum_dev);
	thrust::exclusive_scan(presumPtr, presumPtr + workload, presumPtr);
	cudaDeviceSynchronize();
	checkCudaErrors(cudaMemcpy(&TOTAL_VOXEL_CNT, presum_dev + workload - 1, sizeof(unsigned int), cudaMemcpyDeviceToHost));
	printf("[cuda] : total voxel cnt is %d\n",TOTAL_VOXEL_CNT);
	//step3: new buffer,generate dots 
	glGenBuffers(1,&outvbo);
	glBindBuffer(GL_ARRAY_BUFFER, outvbo);
	glBufferData(GL_ARRAY_BUFFER, TOTAL_VOXEL_CNT*sizeof(float)* 3, nullptr, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	checkCudaErrors(cudaGraphicsGLRegisterBuffer(&cuda_output, outvbo, cudaGraphicsMapFlagsWriteDiscard));
	checkCudaErrors(cudaGraphicsMapResources(1, &cuda_output, 0));
	checkCudaErrors(cudaGraphicsResourceGetMappedPointer((void **)&outptr, &num_bytes, cuda_output));
	printf("[cuda] : mapped %d bytes voxel out\n", num_bytes);
	gendots << <(workload - 1) / 512 + 1, 512 >> >(inputptr, presum_dev, outptr, res2, workload,boxcenter,boxdim);
	
	//step4. unmap, release memory
	checkCudaErrors(cudaGraphicsUnmapResources(1, &cuda_output, 0));
	checkCudaErrors(cudaGraphicsUnmapResources(1, &cuda_input, 0));
	cudaFree(presum_dev);
	return outvbo;
}

extern "C"
void mdVoxelization(GLuint dotsvbo, int dotscnt, redips::float3 dotscenter, int rotx, int roty,const float* mats, unsigned int presicion){
	   float MEM_PER_DIRECTION_MB = (1u << (presicion * 3 - 3))*1.0f / (1u << 20);
	   size_t DIRECTION_CNT_PER_PROCESS = _MAX_GMEM_4_VOXELRESULT_IN_GB_*1024.0f / MEM_PER_DIRECTION_MB;
	   printf("[cuda] : MEM_PER_DIRECTION_MB is %.4f, DIRECTION_CNT_PER_PROCESS %lld using %.4fg memory\n", MEM_PER_DIRECTION_MB, DIRECTION_CNT_PER_PROCESS, DIRECTION_CNT_PER_PROCESS*MEM_PER_DIRECTION_MB/1024);
	   
	   float* dots_dev; size_t num_bytes;
	   struct cudaGraphicsResource *cuda_vbo_binder;
	   checkCudaErrors(cudaGraphicsGLRegisterBuffer(&cuda_vbo_binder, dotsvbo, cudaGraphicsMapFlagsReadOnly));
	   checkCudaErrors(cudaGraphicsMapResources(1, &cuda_vbo_binder, 0));
	   checkCudaErrors(cudaGraphicsResourceGetMappedPointer((void **)&dots_dev, &num_bytes, cuda_vbo_binder));
	   printf("[cuda] : mapped %d bytes dots\n", num_bytes);
	   
	   float* mats_dev;
	   checkCudaErrors(cudaMalloc((void**)&mats_dev, sizeof(float)* 16 * rotx*roty));
	   checkCudaErrors(cudaMemcpy(mats_dev, mats, sizeof(float)* 16 * rotx*roty, cudaMemcpyHostToDevice));

	   unsigned int* result_dev;
	   checkCudaErrors(cudaMalloc((void**)&result_dev, DIRECTION_CNT_PER_PROCESS*(1u << (presicion * 3 - 3))));

	   //launch kernels
	   for (int sid = 0; ;sid += DIRECTION_CNT_PER_PROCESS){
		   int tid = MIN(sid + DIRECTION_CNT_PER_PROCESS - 1, rotx*roty - 1);
		   int blockCnt = ((tid - sid + 1) - 1) / _DIRECTION_CNT_PER_BLOCK_ + 1;

		   if (tid >= rotx*roty - 1) break;
	   }

	   checkCudaErrors(cudaFree(result_dev));
	   checkCudaErrors(cudaFree(mats_dev));
	   checkCudaErrors(cudaGraphicsUnmapResources(1, &cuda_vbo_binder, 0));
}