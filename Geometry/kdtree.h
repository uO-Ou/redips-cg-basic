/*
* Author : redips redips.xin@gmail.com
* Date : 2017.12.19
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
	private:
		//HookCmper is used for compare HOOKs by one dimension
		class HookCmper{
		public:
			explicit HookCmper(BYTE dim):dim(dim){ }
			bool operator()(const HOOK& a,const HOOK& b){
				return (a.second->heart())[dim] < (b.second->heart())[dim];
			}
		private:
			BYTE dim;
		};
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
		~KDTree(){
			delete boxes;
			delete splitDims;
			delete splitValues;
			delete gspos; delete gcnts;
			delete lchild; delete rchild;
		};

		void buildTree(std::vector<HOOK>* hooks){
			hooksPtr = hooks;
			hooksCnt = hooks->size();
			resize(hooksCnt * 4);
			clock_t start, finish;
			start = clock();
			memset(lchild, -1, sizeof(int)*hooksCnt * 4);
			memset(rchild, -1, sizeof(int)*hooksCnt * 4);
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
		
		int split(int spos, int cnt, float &splitValue, BYTE &splitPanel){
			float3 mean, square;
			for (int i = spos; i < spos + cnt; i++){ mean += ((*hooksPtr)[i].second->heart()); }
			mean *= (1.0f / cnt);
			for (int i = spos; i < spos + cnt; i++){ square += (((*hooksPtr)[i].second->heart()) - mean).square(); }

			splitPanel = square.maxdim();
			//sort by maxdim
			std::sort((*hooksPtr).begin() + spos, (*hooksPtr).begin() + spos + cnt, HookCmper(splitPanel));

			splitValue = ((*((*hooksPtr)[spos + cnt / 2].second)).lbb)[splitPanel];

			return spos + cnt / 2;
			//return binarySearch(splitValue, spos, spos + cnt - 1, splitPanel);
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
		int boxCnt,hooksCnt;
		BOX *boxes;
		BYTE *splitDims;
		float *splitValues;
		int *gspos, *gcnts;
		int *lchild, *rchild;

		void resize(int n){
			if (boxes){
				delete boxes;
				delete splitDims; delete splitValues;
				delete gspos;     delete gcnts;
				delete lchild;      delete rchild;
			}
			boxCnt = 0;

			if (!n) return;

			boxes = new BOX[n];
			splitDims = new BYTE[n];
			splitValues = new float[n];
			gspos = new int[n];	gcnts = new int[n]; 
			lchild = new int[n];	rchild = new int[n];
		}
	};
};


