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

//**************************kernels
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
	cudaMemcpy(&TOTAL_VOXEL_CNT,presum_dev+workload-1,sizeof(unsigned int),cudaMemcpyDeviceToHost);
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

