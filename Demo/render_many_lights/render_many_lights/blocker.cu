#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include <cstdio>
#include <direct.h>
#include <helper_gl.h>
// includes, cuda
#include <cuda_runtime.h>
#include <cuda_gl_interop.h>

// Utilities and timing functions
#include <helper_functions.h>    // includes cuda.h and cuda_runtime_api.h

// CUDA helper functions
#include <helper_cuda.h>          // helper functions for CUDA error check
#include <helper_cuda_gl.h>      // helper functions for CUDA/GL interop

#include <vector_types.h>

#include <thrust/scan.h>
#include <thrust/execution_policy.h>
#include <thrust/device_vector.h>
#include <vec.h>
/***********************delete********************/
#include <fstream>
#include <iostream>
/***********************delete********************/
#define _MAX_FLOAT_CNT_PER_BLOCK_SHARED_MEM_ 12288
#define _MAX_LIGHT_CNT_PER_PROCESS_ 512   //32 * 16
#define _UINT_CNT_PER_PIXEL_RESULT_ 32        //暂时只支持1024个灯

/**************************DEVICE FUNCTION*************************/
__inline__ __device__ float dot3(const float3& a, const float3& b){
	return a.x*b.x + a.y*b.y + a.z*b.z;
}

__inline__ __device__ float3 operator- (const float3 &a, const float3 &b){
	return make_float3(a.x - b.x, a.y - b.y, a.z - b.z);
}

__inline__ __device__ float3 operator+ (const float3 &a, const float3 &b){
	return make_float3(a.x + b.x, a.y + b.y, a.z + b.z);
}

__inline__ __device__ float3 operator* (const float3 &v, float b){
	return make_float3(v.x*b, v.y*b, v.z*b);
}

__inline__ __device__ float3 normalize(const float3& v){
	float len_inv = 1.0f / sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
	return make_float3(v.x*len_inv, v.y*len_inv, v.z*len_inv);
}
template <typename T>
__inline__ __device__ T mclamp(T a, T b, T c){
	if (a < b) return b;  if (a>c) return c; return a;
};

__device__ unsigned int cnt1(unsigned int number){
	unsigned int ret = 0;
	while (number > 0){
		if (number & 1u) ret++;
		number >>= 1u;
	}
	return ret;
};
/**************************GLOBAL FUNCTION*************************/
texture<float4, cudaTextureType2D, cudaReadModeElementType> pos_ref;
texture<float4, cudaTextureType2D, cudaReadModeElementType> norm_ref;

__global__ void calVisibility_compressed(int width, int height, int lightCnt, int slight, float3* glights, int rot_xy, float3* gaxises, float3* gradius, unsigned int *columns, unsigned int* indexes, float3 center, unsigned int precision, unsigned int *result){
	unsigned int x = blockIdx.x * blockDim.x + threadIdx.x;
	unsigned int y = blockIdx.y * blockDim.y + threadIdx.y;
	const unsigned int tid_inblock_const = threadIdx.y*blockDim.x + threadIdx.x;
	const unsigned int threads_num_per_block = blockDim.x*blockDim.y;
	const unsigned int res2 = 1u << (precision - 1);
	const unsigned int resolution = 1u << precision;

	__shared__ float3 lights_sm[_MAX_LIGHT_CNT_PER_PROCESS_];
	{
		unsigned int tid_inblock = tid_inblock_const;
		for (; tid_inblock < lightCnt; tid_inblock += threads_num_per_block){
			lights_sm[tid_inblock] = glights[tid_inblock] - center;
		}
	}
	__syncthreads();

	if (x < width&&y < height){
		float4 tmpf4 = tex2D(norm_ref, (x + 0.5f) / (float(width)), (y + 0.5f) / (float(height)));
		const float3 pnorm = make_float3(tmpf4.x, tmpf4.y, tmpf4.z);
#define _poffset_ 0.6f
		tmpf4 = tex2D(pos_ref, (x + 0.5f) / (float(width)), (y + 0.5f) / (float(height)));
		const float3 ppos = make_float3(tmpf4.x + pnorm.x*_poffset_ - center.x, tmpf4.y + pnorm.y*_poffset_ - center.y, tmpf4.z + pnorm.z*_poffset_ - center.z);

		unsigned int xdir, ydir, rotID;
		float3 axis_x, axis_y, axis_z, radius;
		unsigned int *resultptr = result + (y*width + x)*_UINT_CNT_PER_PIXEL_RESULT_;
		for (int l = 0; l < lightCnt; l++){
			float3 lpos = lights_sm[l];
			float3 ray = lpos - ppos;

			if (dot3(ray, pnorm)>0.0){
				{//find most matching coordinate-system
					ray = normalize(ray);
					float cosx = sqrt(ray.x*ray.x + ray.z*ray.z);
					if (cosx < 1e-6){ xdir = rot_xy / 2, ydir = 0; }
					else{
						if (ray.x>0) cosx = -cosx;
						xdir = unsigned int(acos(cosx) * 57.29578f / 180.0f * rot_xy + 0.5f);
						ydir = unsigned int(acos(ray.z / cosx) * 57.29578f / 180.0f * rot_xy + 0.5f);
						xdir = mclamp<unsigned int>(xdir, 0, rot_xy - 1);
						ydir = mclamp<unsigned int>(ydir, 0, rot_xy - 1);
					}
					rotID = (ydir*rot_xy + xdir);
					radius = gradius[rotID];
					axis_x = gaxises[rotID * 3 + 0];
					axis_y = gaxises[rotID * 3 + 1];
					axis_z = gaxises[rotID * 3 + 2];
				}
				float3 midp = (ppos + ray + lpos) * 0.5f;      ////////////////////////////////////////////!!!!!!!!!!!!!!!!!!!!!!!!!! + ray
				unsigned int midx = unsigned int((dot3(midp, axis_x) / radius.x + 1)*res2);
				unsigned int midy = unsigned int((dot3(midp, axis_y) / radius.y + 1)*res2);

				if (midx < 0 || midx >= resolution || midy < 0 || midy >= resolution){ // line segment didnt intersect with scene.
					resultptr[(l + slight) >> 5u] |= (1u << ((l + slight) & 31u));
				}
				else{
					unsigned int z1 = unsigned int((dot3(ppos + ray, axis_z) / radius.z + 1)*res2); //////////////////////////////!!!!!!!!!!!!!!!!!!!+ray
					unsigned int z2 = unsigned int((dot3(lpos, axis_z) / radius.z + 1)*res2);
					if (z1>z2) { unsigned int tmp = z1; z1 = z2; z2 = tmp; }

					if (z1 < resolution && z2 >= 0){
						if (z1 < 0) z1 = 0;
						if (z2 >= resolution) z2 = resolution - 1;
						//unsigned int * tptr = &(gtags[(rotID << (precision * 3 - 5)) + (midx << (precision * 2 - 5)) + (midy << (precision - 5))]);
						unsigned int * tptr = &columns[indexes[((rotID << (precision << 1u)) + (midx << precision) + midy)] << (precision - 5)];

						unsigned int zs = z1 >> 5;
						unsigned int ze = z2 >> 5;
						unsigned int m1 = z1 & 31u;
						unsigned int m2 = z2 & 31u;
						if (zs == ze){
							//if (!((tptr[zs] << (31 - m2)) >> (31 - m2 + m1))) resultptr[(l + slight) >> 5u] |= (1u << ((l + slight) & 31u));
							if (!((tptr[zs] << (32 - m2)) >> (32 - m2 + m1))) resultptr[(l + slight) >> 5u] |= (1u << ((l + slight) & 31u));
						}
						else{
							bool flag = false;
							for (int c = zs + 1; c < ze; c++) if (tptr[c]){ flag = true; break; };
							if (!flag){
								if ((!((~((1u << (m1 + 1)) - 1u))&(tptr[zs]))) && (!(((1u << (m2)) - 1u)&(tptr[ze])))){
									resultptr[(l + slight) >> 5u] |= (1u << ((l + slight) & 31u));
								}
							}
						}
					}
					else {
						resultptr[(l + slight) >> 5u] |= (1u << ((l + slight) & 31u));
					}
				}
			}
		}
	}
}

__global__ void genShadowMap_compressed(int width, int height, int lightCnt, int slight, float3* glights, int rot_xy, float3* gaxises, float3* gradius, unsigned int *columns, unsigned int* indexes, float3 center, unsigned int precision, unsigned int* ssbo_counter){
	unsigned int x = blockIdx.x * blockDim.x + threadIdx.x;
	unsigned int y = blockIdx.y * blockDim.y + threadIdx.y;
	const unsigned int tid_inblock_const = threadIdx.y*blockDim.x + threadIdx.x;
	const unsigned int threads_num_per_block = blockDim.x*blockDim.y;
	const unsigned int res2 = 1u << (precision - 1);
	const unsigned int resolution = 1u << precision;

	__shared__ float3 lights_sm[_MAX_LIGHT_CNT_PER_PROCESS_];
	{
		unsigned int tid_inblock = tid_inblock_const;
		for (; tid_inblock < lightCnt; tid_inblock += threads_num_per_block){
			lights_sm[tid_inblock] = glights[tid_inblock] - center;
		}
	}
	__syncthreads();

	if (x < width&&y < height){
		float4 tmpf4 = tex2D(norm_ref, (x + 0.5f) / (float(width)), (y + 0.5f) / (float(height)));
		const float3 pnorm = make_float3(tmpf4.x, tmpf4.y, tmpf4.z);
#define _poffset_ 0.6f
		tmpf4 = tex2D(pos_ref, (x + 0.5f) / (float(width)), (y + 0.5f) / (float(height)));
		const float3 ppos = make_float3(tmpf4.x + pnorm.x*_poffset_ - center.x, tmpf4.y + pnorm.y*_poffset_ - center.y, tmpf4.z + pnorm.z*_poffset_ - center.z);

		unsigned int xdir, ydir, rotID;
		float3 axis_x, axis_y, axis_z, radius;

		unsigned int littedCnt = 0;
		for (int l = 0; l < lightCnt; l++){
			float3 lpos = lights_sm[l];
			float3 ray = lpos - ppos;

			if (dot3(ray, pnorm)>0.0){
				{   //find most matching coordinate-system
					ray = normalize(ray);
					float cosx = sqrt(ray.x*ray.x + ray.z*ray.z);
					if (cosx < 1e-6){ xdir = rot_xy / 2, ydir = 0; }
					else{
						if (ray.x>0) cosx = -cosx;
						xdir = unsigned int(acos(cosx) * 57.29578f / 180.0f * rot_xy + 0.5f);
						ydir = unsigned int(acos(ray.z / cosx) * 57.29578f / 180.0f * rot_xy + 0.5f);
						xdir = mclamp<unsigned int>(xdir, 0, rot_xy - 1);
						ydir = mclamp<unsigned int>(ydir, 0, rot_xy - 1);
					}
					rotID = (ydir*rot_xy + xdir);
					radius = gradius[rotID];
					axis_x = gaxises[rotID * 3 + 0];
					axis_y = gaxises[rotID * 3 + 1];
					axis_z = gaxises[rotID * 3 + 2];
				}
				float3 midp = (ppos + ray + lpos) * 0.5f;      ////////////////////////////////////////////!!!!!!!!!!!!!!!!!!!!!!!!!! + ray
				unsigned int midx = unsigned int((dot3(midp, axis_x) / radius.x + 1)*res2);
				unsigned int midy = unsigned int((dot3(midp, axis_y) / radius.y + 1)*res2);

				if (midx < 0 || midx >= resolution || midy < 0 || midy >= resolution){ // line segment didnt intersect with scene.
					littedCnt++;
				}
				else{
					unsigned int z1 = unsigned int((dot3(ppos + ray, axis_z) / radius.z + 1)*res2); //////////////////////////////!!!!!!!!!!!!!!!!!!!+ray
					unsigned int z2 = unsigned int((dot3(lpos, axis_z) / radius.z + 1)*res2);
					if (z1>z2) { unsigned int tmp = z1; z1 = z2; z2 = tmp; }

					if (z1 < resolution && z2 >= 0){
						if (z1 < 0) z1 = 0;
						if (z2 >= resolution) z2 = resolution - 1;
						unsigned int * tptr = &columns[indexes[((rotID << (precision << 1u)) + (midx << precision) + midy)] << (precision - 5)];

						unsigned int zs = z1 >> 5;  unsigned int ze = z2 >> 5;
						unsigned int m1 = z1 & 31u;  unsigned int m2 = z2 & 31u;
						if (zs == ze){ if (!((tptr[zs] << (32 - m2)) >> (32 - m2 + m1))) littedCnt++; }
						else{
							bool flag = false;
							for (int c = zs + 1; c < ze; c++) if (tptr[c]){ flag = true; break; };
							if (!flag){
								if ((!((~((1u << (m1 + 1)) - 1u))&(tptr[zs]))) && (!(((1u << (m2)) - 1u)&(tptr[ze])))){ littedCnt++; }
							}
						}
					}
					else littedCnt++;
				}
			}
		}
		ssbo_counter[y*width + x] += littedCnt;
	}
}

__global__ void calVisibility_uncompressed(int width, int height, int lightCnt, int slight, float3* glights, int rot_xy, float3* gaxises, float3* gradius, unsigned int *gtags, float3 center, unsigned int precision, unsigned int *result){
	unsigned int x = blockIdx.x * blockDim.x + threadIdx.x;
	unsigned int y = blockIdx.y * blockDim.y + threadIdx.y;
	const unsigned int tid_inblock_const = threadIdx.y*blockDim.x + threadIdx.x;
	const unsigned int threads_num_per_block = blockDim.x*blockDim.y;
	const unsigned int res2 = 1u << (precision - 1);
	const unsigned int resolution = 1u << precision;

	__shared__ float3 lights_sm[_MAX_LIGHT_CNT_PER_PROCESS_];
	{
		unsigned int tid_inblock = tid_inblock_const;
		for (; tid_inblock < lightCnt; tid_inblock += threads_num_per_block){
			lights_sm[tid_inblock] = glights[tid_inblock] - center;
		}
	}
	__syncthreads();

	if (x < width&&y < height){
		float4 tmpf4 = tex2D(norm_ref, (x + 0.5f) / (float(width)), (y + 0.5f) / (float(height)));
		const float3 pnorm = make_float3(tmpf4.x, tmpf4.y, tmpf4.z);
#define _poffset_ 0.6f
		tmpf4 = tex2D(pos_ref, (x + 0.5f) / (float(width)), (y + 0.5f) / (float(height)));
		const float3 ppos = make_float3(tmpf4.x + pnorm.x*_poffset_ - center.x, tmpf4.y + pnorm.y*_poffset_ - center.y, tmpf4.z + pnorm.z*_poffset_ - center.z);

		unsigned int xdir, ydir, rotID;
		float3 axis_x, axis_y, axis_z, radius;
		unsigned int *resultptr = result + (y*width + x)*_UINT_CNT_PER_PIXEL_RESULT_;
		for (int l = 0; l < lightCnt; l++){
			float3 lpos = lights_sm[l];
			float3 ray = lpos - ppos;

			if (dot3(ray, pnorm)>0.0){
				{   //find most matching coordinate-system
					ray = normalize(ray);
					float cosx = sqrt(ray.x*ray.x + ray.z*ray.z);
					if (cosx < 1e-6){ xdir = rot_xy / 2, ydir = 0; }
					else{
						if (ray.x>0) cosx = -cosx;
						xdir = unsigned int(acos(cosx) * 57.29578f / 180.0f * rot_xy + 0.5f);
						ydir = unsigned int(acos(ray.z / cosx) * 57.29578f / 180.0f * rot_xy + 0.5f);
						xdir = mclamp<unsigned int>(xdir, 0, rot_xy - 1);
						ydir = mclamp<unsigned int>(ydir, 0, rot_xy - 1);
					}
					rotID = (ydir*rot_xy + xdir);
					radius = gradius[rotID];
					axis_x = gaxises[rotID * 3 + 0];
					axis_y = gaxises[rotID * 3 + 1];
					axis_z = gaxises[rotID * 3 + 2];
				}
				float3 midp = (ppos + ray + lpos) * 0.5f;      ////////////////////////////////////////////!!!!!!!!!!!!!!!!!!!!!!!!!! + ray
				unsigned int midx = unsigned int((dot3(midp, axis_x) / radius.x + 1)*res2);
				unsigned int midy = unsigned int((dot3(midp, axis_y) / radius.y + 1)*res2);

				if (midx < 0 || midx >= resolution || midy < 0 || midy >= resolution){ // line segment didnt intersect with scene.
					resultptr[(l + slight) >> 5u] |= (1u << ((l + slight) & 31u));
				}
				else{
					unsigned int z1 = unsigned int((dot3(ppos + ray, axis_z) / radius.z + 1)*res2); //////////////////////////////!!!!!!!!!!!!!!!!!!!+ray
					unsigned int z2 = unsigned int((dot3(lpos, axis_z) / radius.z + 1)*res2);
					if (z1>z2) { unsigned int tmp = z1; z1 = z2; z2 = tmp; }

					if (z1 < resolution && z2 >= 0){   // line segment didnt intersect with scene.
						if (z1 < 0) z1 = 0;
						if (z2 >= resolution) z2 = resolution - 1;
						unsigned int * tptr = &(gtags[(rotID << (precision * 3 - 5)) + (midx << (precision * 2 - 5)) + (midy << (precision - 5))]);

						unsigned int zs = z1 >> 5;  unsigned int ze = z2 >> 5;
						unsigned int m1 = z1 & 31u;  unsigned int m2 = z2 & 31u;
						if (zs == ze){
							//if (!((tptr[zs] << (31 - m2)) >> (31 - m2 + m1))) resultptr[(l + slight) >> 5u] |= (1u << ((l + slight) & 31u));
							if (!((tptr[zs] << (32 - m2)) >> (32 - m2 + m1))) resultptr[(l + slight) >> 5u] |= (1u << ((l + slight) & 31u));
						}
						else{
							bool flag = false;
							for (int c = zs + 1; c < ze; c++) if (tptr[c]){ flag = true; break; };
							if (!flag){
								if ((!((~((1u << (m1 + 1)) - 1u))&(tptr[zs]))) && (!(((1u << (m2)) - 1u)&(tptr[ze])))){
									//if (x == 167 && y == 275 && l == 10)printf("im here");
									//if ((!((~((1u << m1) - 1u))&(tptr[zs]))) && (!(((1u << (m2 + 1)) - 1u)&(tptr[ze])))){
									///////////////////////////////////////////////////////////////////////////write result////////////////////////////////////////////////////////////////
									resultptr[(l + slight) >> 5u] |= (1u << ((l + slight) & 31u));
								}
							}
						}
					}
					else{
						resultptr[(l + slight) >> 5u] |= (1u << ((l + slight) & 31u));
					}
				}
			}
		}
	}
}

__global__ void genShadowMap_uncompressed(int width, int height, int lightCnt, int slight, float3* glights, int rot_xy, float3* gaxises, float3* gradius, unsigned int *gtags, float3 center, unsigned int precision, unsigned int* ssbo_counter){
	unsigned int x = blockIdx.x * blockDim.x + threadIdx.x;
	unsigned int y = blockIdx.y * blockDim.y + threadIdx.y;
	const unsigned int tid_inblock_const = threadIdx.y*blockDim.x + threadIdx.x;
	const unsigned int threads_num_per_block = blockDim.x*blockDim.y;
	const unsigned int res2 = 1u << (precision - 1);
	const unsigned int resolution = 1u << precision;

	__shared__ float3 lights_sm[_MAX_LIGHT_CNT_PER_PROCESS_];
	{
		unsigned int tid_inblock = tid_inblock_const;
		for (; tid_inblock < lightCnt; tid_inblock += threads_num_per_block){
			lights_sm[tid_inblock] = glights[tid_inblock] - center;
		}
	}
	__syncthreads();

	if (x < width&&y < height){
		float4 tmpf4 = tex2D(norm_ref, (x + 0.5f) / (float(width)), (y + 0.5f) / (float(height)));
		const float3 pnorm = make_float3(tmpf4.x, tmpf4.y, tmpf4.z);
#define _poffset_ 0.6f
		tmpf4 = tex2D(pos_ref, (x + 0.5f) / (float(width)), (y + 0.5f) / (float(height)));
		const float3 ppos = make_float3(tmpf4.x + pnorm.x*_poffset_ - center.x, tmpf4.y + pnorm.y*_poffset_ - center.y, tmpf4.z + pnorm.z*_poffset_ - center.z);

		unsigned int xdir, ydir, rotID;
		float3 axis_x, axis_y, axis_z, radius;
		unsigned int littedCnt = 0;
		for (int l = 0; l < lightCnt; l++){
			float3 lpos = lights_sm[l];
			float3 ray = lpos - ppos;

			if (dot3(ray, pnorm)>0.0){
				{   //find most matching coordinate-system
					ray = normalize(ray);
					float cosx = sqrt(ray.x*ray.x + ray.z*ray.z);
					if (cosx < 1e-6){ xdir = rot_xy / 2, ydir = 0; }
					else{
						if (ray.x>0) cosx = -cosx;
						xdir = unsigned int(acos(cosx) * 57.29578f / 180.0f * rot_xy + 0.5f);
						ydir = unsigned int(acos(ray.z / cosx) * 57.29578f / 180.0f * rot_xy + 0.5f);
						xdir = mclamp<unsigned int>(xdir, 0, rot_xy - 1);
						ydir = mclamp<unsigned int>(ydir, 0, rot_xy - 1);
					}
					rotID = (ydir*rot_xy + xdir);
					radius = gradius[rotID];
					axis_x = gaxises[rotID * 3 + 0];
					axis_y = gaxises[rotID * 3 + 1];
					axis_z = gaxises[rotID * 3 + 2];
				}
				float3 midp = (ppos + ray + lpos) * 0.5f;      ////////////////////////////////////////////!!!!!!!!!!!!!!!!!!!!!!!!!! + ray
				unsigned int midx = unsigned int((dot3(midp, axis_x) / radius.x + 1)*res2);
				unsigned int midy = unsigned int((dot3(midp, axis_y) / radius.y + 1)*res2);

				if (midx < 0 || midx >= resolution || midy < 0 || midy >= resolution){ // line segment didnt intersect with scene.
					littedCnt++;
				}
				else{
					unsigned int z1 = unsigned int((dot3(ppos + ray, axis_z) / radius.z + 1)*res2); //////////////////////////////!!!!!!!!!!!!!!!!!!!+ray
					unsigned int z2 = unsigned int((dot3(lpos, axis_z) / radius.z + 1)*res2);
					if (z1>z2) { unsigned int tmp = z1; z1 = z2; z2 = tmp; }

					if (z1 < resolution && z2 >= 0){
						if (z1 < 0) z1 = 0;
						if (z2 >= resolution) z2 = resolution - 1;
						unsigned int * tptr = &(gtags[(rotID << (precision * 3 - 5)) + (midx << (precision * 2 - 5)) + (midy << (precision - 5))]);

						unsigned int zs = z1 >> 5;  unsigned int ze = z2 >> 5;
						unsigned int m1 = z1 & 31u; unsigned int m2 = z2 & 31u;
						if (zs == ze){ if (!((tptr[zs] << (32 - m2)) >> (32 - m2 + m1))) littedCnt++; }
						else{
							bool flag = false;
							for (int c = zs + 1; c < ze; c++) if (tptr[c]){ flag = true; break; };
							if (!flag){
								if ((!((~((1u << (m1 + 1)) - 1u))&(tptr[zs]))) && (!(((1u << (m2)) - 1u)&(tptr[ze])))){ littedCnt++; }
							}
						}
					}
					else littedCnt++;
				}
			}
		}
		ssbo_counter[y*width + x] += littedCnt;
	}
}

__global__ void compute_error(unsigned int* ours,unsigned int* optix,unsigned char* small_err/*,unsigned int* big_err*/,unsigned int* errs){
	const unsigned int offset = blockIdx.x * blockDim.x + threadIdx.x;
	if (offset == 0){ errs[0] = errs[1] = 0; }
	__syncthreads();
	unsigned int* ours_ptr = &ours[offset*_UINT_CNT_PER_PIXEL_RESULT_];
	unsigned int* optix_ptr = &optix[offset*_UINT_CNT_PER_PIXEL_RESULT_];
	int our_cnt = 0;
	int optix_cnt = 0;
	int diff_cnt = 0;
	for (int i = 0; i < _UINT_CNT_PER_PIXEL_RESULT_; i++){
		diff_cnt += cnt1(ours_ptr[i] ^ optix_ptr[i]);
		our_cnt += cnt1(ours_ptr[i]);
		optix_cnt += cnt1(optix_ptr[i]);
	}
	atomicAdd(errs+0,diff_cnt);
	atomicAdd(errs+1,abs(our_cnt-optix_cnt));

	unsigned char* img_ptr = &small_err[offset * 3];
	if (our_cnt < optix_cnt){
		img_ptr[0] = ((optix_cnt - our_cnt) / 1024.0f * 10 * 255); if (img_ptr[0]>255) img_ptr[0] = 255;
		img_ptr[1] = img_ptr[2] = 0;
	}
	else{
		img_ptr[2] = (our_cnt - optix_cnt) / 1024.0f * 10 * 255; if (img_ptr[2]>255) img_ptr[2] = 255;
		img_ptr[0] = img_ptr[1] = 0;
	}
}

/**************************HOST FUNCTION*************************/
int lightCnt = 1024;
unsigned int scr_width = 512;
unsigned int  scr_height = 512;
unsigned int  rotcnt = 90;
unsigned int  precision = 7;
bool useCompressedColumns = false;
float3 boxcenter;

float3* lights_dev = nullptr;
float3* axises_dev = nullptr;
float3* radius_dev = nullptr;
unsigned int *tags_dev = nullptr;
unsigned int *indexes_dev = nullptr;
unsigned int *columns_dev = nullptr;
unsigned int *visibilities_dev = nullptr;
unsigned int *shadowMapAB_devptr = nullptr;

unsigned char *errorImage_dev = nullptr;
unsigned int *errors_dev = nullptr;

cudaChannelFormatDesc channelDesc;
cudaGraphicsResource *postex_resource, *normtex_resource, *shadowMap_resource;

extern "C" void blocker_initialize_cuda(GLuint postex,GLuint normtex,GLuint shadowmap,
	                                                      int lcnt,unsigned int width,unsigned int height,unsigned int rcnt,unsigned int columnCnt,unsigned int pres,bool compressed,redips::float3 heart,
														  const float* lights,const float* axises,const float* radius,const unsigned int* tags,const unsigned int* indexes,const unsigned int* columns)
{
	checkCudaErrors(cudaGraphicsGLRegisterImage(&postex_resource, postex, GL_TEXTURE_2D, cudaGraphicsMapFlagsReadOnly));
	checkCudaErrors(cudaGraphicsGLRegisterImage(&normtex_resource, normtex, GL_TEXTURE_2D, cudaGraphicsMapFlagsReadOnly));
	checkCudaErrors(cudaGraphicsGLRegisterBuffer(&shadowMap_resource, shadowmap, cudaGraphicsMapFlagsWriteDiscard));
	
	channelDesc = cudaCreateChannelDesc(32, 32, 32, 32, cudaChannelFormatKindFloat);
	
	pos_ref.addressMode[0] = cudaAddressModeWrap;
	pos_ref.addressMode[1] = cudaAddressModeWrap;
	pos_ref.filterMode = cudaFilterModeLinear;
	pos_ref.normalized = true;

	norm_ref.addressMode[0] = cudaAddressModeWrap;
	norm_ref.addressMode[1] = cudaAddressModeWrap;
	norm_ref.filterMode = cudaFilterModeLinear;
	norm_ref.normalized = true;

	lightCnt = lcnt;
	scr_width = width;
	scr_height = height;
	rotcnt = rcnt;
	precision = pres;
	useCompressedColumns = compressed;
	boxcenter = make_float3(heart.x,heart.y,heart.z);

	//COMMON
	checkCudaErrors(cudaMalloc((void**)&lights_dev, sizeof(float)* 3 * lightCnt));
	checkCudaErrors(cudaMemcpy(lights_dev, lights, sizeof(float)* 3 * lightCnt, cudaMemcpyHostToDevice));

	checkCudaErrors(cudaMalloc((void**)&axises_dev, sizeof(float)* 9 * rotcnt*rotcnt));
	checkCudaErrors(cudaMemcpy(axises_dev, axises, sizeof(float)* 9 * rotcnt*rotcnt, cudaMemcpyHostToDevice));

	checkCudaErrors(cudaMalloc((void**)&radius_dev, sizeof(float)* 3 * rotcnt *rotcnt));
	checkCudaErrors(cudaMemcpy(radius_dev, radius, sizeof(float)* 3 * rotcnt*rotcnt, cudaMemcpyHostToDevice));
	
	if (useCompressedColumns){
		checkCudaErrors(cudaMalloc((void**)&columns_dev, (columnCnt << (precision - 5))*sizeof(unsigned int)));
		checkCudaErrors(cudaMemcpy(columns_dev, columns, sizeof(unsigned int)*(columnCnt << (precision - 5)), cudaMemcpyHostToDevice));
		checkCudaErrors(cudaMalloc((void**)&indexes_dev, sizeof(unsigned int)*rotcnt*rotcnt*(1u << (precision * 2))));
		checkCudaErrors(cudaMemcpy(indexes_dev, indexes, sizeof(unsigned int)*rotcnt*rotcnt*(1u << (precision * 2)), cudaMemcpyHostToDevice));
	}
	else{
		checkCudaErrors(cudaMalloc((void**)&tags_dev, (1u << (precision * 3 - 3))*rotcnt*rotcnt));
		checkCudaErrors(cudaMemcpy(tags_dev, tags, (1u << (precision * 3 - 3))*rotcnt*rotcnt, cudaMemcpyHostToDevice));
	}

	//for result
	checkCudaErrors(cudaMalloc((void**)&visibilities_dev, sizeof(unsigned int)*_UINT_CNT_PER_PIXEL_RESULT_*scr_width*scr_height));

	//for error
	checkCudaErrors(cudaMalloc((void**)&errorImage_dev,sizeof(unsigned char)*3*scr_width*scr_height));
	checkCudaErrors(cudaMalloc((void**)&errors_dev, sizeof(unsigned int)* 2));
	/***********************delete********************/
	unsigned int* tmp = new unsigned int[512*512*32];
	std::ifstream fin("D:/孟学长程序-用完即删/AnalyseData/sponza/rtresult0");   
	for (int i = 0; i < 512*512*32; i++) {
		fin >> tmp[i];
	}
	fin.close();
	cudaMemcpy(visibilities_dev,tmp,sizeof(unsigned int)*512*512*32,cudaMemcpyHostToDevice);
	/***********************delete********************/
	checkCudaErrors(cudaDeviceSynchronize());
}

extern "C" void launch_4_compare(){ //for compare
	cudaArray *pos_array, *norm_array;
	//map position texture
	checkCudaErrors(cudaGraphicsMapResources(1, &postex_resource, 0));
	checkCudaErrors(cudaGraphicsSubResourceGetMappedArray(&pos_array, postex_resource, 0, 0));
	//map normal texture
	checkCudaErrors(cudaGraphicsMapResources(1, &normtex_resource, 0));
	checkCudaErrors(cudaGraphicsSubResourceGetMappedArray(&norm_array, normtex_resource, 0, 0));
	cudaBindTextureToArray(pos_ref, pos_array, channelDesc);
	cudaBindTextureToArray(norm_ref, norm_array, channelDesc);
	thrust::device_ptr<unsigned int > vis_ptr(visibilities_dev);       //may waste time
	thrust::fill(vis_ptr, vis_ptr + scr_width*scr_height*_UINT_CNT_PER_PIXEL_RESULT_, 0u);
	checkCudaErrors(cudaDeviceSynchronize());

	dim3 blockDim(32, 32);
	dim3 gridDim((scr_width - 1) / 32 + 1, (scr_height - 1) / 32 + 1);
	for (int sid = 0; sid < lightCnt; sid += _MAX_LIGHT_CNT_PER_PROCESS_){
		int eid = sid + _MAX_LIGHT_CNT_PER_PROCESS_;  if (eid>lightCnt) eid = lightCnt;
		if (useCompressedColumns){
			calVisibility_compressed << <gridDim, blockDim >> >(scr_width, scr_height, eid - sid, sid, lights_dev + sid, rotcnt, axises_dev, radius_dev, columns_dev, indexes_dev, boxcenter, precision, visibilities_dev);
		}
		else{
			calVisibility_uncompressed << <gridDim, blockDim >> >(scr_width, scr_height, eid - sid, sid, lights_dev + sid, rotcnt, axises_dev, radius_dev, tags_dev, boxcenter, precision, visibilities_dev);
		}
		checkCudaErrors(cudaDeviceSynchronize());
		if (eid >= lightCnt) break;
	}

	checkCudaErrors(cudaGraphicsUnmapResources(1, &postex_resource, 0));
	checkCudaErrors(cudaGraphicsUnmapResources(1, &normtex_resource, 0));
}

extern "C" void launch_4_rendering(){ //for rendering
	//map position texture
	cudaArray *pos_array, *norm_array;
	checkCudaErrors(cudaGraphicsMapResources(1, &postex_resource, 0));
	checkCudaErrors(cudaGraphicsSubResourceGetMappedArray(&pos_array, postex_resource, 0, 0));
	//map normal texture
	checkCudaErrors(cudaGraphicsMapResources(1, &normtex_resource, 0));
	checkCudaErrors(cudaGraphicsSubResourceGetMappedArray(&norm_array, normtex_resource, 0, 0));
	cudaBindTextureToArray(pos_ref, pos_array, channelDesc);
	cudaBindTextureToArray(norm_ref, norm_array, channelDesc);
	//map shadow-map buffer
	size_t num_bytes;
	checkCudaErrors(cudaGraphicsMapResources(1, &shadowMap_resource, 0));
	checkCudaErrors(cudaGraphicsResourceGetMappedPointer((void **)&shadowMapAB_devptr, &num_bytes, shadowMap_resource));
	thrust::device_ptr<unsigned int > ssbo_ptr(shadowMapAB_devptr);       //may waste time
	thrust::fill(ssbo_ptr, ssbo_ptr + scr_width*scr_height, 0u);
	checkCudaErrors(cudaDeviceSynchronize());

	dim3 blockDim(32, 32);
	dim3 gridDim((scr_width - 1) / 32 + 1, (scr_height - 1) / 32 + 1);

	float elapseTime; cudaEvent_t start, stop;	cudaEventCreate(&start);  cudaEventCreate(&stop);
	cudaEventRecord(start, 0);

	for (int sid = 0; sid < lightCnt; sid += _MAX_LIGHT_CNT_PER_PROCESS_){
		int eid = sid + _MAX_LIGHT_CNT_PER_PROCESS_;  if (eid>lightCnt) eid = lightCnt;
		if (useCompressedColumns){
			genShadowMap_compressed << <gridDim, blockDim >> >(scr_width, scr_height, eid - sid, sid, lights_dev + sid, rotcnt, axises_dev, radius_dev, columns_dev, indexes_dev, boxcenter, precision, shadowMapAB_devptr);
		}
		else{
			genShadowMap_uncompressed << <gridDim, blockDim >> >(scr_width, scr_height, eid - sid, sid, lights_dev + sid, rotcnt, axises_dev, radius_dev, tags_dev, boxcenter, precision, shadowMapAB_devptr);
		}
		checkCudaErrors(cudaDeviceSynchronize());
		if (eid >= lightCnt) break;
	}

	cudaEventRecord(stop, 0);
	cudaEventSynchronize(stop);
	cudaEventElapsedTime(&elapseTime, start, stop);
	//printf("[blocker] :calculate visibility cost %.2f ms\n", elapseTime);

	checkCudaErrors(cudaGraphicsUnmapResources(1, &postex_resource, 0));
	checkCudaErrors(cudaGraphicsUnmapResources(1, &normtex_resource, 0));
	checkCudaErrors(cudaGraphicsUnmapResources(1, &shadowMap_resource, 0));
}

cudaGraphicsResource *optix_buffer_resource;
extern "C" void compute_errors(GLuint optix_buffer,unsigned char* img_cpu,redips::int2& ret){
	size_t num_bytes; unsigned int* optix_buffer_ptr;
	checkCudaErrors(cudaGraphicsGLRegisterBuffer(&optix_buffer_resource, optix_buffer, cudaGraphicsMapFlagsReadOnly));
	checkCudaErrors(cudaGraphicsMapResources(1, &optix_buffer_resource, 0));
	checkCudaErrors(cudaGraphicsResourceGetMappedPointer((void **)&optix_buffer_ptr, &num_bytes, optix_buffer_resource));

	compute_error<<<scr_height, scr_width >>>(visibilities_dev, optix_buffer_ptr, errorImage_dev, errors_dev);
	checkCudaErrors(cudaDeviceSynchronize());
	cudaMemcpy(img_cpu,errorImage_dev,sizeof(unsigned int)*3*scr_width*scr_height,cudaMemcpyDeviceToHost);
	cudaMemcpy(&ret.x, errors_dev,sizeof(unsigned int)*2,cudaMemcpyDeviceToHost);
	checkCudaErrors(cudaDeviceSynchronize());
	checkCudaErrors(cudaGraphicsUnmapResources(1, &optix_buffer_resource, 0));
}

extern "C" void clean_cuda_blocker(){
	if (lights_dev) checkCudaErrors(cudaFree(lights_dev));              lights_dev = nullptr;
	if (axises_dev) checkCudaErrors(cudaFree(axises_dev));             axises_dev = nullptr;
	if (radius_dev) checkCudaErrors(cudaFree(radius_dev));             radius_dev = nullptr;
	if (tags_dev) checkCudaErrors(cudaFree(tags_dev));                  tags_dev = nullptr;
	if (indexes_dev) checkCudaErrors(cudaFree(indexes_dev));        indexes_dev = nullptr;
	if (columns_dev) checkCudaErrors(cudaFree(columns_dev));     columns_dev = nullptr;
	if (visibilities_dev) checkCudaErrors(cudaFree(visibilities_dev));   visibilities_dev = nullptr;
	if (errorImage_dev) checkCudaErrors(cudaFree(errorImage_dev)); errorImage_dev = nullptr;
	if (errors_dev) checkCudaErrors(cudaFree(errors_dev)); errors_dev = nullptr;
}