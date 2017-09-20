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

#define _MAX_GMEM_4_VOXELRESULT_IN_GB_ 2.0f
#define _DIRECTION_CNT_PER_BLOCK_ 32u
#define _THREAD_NUM_PER_BLOCK_ 512
#define CC(x) {const cudaError_t a = (x); if(a!=cudaSuccess){printf("\ncuda error:%s(err_num=%d)\n",cudaGetErrorString(a),a);cudaDeviceReset();assert(0);}}

/******************************************************************/
/**********                    device function                        ***********/
/******************************************************************/
__device__ float3 multy(float* mat,const float4& dot){
	float3 ret;
	float winv = 1.0f/((mat[12] * dot.x) + (mat[13] * dot.y) + (mat[14] * dot.z) + mat[15]);
	ret.x = ((mat[0] * dot.x) + (mat[1] * dot.y) + (mat[2] * dot.z) + mat[3])*winv;
	ret.y = ((mat[4] * dot.x) + (mat[5] * dot.y) + (mat[6] * dot.z) + mat[7])*winv;
	ret.z = ((mat[8] * dot.x) + (mat[9] * dot.y) + (mat[10] * dot.z) + mat[11])*winv;
	return ret;
}
__inline__ __device__ unsigned int  mclamp(unsigned int a, unsigned int b, unsigned c){
	if (a <= b) return b;
	if (a >= c) return c;
	return a;
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


/******************************************************************/
/**********                          kernels                               ***********/
/******************************************************************/
//allocate frags to cell
__global__ void frags2Cell(float4* frags, int fragcnt, redips::float3 center, float *mats, unsigned int precision, unsigned int * result){
	const int gap = blockDim.x * gridDim.x;
	const int tid = blockIdx.x*blockDim.x + threadIdx.x;

	__shared__ float mats_sm[16];
	if (threadIdx.x < 16) mats_sm[threadIdx.x] = mats[threadIdx.x];
	__syncthreads();

	unsigned int resolution = 1u << precision;
	unsigned int res2 = resolution >> 1u;
	for (int fid = tid; fid < fragcnt; fid += gap){
		float4 tdot = frags[fid];   tdot.x -= center.x, tdot.y -= center.y, tdot.z -= center.z;
		float3 ndc = multy(mats_sm, tdot);

		unsigned int tx = 0.5f + ndc.x * res2 + res2;  tx = mclamp(tx, 0, resolution - 1);
		unsigned int ty = 0.5f + ndc.y * res2 + res2;  ty = mclamp(ty, 0, resolution - 1);
		unsigned int tz = 0.5f + ndc.z * res2 + res2;  tz = mclamp(tz, 0, resolution - 1);

		atomicOr(&result[(tx << (precision * 2 - 5)) + (ty << (precision - 5)) + (tz >> 5)], 1u << (tz & 31u));
	}
}
//calculate 1's count in each cell
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
//calculate cell center's coordinate
__global__ void gendots(unsigned int *input, unsigned int *presum, float3* output, float* axis,int precision, int workload, redips::float3 boxcenter, redips::float3 boxdim){
	const unsigned int gap = gridDim.x * blockDim.x;
	unsigned int tid = blockIdx.x * blockDim.x + threadIdx.x;
	float resolution = 1u << precision;
	float3 dot;
	for (; tid < workload; tid += gap){
		unsigned int status = input[tid];
		if (!status) continue;
		float3* ptrf3 = output + presum[tid];
		unsigned int voxelx = tid >> (precision + precision - 5);
		unsigned int voxely = (tid&((1u << (precision + precision - 5)) - 1u)) >> (precision - 5);
		unsigned int voxelz = (tid&((1u << (precision - 5)) - 1u)) << 5;
		
		for (int i = 0; i < 32; i++){
			if (status&(1u << i)) {
			    float tx = (voxelx / resolution - 0.5f)*boxdim.x;
			    float ty = (voxely / resolution - 0.5f)*boxdim.y;
				float tz = ((voxelz + i) / resolution - 0.5f)*boxdim.z;

				dot.x = tx * axis[0] + ty * axis[3] + tz * axis[6] + boxcenter.x;
				dot.y = tx * axis[1] + ty * axis[4] + tz * axis[7] + boxcenter.y;
				dot.z = tx * axis[2] + ty * axis[5] + tz * axis[8] + boxcenter.z;

				*(ptrf3++) = dot;
			}
		}
	}
}
//transform frags [sid-eid] to cell
__global__ void transform(float4* frags, int fragcnt, redips::float3 center, float* mats,unsigned int sid,unsigned int eid,unsigned int precision,unsigned int * result){
	unsigned int tid = threadIdx.x;
	unsigned int matSId = blockIdx.x * _DIRECTION_CNT_PER_BLOCK_;
	unsigned int matEId = matSId + _DIRECTION_CNT_PER_BLOCK_ - 1; if (matEId + sid > eid) matEId = eid - sid;
	unsigned int matCnt = matEId - matSId + 1;
	unsigned int resolution = 1u << precision;
	unsigned int shiftleft = precision * 3 - 5;            //*
	unsigned int *resultPtr = &result[(matSId<<shiftleft)]; //*

	__shared__ float mats_sm[_DIRECTION_CNT_PER_BLOCK_ << 4];
	{
		float* matsptr = mats + ((sid+matSId) << 4);
		if (tid < (matCnt << 4)){
			mats_sm[tid] = matsptr[tid];
		}
		__syncthreads();
	}
	
	unsigned int res2 = resolution >> 1;
	for (; tid < fragcnt; tid += _THREAD_NUM_PER_BLOCK_){
		float4 dot = frags[tid]; 
		dot.x -= center.x; dot.y -= center.y; dot.z -= center.z;
		for (unsigned int mid = 0; mid < matCnt; mid++){  // mid+sid 
			float3 ndc = multy(&(mats_sm[(mid<<4)]),dot);
			
			unsigned int tx = 0.5f + ndc.x * res2 + res2;  tx = mclamp(tx, 0, resolution - 1);
			unsigned int ty = 0.5f + ndc.y * res2 + res2;  ty = mclamp(ty, 0, resolution - 1);
			unsigned int tz = 0.5f + ndc.z * res2 + res2;  tz = mclamp(tz, 0, resolution - 1);

			unsigned int offset = (mid<<shiftleft) + ((tx<<(precision*2 - 5)) + (ty<<(precision - 5)) + (tz>>5));

			atomicOr(&resultPtr[offset], (1u << (tz & 31u)));
		}
	}
}


/******************************************************************/
/**********                    host function                        ***********/
/******************************************************************/
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
unsigned int compact_cuda(GLuint invbo,int precision){
	unsigned int* inputptr; size_t num_bytes;
	struct cudaGraphicsResource *cuda_input;
	//map vbo 2 cuda
	checkCudaErrors(cudaGraphicsGLRegisterBuffer(&cuda_input, invbo, cudaGraphicsMapFlagsNone));
	checkCudaErrors(cudaGraphicsMapResources(1, &cuda_input, 0));
	checkCudaErrors(cudaGraphicsResourceGetMappedPointer((void **)&inputptr, &num_bytes, cuda_input));
	SDK_CHECK_ERROR_GL();
	printf("[cuda] : mapped %d bytes voxel input\n", num_bytes);

	unsigned int workload = (1u << (precision * 3 - 5));

	unsigned int LAST_CELL_CNT = 0;
	checkCudaErrors(cudaMemcpy(&LAST_CELL_CNT, inputptr + workload - 1, sizeof(unsigned int), cudaMemcpyDeviceToHost));
	CC(cudaDeviceSynchronize());

	thrust::device_ptr<unsigned int> presumPtr(inputptr);
	thrust::exclusive_scan(presumPtr, presumPtr + workload, presumPtr);
	CC(cudaDeviceSynchronize());

	unsigned int TOTAL_FRAG_CNT = 0;
	checkCudaErrors(cudaMemcpy(&TOTAL_FRAG_CNT, inputptr + workload - 1, sizeof(unsigned int), cudaMemcpyDeviceToHost));

	checkCudaErrors(cudaGraphicsUnmapResources(1, &cuda_input, 0));
	return TOTAL_FRAG_CNT + LAST_CELL_CNT;
};

extern "C"
unsigned int onedVoxelization_cuda(GLuint fragsvbo, unsigned int fragscnt, redips::float3 fragscenter, redips::float3 boxdim,const float* mats,const float* axises, unsigned int precision, GLuint resultvbo){
	float* frags_dev;  float3* voxels_dev; size_t num_bytes;
	struct cudaGraphicsResource *cuda_vbo_binder1, *cuda_vbo_binder2;
	checkCudaErrors(cudaGraphicsGLRegisterBuffer(&cuda_vbo_binder1, fragsvbo, cudaGraphicsMapFlagsReadOnly));
	checkCudaErrors(cudaGraphicsMapResources(1, &cuda_vbo_binder1, 0));
	checkCudaErrors(cudaGraphicsResourceGetMappedPointer((void **)&frags_dev, &num_bytes, cuda_vbo_binder1));
	printf("[cuda] : mapped %d bytes[%.fM] frags_ssbo\n", num_bytes, num_bytes*1.0f / 1024 / 1024);

	checkCudaErrors(cudaGraphicsGLRegisterBuffer(&cuda_vbo_binder2, resultvbo, cudaGraphicsMapFlagsWriteDiscard));
	checkCudaErrors(cudaGraphicsMapResources(1, &cuda_vbo_binder2, 0));
	checkCudaErrors(cudaGraphicsResourceGetMappedPointer((void **)&voxels_dev, &num_bytes, cuda_vbo_binder2));
	printf("[cuda] : mapped %d bytes[%.fM] voxel_vbo\n", num_bytes, num_bytes*1.0f / 1024 / 1024);

	float* mats_dev;
	CC(cudaMalloc((void**)&mats_dev, sizeof(float) * 16));
	CC(cudaMemcpy(mats_dev, mats, sizeof(float) * 16, cudaMemcpyHostToDevice));

	unsigned int * cellflag_dev;
	CC(cudaMalloc((void**)&cellflag_dev, (1u << (precision * 3 - 3))));
	CC(cudaMemset(cellflag_dev, 0, (1u << (precision * 3 - 3))));
	CC(cudaDeviceSynchronize());

	//allocate to cell
	(frags2Cell << <(fragscnt - 1) / 512 + 1, 512 >> >((float4*)frags_dev, fragscnt, fragscenter, mats_dev, precision, cellflag_dev));
	CC(cudaDeviceSynchronize());

	//calculate presum
	unsigned int workload = 1u << (precision * 3 - 5);
	unsigned int * presum_dev;
	CC(cudaMalloc((void**)&presum_dev, (1u << (precision * 3 - 3))));
	calcnt << <(workload - 1) / 512 + 1, 512 >> >(cellflag_dev, presum_dev, workload);
	CC(cudaDeviceSynchronize());
	unsigned int LAST_CNT;
	checkCudaErrors(cudaMemcpy(&LAST_CNT, presum_dev + workload - 1, sizeof(unsigned int), cudaMemcpyDeviceToHost));

	thrust::device_ptr<unsigned int> presumPtr(presum_dev);
	thrust::exclusive_scan(presumPtr, presumPtr + workload, presumPtr);
	CC(cudaDeviceSynchronize());
	unsigned int TOTAL_VOXEL_CNT;
	checkCudaErrors(cudaMemcpy(&TOTAL_VOXEL_CNT, presum_dev + workload - 1, sizeof(unsigned int), cudaMemcpyDeviceToHost));
	CC(cudaDeviceSynchronize());

	//generate dots
	CC(cudaMemcpy(mats_dev, axises, sizeof(float)* 9, cudaMemcpyHostToDevice));
	gendots << <(workload - 1) / 512 + 1, 512 >> >(cellflag_dev, presum_dev, voxels_dev, mats_dev, precision, workload, fragscenter, boxdim);


	checkCudaErrors(cudaGraphicsUnmapResources(1, &cuda_vbo_binder1, 0));
	checkCudaErrors(cudaGraphicsUnmapResources(1, &cuda_vbo_binder2, 0));
	checkCudaErrors(cudaFree(mats_dev));
	checkCudaErrors(cudaFree(cellflag_dev));
	checkCudaErrors(cudaFree(presum_dev));
	return TOTAL_VOXEL_CNT + LAST_CNT;
}

extern "C"
void mdVoxelization_cuda(GLuint fragsvbo, unsigned int fragscnt, redips::float3 fragscenter, int rotx, int roty, const float* mats, unsigned int presicion, std::string outputdir){
	   //prepare
	   float MEM_PER_DIRECTION_MB = (1u << (presicion * 3 - 3))*1.0f / (1u << 20);
	   int DIRECTION_CNT_PER_PROCESS = _MAX_GMEM_4_VOXELRESULT_IN_GB_ * 1024.0f / MEM_PER_DIRECTION_MB;
	   size_t result_uint_cnt = DIRECTION_CNT_PER_PROCESS*(1u << (presicion * 3 - 5));
	   printf("[cuda] : MEM_PER_DIRECTION_MB is %.4f, DIRECTION_CNT_PER_PROCESS %lld using %.4f g memory\n", MEM_PER_DIRECTION_MB, DIRECTION_CNT_PER_PROCESS, DIRECTION_CNT_PER_PROCESS*MEM_PER_DIRECTION_MB/1024.0f);
	   unsigned int * rbuffer = new unsigned int[result_uint_cnt];
	   char strBuffer[256];  sprintf(strBuffer, "%s/%d", outputdir.c_str(), (1u << presicion)); outputdir = std::string(strBuffer)+"/";
	   _mkdir(strBuffer);
	   
	   //allocate memory
	   float* frags_dev; size_t num_bytes;
	   struct cudaGraphicsResource *cuda_vbo_binder;
	   checkCudaErrors(cudaGraphicsGLRegisterBuffer(&cuda_vbo_binder, fragsvbo, cudaGraphicsMapFlagsReadOnly));
	   checkCudaErrors(cudaGraphicsMapResources(1, &cuda_vbo_binder, 0));
	   checkCudaErrors(cudaGraphicsResourceGetMappedPointer((void **)&frags_dev, &num_bytes, cuda_vbo_binder));
	   printf("[cuda] : mapped %d bytes[%.fM] dots\n", num_bytes,num_bytes*1.0f/1024/1024);
	   
	   float* mats_dev;
	   checkCudaErrors(cudaMalloc((void**)&mats_dev, sizeof(float)* 16 * rotx * roty));
	   checkCudaErrors(cudaMemcpy(mats_dev, mats, sizeof(float)* 16 * rotx * roty, cudaMemcpyHostToDevice));

	   unsigned int* result_dev;
	   checkCudaErrors(cudaMalloc((void**)&result_dev, result_uint_cnt*sizeof(unsigned int)));
	   thrust::device_ptr<unsigned int > resultSPtr(result_dev);
	   thrust::device_ptr<unsigned int > resultEPtr(result_dev + result_uint_cnt);
	   
	   //launch kernels
	   int cid = 0;
	   StopWatchInterface *timer = 0;
	   sdkCreateTimer(&timer);
	   cudaDeviceSynchronize();
	   sdkStartTimer(&timer);
	   
	   for (int sid = 0; ;sid += DIRECTION_CNT_PER_PROCESS){
		   int tid = std::min(sid + DIRECTION_CNT_PER_PROCESS - 1, rotx*roty - 1);
		   int blockCnt = ((tid - sid + 1) - 1) / _DIRECTION_CNT_PER_BLOCK_ + 1;

		   printf("[cuda] : dealing %d - %d\n",sid,tid);
		   thrust::fill(resultSPtr, resultEPtr,0u);
		   CC(cudaDeviceSynchronize());

		   transform << <blockCnt, _THREAD_NUM_PER_BLOCK_ >> >((float4*)frags_dev, fragscnt, fragscenter, mats_dev, sid, tid, presicion, result_dev);
		   CC(cudaDeviceSynchronize());

		   printf("[cuda] : copying to cpu \n");
		   checkCudaErrors(cudaMemcpy(rbuffer, result_dev, size_t(tid - sid + 1)*(1u<<(presicion*3-3)),cudaMemcpyDeviceToHost));
		   CC(cudaDeviceSynchronize());
		   
		   printf("[cuda] : writing to hard disk \n");
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

