#pragma once
#include <vec.h>
#include <camera/phc.h>
#include <geos/triangles.h>
#include <geos/particles.h>
#include "ColumnChooser.h"
#define BASE_PATH "C:/del/"
using namespace redips;
class Compare256{
public:
	Compare256(const Triangles* model, uint rotx, uint roty, uint precision) :rotx(rotx), roty(roty), precision(precision){
		resolution = (1u << precision);
		BOX box = model->aabb(), tbox;
		mcenter = box.heart();
		float3 DIM = box.dim();
		for (int y = 0; y < roty; y++) for (int x = 0; x < rotx; x++) {
			int id = y*rotx + x;
			rmats[id] = Mat33f::tilt(RAD(x*180.0f / rotx))*Mat33f::pan(RAD(y*180.0f / roty));
			for (unsigned int i = 0; i < 8; i++){ tbox += (rmats[id] * (DIM*-0.5f + (DIM*float3::bits(i)))); }
			float3 dim = tbox.dim();
			pmats[id] = GeoUtil::glOrtho(dim.x*-0.5f, dim.x*0.5f, dim.y*-0.5f, dim.y*0.5f, dim.z*-0.5f, dim.z*0.5f)*Mat44f(rmats[id]);
			tbox.reset();
		}

		uint_cnt_per_direction = 1u << (precision * 3 - 5);

		puts("[Compare] : init finish");
	};
	~Compare256(){};
	string split(unsigned int data){
		string ret; ret.resize(32);
		for (int i = 0; i < 32; i++){
			ret[i] = (data & 1u)>0 ? '1' : '0';
			data >>= 1;
		}
		return ret;
	};
	bool check(float3 start, float3 end){
		float3 ray = end - start;
		start = start - mcenter;        //转换到物体坐标系，这样其实是不对的，
		end = end - mcenter;

		//1 找到最合适的方向
		int maxId = 0;
		float maxl = fabs(ray.dot(rmats[0].z()));
		for (int i = 1; i < 8100; i++){
			float tmp = fabs(ray.dot(rmats[i].z()));
			if (tmp>maxl) maxl = tmp, maxId = i;
		}
		//2 找到该方向下的xy
		float3 center = (start + end)*0.5f;
		float3 midp = (pmats[maxId] * float4(center, 1.0f)).vec3();
		uint x = ((midp.x*0.5f + 0.5f)*resolution + 0.5f);
		uint y = ((midp.y*0.5f + 0.5f)*resolution + 0.5f);
		if (x < 0 || x >= resolution) return false;
		if (y < 0 || y >= resolution) return false;
		//x = CLAMP(x, 0, (resolution - 1));
		//y = CLAMP(y, 0, (resolution - 1));
		//3 找到该方向下的z1,z2
		float3 z1 = (pmats[maxId] * float4(start, 1.0f)).vec3();
		float3 z2 = (pmats[maxId] * float4(end, 1.0f)).vec3();
		uint zs = (z1.z*0.5f + 0.5f)*resolution + 0.5f;
		uint ze = (z2.z*0.5f + 0.5f)*resolution + 0.5f;
		zs = CLAMP(zs, 0, (resolution - 1));
		ze = CLAMP(ze, 0, (resolution - 1));
		if (zs>ze) swap(zs, ze);
		
		//4. 取出该列
		int blockId = 0;
		auto iter = mapper.find(maxId);
		if (iter == mapper.end()){
			blockId = dirCntLoaded++;
			mapper[maxId] = blockId;
			load("E:/Documents/papers/ppp/ppp/result/trees/256/",maxId%rotx,maxId/rotx);
		}
		else{
			blockId = iter->second;
		}
		
		Column256 curc = container[blockId*resolution*resolution + x*resolution + y];
		
		//判断是否遮挡
		int sid = zs / 32;
		int eid = ze / 32;
		for (int id = sid + 1; id < eid; id++) if (curc.v[id]>0) return true;

		uint spos = zs % 32;
		uint epos = ze % 32;
		if (curc.v[eid] & ((1u << (epos + 1)) - 1)) return true;
		if (curc.v[sid] & ((~((1u << spos) - 1u)))) return true;
		return false;
	}
	void load(const char * basepath,int x,int y){
		Column256 tmp;
		const uint column_per_file = resolution * resolution;
		sprintf(strBuffer, "%sx%02d_y%02d.txt", basepath, x, y);
		//printf("[Column Chooser] : loading %s ... \n", strBuffer);
		freopen(strBuffer, "r", stdin);
		for (int z = 0; z < column_per_file; z++){
			  for (int i = 0; i < 8; i++) scanf("%u",&tmp[i]);
			  container.push_back(tmp);
		}
		fclose(stdin);
	}
private:
	int dirCntLoaded=0;
	float3 mcenter;
	uint rotx, roty;
	unsigned int precision;
	unsigned int resolution;
	Mat33f rmats[8100];
	Mat44f pmats[8100];
	char strBuffer[222];
	std::map<int, int> mapper;
	std::vector<Column256>  container;
	unsigned int * indexes;
	unsigned int uint_cnt_per_direction;
};

