/*
* Author : redips redips.xin@gmail.com
* Date : 2017.12.9
* Description : kdtree
*/
#pragma once

#include <ctime>
#include <vector>
#include <algorithm>
#include "geometry.h"

//bug, what if a node has geometry's count larger than MAX_LEAF_SIZE,but after divide,all its geometry allocate to one child;
namespace redips{
	class KDTree{
	public:
		KDTree(){
			boxes = nullptr;
			splitDims = nullptr;
			splitValues = nullptr;
			gspos = gcnts = nullptr;
			lchild = rchild = nullptr;
			boxCnt = 0;
			MAX_LEAF_SIZE = 16;
		};
		~KDTree(){};

		void buildTree(std::vector<HOOK>* hooks){
			hooksPtr = hooks;
			resize(hooksPtr->size() * 4);
			clock_t start, finish;
			start = clock();
			build(0, 0, hooksPtr->size());
			finish = clock();
			printf("[kd-tree builder] : build finish,%d node,cost %.2lfms\n", boxCnt, (double)(finish - start) / CLOCKS_PER_SEC * 1000);
		}
		void setLeafSize(int size) { MAX_LEAF_SIZE = size; };
	private:
		void build(int id, int spos, int cnt){
			//printf("building %d : %d-%d\n",id,spos,spos+cnt);
			gcnts[id] = cnt;
			gspos[id] = spos;
			if (cnt <= MAX_LEAF_SIZE) {
				for (int i = spos; i < spos + cnt; i++){ boxes[id] += *((*hooksPtr)[i].second); }
				return;
			}

			int pos = split(spos, cnt, splitValues[id], splitDims[id]);

			if (spos < pos) {
				lchild[id] = (++boxCnt);
				build(lchild[id], spos, pos - spos);
				boxes[id] += boxes[lchild[id]];
			}
			if (pos < spos + cnt){
				rchild[id] = (++boxCnt);
				build(rchild[id], pos, spos + cnt - pos);
				boxes[id] += boxes[rchild[id]];
			}
		}
		//bug, what if a node has geometry's count larger than MAX_LEAF_SIZE,but after divide,all its geometry allocate to one child;
		int split_old(int spos, int cnt, float &splitValue, BYTE &splitPanel){
			float3 mean, square;
			for (int i = spos; i < spos + cnt; i++){ mean += ((*hooksPtr)[i].second->heart()); }
			mean *= (1.0f / cnt);
			for (int i = spos; i < spos + cnt; i++){ square += (((*hooksPtr)[i].second->heart()) - mean).square(); }

			splitPanel = square.maxdim();
			switch (splitPanel){
			case 0:
				std::sort((*hooksPtr).begin() + spos, (*hooksPtr).begin() + spos + cnt, cmpByHookX);
				splitValue = mean.x;
				break;
			case 1:
				std::sort((*hooksPtr).begin() + spos, (*hooksPtr).begin() + spos + cnt, cmpByHookY);
				splitValue = mean.y;
				break;
			case 2:
				std::sort((*hooksPtr).begin() + spos, (*hooksPtr).begin() + spos + cnt, cmpByHookZ);
				splitValue = mean.z;
				break;
			}
			return binarySearch(splitValue, spos, spos + cnt - 1, splitPanel);
		}

		int split(int spos, int cnt, float &splitValue, BYTE &splitPanel){
			float3 mean, square;
			for (int i = spos; i < spos + cnt; i++){ mean += ((*hooksPtr)[i].second->heart()); }
			mean *= (1.0f / cnt);
			for (int i = spos; i < spos + cnt; i++){ square += (((*hooksPtr)[i].second->heart()) - mean).square(); }

			splitPanel = square.maxdim();
			switch (splitPanel){
			case 0:
				std::sort((*hooksPtr).begin() + spos, (*hooksPtr).begin() + spos + cnt, cmpByHookX);
				splitValue = (*((*hooksPtr)[spos + cnt / 2].second)).lbb.x;
				break;
			case 1:
				std::sort((*hooksPtr).begin() + spos, (*hooksPtr).begin() + spos + cnt, cmpByHookY);
				splitValue = (*((*hooksPtr)[spos + cnt / 2].second)).lbb.y;
				break;
			case 2:
				std::sort((*hooksPtr).begin() + spos, (*hooksPtr).begin() + spos + cnt, cmpByHookZ);
				splitValue = (*((*hooksPtr)[spos + cnt / 2].second)).lbb.z;
				break;
			}
			return spos + cnt / 2;
			//return binarySearch(splitValue, spos, spos + cnt - 1, splitPanel);
		}

		static bool cmpByHookX(const HOOK &a, const HOOK &b){
			return a.second->heart().x < b.second->heart().x;
		}
		static bool cmpByHookY(const HOOK &a, const HOOK &b){
			return a.second->heart().y < b.second->heart().y;
		}
		static bool cmpByHookZ(const HOOK &a, const HOOK &b){
			return a.second->heart().z < b.second->heart().z;
		}
		int binarySearch(float data, int spos, int epos, BYTE dim){
			int low = spos, high = epos;
			while (low <= high){
				int mid = (low + high) >> 1;
				float diff = (*hooksPtr)[mid].second->lbb[dim] - data;
				if (fabs(diff) < 1e-2) return mid;
				if (diff < 0) low = mid + 1;
				else high = mid - 1;
			}
			return low;
		}
	public:
		int MAX_LEAF_SIZE;
		std::vector<HOOK>* hooksPtr;
		// each box is a tree-node;
		// boxes[0] is root. boxes[i] contains hooks[gspos[i],gspos+gcnts[i])
		// boxes[i] split to boxes[lchild[i]] and boxes[rchild[i]] based on splitDims[i]'s splitValues[i] when gcnts[i] > MAX_LEAF_SIZE
		int boxCnt;
		BOX *boxes;
		BYTE *splitDims;
		float *splitValues;
		int *gspos, *gcnts;
		int *lchild, *rchild;

		void resize(int n){
			if (boxes){
				delete boxes;
				delete splitDims;
				delete splitValues;
				delete gspos; delete gcnts;
				delete lchild; delete rchild;
			}
			boxCnt = 0;

			if (!n) return;

			boxes = new BOX[n];
			splitDims = new BYTE[n];
			splitValues = new float[n];
			gspos = new int[n];	gcnts = new int[n]; lchild = new int[n];	rchild = new int[n];

			memset(lchild, -1, sizeof(int)*n);
			memset(rchild, -1, sizeof(int)*n);
		}
	};
};


