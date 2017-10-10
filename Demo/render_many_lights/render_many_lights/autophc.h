#pragma once
#include <camera/phc.h>
#include <vector>
#include <fstream>
#include <iostream>
using namespace std;

//ÃÏÑ§³¤header
#include "FrameTimer.h"

class AutoPhc : public redips::PHC{
public:
	AutoPhc(float hAov, float aspect/*width/height*/, float nearp, float farp, const char * file) :PHC(hAov, aspect, nearp, farp, "moving phc"){
		ifstream fin(file);
		fin >> poscnt;  params.resize(poscnt);
	    redips::float3 tmp;
		for (int i = 0; i < poscnt; i++){
			for (int j = 0; j < 3; j++)  {
				fin >> tmp[0] >> tmp[1] >> tmp[2];
				params[i].setrow(tmp,j);
			}
		}
		gapcnt = poscnt - 1;
		intervals.resize(gapcnt);
		for (int i = 0; i < gapcnt; i++) { 
			fin >> intervals[i]; 
			totalLen += intervals[i];
		}
		fin.close();
	}
	void update(){
		
		int curFrame = F_CUR_FRAME%totalLen;

		int phaseId = 0;
		int sumLen = 0;
		for (int i = 0; i<gapcnt; ++i) {
			if (curFrame <= intervals[i]){
				sumLen = intervals[i];
				phaseId = i;
				break;
			}
			curFrame -= intervals[i];
		}
		int start = phaseId;
		double rate = 1.0f*curFrame / sumLen;

		float3 pos = params[start].x()*(1 - rate) + params[start + 1].x()*rate;
		float3 focus = params[start].y()*(1 - rate) + params[start + 1].y()*rate;
		float3 up = params[start].z()*(1 - rate) + params[start + 1].z()*rate;

		this->lookAt(pos,focus,up);
		updateExtrinsic();
	}
	~AutoPhc(){}
private:
	std::vector<redips::Mat33f> params;
	std::vector<int > intervals;
	int totalLen = 0;
	int poscnt = 0,gapcnt = 0;
};