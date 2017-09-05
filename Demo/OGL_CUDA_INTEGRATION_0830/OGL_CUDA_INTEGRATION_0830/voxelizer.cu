#pragma once
#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include "stdio.h"
#include <direct.h>
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
__inline__ __device__ float3 multy(float* mat,const float3& dot){
	float3 ret;
	float winv = 1.0f/((mat[12] * dot.x) + (mat[13] * dot.y) + (mat[14] * dot.z) + mat[15]);
	ret.x = ((mat[0] * dot.x) + (mat[1] * dot.y) + (mat[2] * dot.z) + mat[3])*winv;
	ret.y = ((mat[4] * dot.x) + (mat[5] * dot.y) + (mat[6] * dot.z) + mat[7])*winv;
	ret.z = ((mat[8] * dot.x) + (mat[9] * dot.y) + (mat[10] * dot.z) + mat[11])*winv;
	return ret;
}

/*__device__ unsigned char atomicOrChar(unsigned char* address, unsigned char val){
	unsigned int *base_address = (unsigned int *)((size_t)address & ~3);
	unsigned int selectors[] = { 0x3214, 0x3240, 0x3410, 0x4210 };
	unsigned int sel = selectors[(size_t)address & 3];
	unsigned int old, assumed, min_, new_;
	old = *base_address;
	do {
		assumed = old;
		min_ = min(val, (char)__byte_perm(old, 0, ((size_t)address & 3) | 0x4440));
		new_ = __byte_perm(old, min_, sel);
		if (new_ == old)
			break;
		old = atomicCAS(base_address, assumed, new_);
	} while (assumed != old);
	return old;
}*/

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
__global__ void transform(float3* dots, unsigned dotcnt, redips::float3 center, float* mats,unsigned int sid,unsigned int eid,unsigned int precision,unsigned int * result){
	const unsigned int gap = blockDim.x;
	unsigned int tid = threadIdx.x;
	unsigned int matSId = blockIdx.x * _DIRECTION_CNT_PER_BLOCK_;
	unsigned int matEId = matSId + _DIRECTION_CNT_PER_BLOCK_ - 1; if (matEId + sid > eid) matEId = eid - sid;
	unsigned int matCnt = matEId - matSId + 1;
	unsigned int resolution = 1u << precision;
	unsigned int shiftleft = precision * 3 - 5;            //*
	unsigned int * resultPtr = result + (matSId<<shiftleft); //*

	__shared__ float mats_sm[_DIRECTION_CNT_PER_BLOCK_ << 4];
	{
		float* matsptr = mats + ((sid+matSId) << 4);
		if (tid < (matCnt << 4)){
			mats_sm[tid] = matsptr[tid];
		}
		__syncthreads();
	}
	
	
	unsigned int res2 = resolution >> 1;
	for (; tid < dotcnt; tid += gap){
		float3 dot = dots[tid]; 
		dot.x -= center.x; dot.y -= center.y; dot.z -= center.z;
		for (unsigned int mid = 0; mid < matCnt; mid++){  // mid+sid 
			float3 ndc = multy(mats_sm+(mid<<4),dot);
			unsigned int tx = 0.5f + ndc.x * res2 + res2;  tx = CLAMP(tx, 0, resolution - 1);
			unsigned int ty = 0.5f + ndc.y * res2 + res2;  ty = CLAMP(ty, 0, resolution - 1);
			unsigned int tz = 0.5f + ndc.z * res2 + res2;  tz = CLAMP(tz, 0, resolution - 1);
			
			atomicOr((resultPtr + (mid << shiftleft)) + ((tx << ((precision << 1) - 5)) + (ty << (precision - 5)) + (tz >> 5)),1u<<(tz&31u));
			
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
void mdVoxelization(GLuint dotsvbo, int dotscnt, redips::float3 dotscenter, int rotx, int roty,const float* mats, unsigned int presicion,std::string outputdir){
	   float MEM_PER_DIRECTION_MB = (1u << (presicion * 3 - 3))*1.0f / (1u << 20);
	   size_t DIRECTION_CNT_PER_PROCESS = _MAX_GMEM_4_VOXELRESULT_IN_GB_*1024.0f / MEM_PER_DIRECTION_MB;
	   printf("[cuda] : MEM_PER_DIRECTION_MB is %.4f, DIRECTION_CNT_PER_PROCESS %lld using %.4fg memory\n", MEM_PER_DIRECTION_MB, DIRECTION_CNT_PER_PROCESS, DIRECTION_CNT_PER_PROCESS*MEM_PER_DIRECTION_MB/1024);
	   unsigned int * rbuffer = new unsigned int[DIRECTION_CNT_PER_PROCESS*(1u<<(presicion*3-5))];
	   char strBuffer[256];  sprintf(strBuffer, "%s/%d", outputdir.c_str(), (1u << presicion)); outputdir = std::string(strBuffer)+"/";
	   _mkdir(strBuffer);

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
	   int cid = 0;
	   StopWatchInterface *timer = 0;
	   sdkCreateTimer(&timer);
	   cudaDeviceSynchronize();
	   sdkStartTimer(&timer);
	   for (int sid = 0; ;sid += DIRECTION_CNT_PER_PROCESS){
		   int tid = MIN(sid + DIRECTION_CNT_PER_PROCESS - 1, rotx*roty - 1);
		   int blockCnt = ((tid - sid + 1) - 1) / _DIRECTION_CNT_PER_BLOCK_ + 1;

		   transform <<<blockCnt, _THREAD_NUM_PER_BLOCK_ >>>((float3*)dots_dev, dotscnt, dotscenter,mats_dev,sid, tid, (presicion), result_dev);
		   cudaDeviceSynchronize();
		   checkCudaErrors(cudaMemcpy(rbuffer, result_dev, size_t(tid - sid + 1)*(1u<<(presicion*3-3)),cudaMemcpyDeviceToHost));
		   cudaDeviceSynchronize();

		   unsigned int *uintptr = rbuffer;
		   unsigned int cnt_per_line = (1u << (presicion - 5));
		   for (int id = sid; id <= tid; id++){
			   int anglex = id % rotx;
			   int angley = id / rotx;
			   sprintf(strBuffer, "x%02d_y%02d.txt", anglex, angley);
			   freopen((outputdir+strBuffer).c_str(),"w",stdout);
			   
			   for (unsigned int ind = 0; ind < (1u << (presicion * 3 - 5)); ind+=cnt_per_line){
				   for (unsigned int step = 0; step < cnt_per_line; step++) printf("%u ",uintptr[ind+step]); puts("");
			   }

			   fclose(stdout);
			   uintptr += (1u << (presicion * 3 - 5));
		   }
		   freopen("CON","w",stdout);

		   printf("[cuda] : iteration %d finish \n", cid++);
		   if (tid >= rotx*roty - 1) break;
	   }
	   sdkStopTimer(&timer);

	   float elapseTime = 1.0e-3 * sdkGetTimerValue(&timer);
	   printf("[cuda] : transform total cost %f s\n",elapseTime);

	   delete rbuffer;
	   sdkDeleteTimer(&timer);
	   checkCudaErrors(cudaFree(result_dev));
	   checkCudaErrors(cudaFree(mats_dev));
	   checkCudaErrors(cudaGraphicsUnmapResources(1, &cuda_vbo_binder, 0));
}