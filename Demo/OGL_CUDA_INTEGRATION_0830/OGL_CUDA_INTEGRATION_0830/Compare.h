#pragma once
#include <vec.h>
#include <camera/phc.h>
#include <geos/triangles.h>
#include <geos/particles.h>
#include "ColumnChooser.h"
#define BASE_PATH "C:/del/"
using namespace redips;
class Compare{
public:
	Compare(const Triangles* model,uint rotx, uint roty, uint precision):rotx(rotx),roty(roty),precision(precision){
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

		load128("E:/Documents/papers/ppp/ppp/result/garden/128/");

		puts("[Compare] : init finish");
	};
	~Compare(){};
	bool check(float3 start,float3 end){
		   float3 ray = end - start;
		   start = start - mcenter;        //转换到物体坐标系，这样其实是不对的，
		   end = end - mcenter;

		   //找到最合适的方向
		   int maxId = 0;
		   float maxl = fabs(ray.dot(rmats[0].z()));
		   for (int i = 1; i < 8100; i++){
			   float tmp = fabs(ray.dot(rmats[i].z()));
			   if (tmp>maxl) maxl = tmp, maxId = i;
		   }
		   //printf(" [%f] ",ray.unit().dot(rmats[maxId].z()));

		   //找到该方向下的xy
		   float3 center = (start + end)*0.5f;
		   float3 midp = (pmats[maxId] * float4(center,1.0f)).vec3();
		   uint x = ((midp.x*0.5f + 0.5f)*resolution + 0.5f);
		   uint y = ((midp.y*0.5f + 0.5f)*resolution + 0.5f);
		   x = CLAMP(x, 0, (resolution - 1));
		   y = CLAMP(y, 0, (resolution - 1));

		   //找到该方向下的z1,z2
		   float3 z1 = (pmats[maxId] * float4(start, 1.0f)).vec3();
		   float3 z2 = (pmats[maxId] * float4(end, 1.0f)).vec3();
		   uint zs = (z1.z*0.5f + 0.5f)*resolution + 0.5f;
		   uint ze = (z2.z*0.5f + 0.5f)*resolution + 0.5f;
		   zs = CLAMP(zs, 0, (resolution - 1));
		   ze = CLAMP(ze, 0, (resolution - 1));
		   if (zs>ze) swap(zs,ze);

		   Column128 curc = container[maxId*resolution*resolution+x*resolution+y];

		   int sid = zs / 32;
		   int eid = ze / 32;
		   for (int id = sid + 1; id < eid; id++) if (curc.v[id]>0) return true;

		   uint spos = zs % 32;
		   uint epos = ze % 32;
		   if (curc.v[eid] & ((1u << (epos + 1)) - 1)) return true;
		   if (curc.v[sid] & ((~((1u << spos) - 1u)))) return true;
		   return false;
	}
	void load(const char* columns,const char* index,unsigned int columnCnt){
		choosedColumn.resize(columnCnt);
		freopen(columns,"r",stdin);
		Column128 tmp; uint cnt;
		for (int i = 0; i < columnCnt; i++){
			scanf("%u %u %u %u %u", &tmp.v[0], &tmp.v[1], &tmp.v[2], &tmp.v[3], &cnt);
			choosedColumn[i] = tmp;
		}
		fclose(stdin);
		std::sort(choosedColumn.begin(), choosedColumn.end());
		
		unsigned int idCnts = uint_cnt_per_direction * 8100;
		indexes = new unsigned int[idCnts];
		ifstream fin(index);
		for (int i = 0; i < idCnts; i++) {
			fin >> indexes[i] ;
			if (i % 5308416 == 0) printf("%d/100\n", i / 5308416);
		}
		fclose(stdin);
	}
	void load128(const char * basepath){
		container.resize(rotx*roty*resolution*resolution);
		uint idx = 0;
		Column128 tmp;
		const uint column_per_file = resolution * resolution;
		for (int y = 0; y < roty; y++) {
			printf("[Column Chooser] : loading y %d \n", y);
			for (int x = 0; x < rotx; x++) {
				sprintf(strBuffer, "%sx%02d_y%02d.txt", basepath, x, y);
				freopen(strBuffer, "r", stdin);
				for (int z = 0; z < column_per_file; z++){
					scanf("%u %u %u %u", &tmp.v[0], &tmp.v[1], &tmp.v[2], &tmp.v[3]);
					container[idx++] = tmp;
				}
			}
		}
		fclose(stdin);
	}
private:
	float3 mcenter;
	uint rotx, roty;
	unsigned int precision;
	unsigned int resolution;
	const Particles* lights;
	Mat33f rmats[8100];
	Mat44f pmats[8100];
	char strBuffer[222];
	std::vector<Column128>  choosedColumn;
	std::vector<Column128>  container;
	unsigned int * indexes;
	unsigned int uint_cnt_per_direction;
};

